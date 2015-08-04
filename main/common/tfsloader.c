/**************************************************************************
 *
 * Copyright (c) 2013 Alcatel-Lucent
 *
 * Alcatel Lucent licenses this file to You under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License.  A copy of the License is contained the
 * file LICENSE at the top level of this repository.
 * You may also obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************
 *
 * tfsloader.c:
 *
 * This file contains the code that is specific to each of the file types
 * supported by TFS as the binary executable type.
 * Currently, COFF, ELF, A.OUT and MSBIN are supported.  This requires that
 * TFS_EBIN_COFF, TFS_EBIN_ELF, TFS_EBIN_AOUT or TFS_EBIN_MSBIN
 * respectively, be set in the monitor's config.h file.  Also, defining
 * TFS_EBIN_ELFMSBIN will allow TFS to support both ELF and MSBIN.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "stddefs.h"
#include "genlib.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "coff.h"
#include "elf.h"
#include "aout.h"
#include "msbin.h"
#if INCLUDE_TFS


/* tfsld_memcpy(), tfsld_memset() & tfsld_decompress():
 * Front end to all bulk memory modification calls done by the
 * loader.  Each function just does a quick check to make sure that
 * the address ranged being copied to is not part of the monitor's
 * own .bss space...
 * Then after the memory transfer, caches are flushed/invalidated.
 *
 * How we return here depends on the verbosity level.  If verbosity
 * is greater than 1, then assume that this pass through tfsld_memcpy()
 * is to dump out map information, not to actually do a memcpy().
 * This same logic is used within the s_memcpy() routine and must remain
 * consistent to work properly.
 */
#ifdef USE_ALTERNATE_TFSLD_MEMCPY
extern int tfsld_memcpy(char *to,char *from,int count,int verbose,int verify);
#else
int
tfsld_memcpy(char *to, char *from, int count, int verbose, int verify)
{
    int rc;

    if(verbose <= 1) {
        if(inUmonBssSpace(to,to+count-1)) {
            return(-1);
        }
    }

    rc = s_memcpy(to,from,count,verbose,verify);

    if(verbose <= 1) {
        flushDcache(to,count);
        invalidateIcache(to,count);
    }

    return(rc);
}
#endif

int
tfsld_memset(uchar *to,uchar val,int count,int verbose,int verifyonly)
{
    int rc;

    if(verbose <= 1) {
        if(inUmonBssSpace((char *)to, (char *)to+count-1)) {
            return(-1);
        }
    }

    rc = s_memset(to,val,count,verbose,verifyonly);

    if(verbose <= 1)  {
        flushDcache((char *)to, count);
        invalidateIcache((char *)to, count);
    }

    return(rc);
}

void
showEntrypoint(unsigned long entrypoint)
{
    printf(" entrypoint: 0x%lx\n",entrypoint);
}

void
showSection(char *sname)
{
    printf(" %-10s: ",sname);
}

#if TFS_EBIN_AOUT

/* tfsloadaout():
 *  The file pointed to by fp is assumed to be an AOUT
 *  formatted file.  This function loads the sections of that file into
 *  the designated locations and returns the address of the entry point.
 */
int
tfsloadaout(TFILE *fp,int verbose,long *entrypoint,char *sname,int verifyonly)
{
    uchar   *tfrom, *dfrom;
    struct  exec *ehdr;

    if(tfsTrace) {
        printf("tfsloadaout(%s)\n",TFS_NAME(fp));
    }

    /* Establish file header pointer... */
    ehdr = (struct exec *)(tfsBase(fp));

    /* Return error if relocatable... */
    if((ehdr->a_trsize) || (ehdr->a_drsize)) {
        return(TFSERR_BADHDR);
    }

    /* Establish locations from which text and data are to be */
    /* copied from ... */
    tfrom = (uchar *)(ehdr+1);
    dfrom = tfrom+ehdr->a_text;

    /* Copy/verify text and data sections to RAM: */
    if(verbose) {
        showSection("text");
    }

    if(tfsld_memcpy((char *)(ehdr->a_entry),(char *)tfrom,
                    ehdr->a_text,verbose,verifyonly) != 0) {
        return(TFSERR_MEMFAIL);
    }

    if(verbose) {
        showSection("data");
    }

    if(tfsld_memcpy((char *)(ehdr->a_entry+ehdr->a_text),(char *)dfrom,
                    ehdr->a_data,verbose,verifyonly) != 0) {
        return(TFSERR_MEMFAIL);
    }

    /* Clear out bss space: */
    if(verbose) {
        showSection("bss");
    }

    if(tfsld_memset((char *)(ehdr->a_entry+ehdr->a_text+ehdr->a_data),
                    0,ehdr->a_bss,verbose,verifyonly) != 0) {
        return(TFSERR_MEMFAIL);
    }


    if(verbose & !verifyonly) {
        showEntrypoint(ehdr->a_entry);
    }

    /* Store entry point: */
    if(entrypoint) {
        *entrypoint = (long)(ehdr->a_entry);
    }

    return(TFS_OKAY);
}
#endif

#if TFS_EBIN_COFF

/* tfsloadcoff():
 *  The file pointed to by fp is assumed to be a COFF file.
 *  This function loads the sections of that file into the designated
 *  locations.
 */
int
tfsloadcoff(TFILE *fp,int verbose,long *entrypoint,char *sname,int verifyonly)
{
    int     i, err;
    FILHDR  *fhdr;
    AOUTHDR *ahdr;
    SCNHDR  *shdr;

    if(tfsTrace) {
        printf("tfsloadcoff(%s)\n",TFS_NAME(fp));
    }

    /* Establish file header pointers... */
    fhdr = (FILHDR *)(tfsBase(fp));
    if((fhdr->f_opthdr == 0) || ((fhdr->f_flags & F_EXEC) == 0)) {
        return(TFSERR_BADHDR);
    }

    err = 0;
    ahdr = (AOUTHDR *)(fhdr+1);
    shdr = (SCNHDR *)((uchar *)ahdr + fhdr->f_opthdr);

    /* For each section header, relocate or clear if necessary... */
    for(i=0; !err && i<fhdr->f_nscns; i++) {
        if(shdr->s_size == 0) {
            shdr++;
            continue;
        }

        /* If incoming section name is specified, then we only load the
         * section with that name...
         */
        if((sname != 0) && (strcmp(sname,shdr->s_name) != 0)) {
            continue;
        }

        if(verbose) {
            showSection(shdr->s_name);
        }

        if(ISLOADABLE(shdr->s_flags)) {
            if(tfsld_memcpy((char *)(shdr->s_paddr),
                            (char *)(shdr->s_scnptr+(int)fhdr),
                            shdr->s_size,verbose,verifyonly) != 0) {
                err++;
            }
        } else if(ISBSS(shdr->s_flags)) {
            if(tfsld_memset((char *)(shdr->s_paddr),0,shdr->s_size,
                            verbose,verifyonly) != 0) {
                err++;
            }
        } else if(verbose) {
            printf("???\n");
        }
        shdr++;
    }

    if(verbose && !verifyonly && !sname) {
        showEntrypoint(ahdr->entry);
    }

    if(err) {
        return(TFSERR_MEMFAIL);
    }

    /* Store entry point: */
    if(entrypoint) {
        *entrypoint = (long)(ahdr->entry);
    }

    return(TFS_OKAY);
}
#endif

#if TFS_EBIN_ELF | TFS_EBIN_ELFMSBIN

/* tfsloadelf():
 *  The file pointed to by fp is assumed to be an ELF file.
 *  This function loads the sections of that file into the designated
 *  locations.
 */
int
tfsloadelf(TFILE *fp,int verbose,long *entrypoint,char *sname,int verifyonly)
{
    Elf32_Word  size, notproctot;
    int         i, j, err;
    char        *shname_strings, *name;
    Elf32_Addr  sh_addr;
    ELFFHDR     *ehdr;
    ELFSHDR     *shdr;
    ELFPHDR     *phdr;

    if(tfsTrace) {
        printf("tfsloadelf(%s)\n",TFS_NAME(fp));
    }

    /* Establish file header pointers... */
    /* If the first reserved entry isn't 0xffffffff, then assume this is a
     * 'fake' header, and it contains the base address of the file data...
     * See tfsld() for more info.
     */
    ehdr = (ELFFHDR *)(tfsBase(fp));
    shdr = (ELFSHDR *)((int)ehdr + ehdr->e_shoff);
    err = 0;

    /* Verify basic file sanity... */
    if((ehdr->e_ident[0] != 0x7f) || (ehdr->e_ident[1] != 'E') ||
            (ehdr->e_ident[2] != 'L') || (ehdr->e_ident[3] != 'F')) {
        return(TFSERR_BADHDR);
    }

    /* Store the section name string table base: */
    shname_strings = (char *)ehdr + shdr[ehdr->e_shstrndx].sh_offset;
    notproctot = 0;

    /* For each section header, relocate or clear if necessary... */
    for(i=0; !err && i<ehdr->e_shnum; i++,shdr++) {
        if((size = shdr->sh_size) == 0) {
            continue;
        }

        name = shname_strings + shdr->sh_name;

        /* If incoming section name is specified, then we only load the
         * section with that name...
         */
        if((sname != 0) && (strcmp(sname,name) != 0)) {
            continue;
        }

        if((verbose) && (ehdr->e_shstrndx != SHN_UNDEF)) {
            showSection(shname_strings + shdr->sh_name);
        }

        if(!(shdr->sh_flags & SHF_ALLOC)) {
            notproctot += size;
            if(verbose)
                printf("     %7ld bytes not processed (tot=%ld)\n",
                       size,notproctot);
            continue;
        }

        sh_addr = shdr->sh_addr;

        /* Look to the program header to see if the destination address
         * of this section needs to be adjusted.  If this section is
         * within a program header and that program header's members
         * p_vaddr & p_paddr are not equal, then adjust the section
         * address based on the delta between p_vaddr and p_paddr...
         */
        phdr = (ELFPHDR *)((int)ehdr + ehdr->e_phoff);
        for(j=0; j<ehdr->e_phnum; j++,phdr++) {
            if((phdr->p_type == PT_LOAD) &&
                    (sh_addr >= phdr->p_vaddr) &&
                    (sh_addr < phdr->p_vaddr+phdr->p_filesz) &&
                    (phdr->p_vaddr != phdr->p_paddr)) {
                sh_addr += (phdr->p_paddr - phdr->p_vaddr);
                break;
            }
        }

        if(shdr->sh_type == SHT_NOBITS) {
            if(tfsld_memset((uchar *)(sh_addr),0,size,
                            verbose,verifyonly) != 0) {
                err++;
            }
        } else {
            if(tfsld_memcpy((char *)(sh_addr),
                            (char *)((int)ehdr+shdr->sh_offset),
                            size,verbose,verifyonly) != 0) {
                err++;
            }
        }
    }

    if(err) {
        return(TFSERR_MEMFAIL);
    }

    if(verbose && !verifyonly && !sname) {
        showEntrypoint(ehdr->e_entry);
    }

    /* Store entry point: */
    if(entrypoint) {
        *entrypoint = (long)(ehdr->e_entry);
    }

    return(TFS_OKAY);
}

#endif

#if TFS_EBIN_ELFMSBIN | TFS_EBIN_MSBIN

/* tfsloadmsbin():
 * This loader is a bit different than the others.
 * The model for AOUT, COFF & ELF is simply to load each section from
 * the stored location in flash to the location specifed by the section
 * header.  The sections may be compressed.
 * The MSBIN loader supports a few additional options because of the
 * fact that WinCE files can be formatted as .bin or .nbo.  When
 * the file is the ".bin" type, then this loader is used similar to
 * the AOUT, COFF & ELF loaders. When the file is the ".nbo" type,
 * then it is simply loaded into the starting address of the target's
 * DRAM and the entrypoint is that base address.
 * To support this scheme, TFS uses both the 'c' flag (compressed)
 * and the extension on the filename as follows...
 *
 * 1 filename.bin with 'c' flag inactive:
 *   Load the file as specified by the sections within the file
 *   header.
 * 2 filename.bin with 'c' flag active:
 *   Load the file as specified by the sections within the file
 *   header and assume each section is compressed.
 * 3 filename.nbo with 'c' flag inactive:
 *   Copy the entire content of the file from TFS flash space to
 *   APPRAMBASE and return the address APPRAMBASE as the entrypoint.
 * 4 filename.nbo with 'c' flag active:
 *   Decompress the entire content of the file from TFS flash space
 *   to APPRAMBASE and return the address APPRAMBASE as the entrypoint.
 *
 *  In three of the 4 cases above (1,2&3) some level of sanity checking
 *  can be done on the file prior to beginning the load.  For the 4th
 *  case there is no sanity check, this function assumes the file is
 *  compressed and is destined for the base of DRAM.
 */
int
tfsloadmsbin(TFILE *fp,int verbose,long *entrypoint,char *sname,int verifyonly)
{
    ulong       offset;
    MSBINFHDR   filhdr;
    MSBINSHDR   scnhdr;
    char        *dot;
    int         err, snum;

    if(tfsTrace) {
        printf("tfsloadmsbin(%s)\n",TFS_NAME(fp));
    }

    dot = strrchr(TFS_NAME(fp),'.');
    if(!dot) {
        return(TFSERR_BADEXTENSION);
    }

    /* Check the filename extension for ".bin" or ".nbo" and process
     * accordingly...
     */
    if(strcmp(dot,".bin") == 0) {
        /* Verify basic file sanity... */
        if(memcmp((char *)tfsBase(fp),MSBIN_SYNC_DATA,MSBIN_SYNC_SIZE) != 0) {
            return(TFSERR_BADHDR);
        }

        /* The file header is at offset MSBIN_SYNC_SIZE in the
         * file, so copy it from the file to a local structure...
         */
        memcpy((char *)&filhdr,(tfsBase(fp) + MSBIN_SYNC_SIZE),
               sizeof(MSBINFHDR));

        /* Start by clearing the entire image space to zero...
         */
        if(verbose) {
            printf("MsbinImage: ");
        }

        tfsld_memset((uchar *)filhdr.imageaddr,0,(int)filhdr.imagelen,verbose,
                     verifyonly);

        err = snum = 0;
        offset = (ulong)(tfsBase(fp) + MSBIN_SYNC_SIZE + sizeof(MSBINFHDR));

        /* Walk through all sections within the file.  For each section,
         * copy the structure out of TFS to local space, then process it.
         */
        for(snum = 1; err==0; snum++) {
            memcpy((char *)&scnhdr,(char *)offset,sizeof(MSBINSHDR));
            offset += sizeof(MSBINSHDR);

            /* The image terminates with an image record header with the
             * physical address and checksum set to zero...
             */
            if((scnhdr.addr == 0) && (scnhdr.csum == 0)) {
                break;
            }

            if(verbose) {
                printf("section %02d: ", snum);
            }

            if(tfsld_memcpy((char *)(scnhdr.addr),
                            (char *)offset,scnhdr.len,verbose,verifyonly) != 0) {
                err++;
            }

            offset += scnhdr.len;
        }

        if(err) {
            return(TFSERR_MEMFAIL);
        }

        if(verbose && !verifyonly && !sname) {
            showEntrypoint(filhdr.imageaddr);
        }

        /* Store entry point: */
        if(entrypoint) {
            *entrypoint = (long)(filhdr.imageaddr);
        }
    } else if(strcmp(dot,".nb0") == 0) {
        char *drambase;

        drambase = (char *)getAppRamStart();

        tfsld_memcpy(drambase,(char *)tfsBase(fp),(int)TFS_SIZE(fp),
                     verbose,verifyonly);

        /* Store entry point: */
        if(entrypoint) {
            *entrypoint = (long)(drambase);
        }
    } else {
        return(TFSERR_BADEXTENSION);
    }

    return(TFS_OKAY);
}

#endif

int
tfsloadebin(TFILE *fp,int verbose,long *entrypoint,char *sname,int verifyonly)
{
#if TFS_EBIN_ELFMSBIN
    int err;
#endif

    /* If verbosity is greater than one and verifyonly is not set, then
     * we are simply dumping a map, so start with an appropriate
     * header...
     */
    if((verbose > 1) && (verifyonly == 0)) {
        printf("Load map:\n");
    }

#if TFS_EBIN_AOUT
    return(tfsloadaout(fp,verbose,entrypoint,sname,verifyonly));
#elif TFS_EBIN_COFF
    return(tfsloadcoff(fp,verbose,entrypoint,sname,verifyonly));
#elif TFS_EBIN_ELF
    return(tfsloadelf(fp,verbose,entrypoint,sname,verifyonly));
#elif TFS_EBIN_MSBIN
    return(tfsloadmsbin(fp,verbose,entrypoint,sname,verifyonly));
#elif TFS_EBIN_ELFMSBIN
    err = tfsloadelf(fp,verbose,entrypoint,sname,verifyonly);
    if(err == TFSERR_BADHDR) {
        return(tfsloadmsbin(fp,verbose,entrypoint,sname,verifyonly));
    } else {
        return(err);
    }
#else
#error Invalid TFS_EBIN type.
#endif
}


#endif  /* INCLUDE_TFS */
