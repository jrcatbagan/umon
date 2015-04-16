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
 * nand.c
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#if INCLUDE_NANDCMD
#include "cpuio.h"
#include "genlib.h"
#include "stddefs.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "cli.h"
#include "nand.h"

int nandVerbose;

#define END_OF_NAND (BASE_OF_NAND+SIZE_OF_NAND-1)

#ifdef FLASHRAM_BASE
#ifndef NAND_TFSRAM_BASE
#define NAND_TFSRAM_BASE FLASHRAM_BASE
#endif
#endif

char *nandHelp[] = {
	"Interface with Nand-Flash",
	"-[v] {cmd} [cmd-specific args]",
#if INCLUDE_VERBOSEHELP
	"Options:",
	" -v   incrementing verbosity level",
	"",
	"Cmds:",
	" init                      initialize interface",
	" info                      show device info",
#ifdef FLASHRAM_BASE
	" tfsls                     list files directly out of SPI flash",
	" tfsload                   copy NAND to TFSRAM",
	" tfsstat                   show state of SPI flash",
	" tfsstore                  copy TFSRAM to NAND",
	" tfserase                  erase NAND space allocated to TFS",
	" tfsrm {name}              remove file directly out of NAND flash",
	" tfsadd {name} [src sz]    add file directly to NAND flash",
#endif
	" erase {addr len}          erase block",
	" read {addr dest len}      read block",
	" write {addr src len}      write block",
#endif
	0,
};

int
nandCmd(int argc,char *argv[])
{
	unsigned long addr;
	char *cmd, *dest, *src;
	int opt, len, rc;

	rc = 0;
	nandVerbose = 0;
	while((opt=getopt(argc,argv,"v")) != -1) {
		switch(opt) {
		case 'v':
			nandVerbose++;
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}
	
	if (argc < optind+1)
		return(CMD_PARAM_ERROR);

	cmd = argv[optind];

	if (nandVerbose)
		printf("CMD: %s\n",cmd);

	if (strcmp(cmd,"init") == 0) {
		nandInit();
	}
	else if (strcmp(cmd,"info") == 0) {
		nandInfo();
	}
	else if (strcmp(cmd,"erase") == 0) {
		if (argc != optind+3)
			return(CMD_PARAM_ERROR);
		addr = strtoul(argv[optind+1],0,0);
		len = (int)strtol(argv[optind+2],0,0);
		nandEraseChunk((char *)addr,len);
	}
	else if (strcmp(cmd,"write") == 0) {
		if (argc != optind+4)
			return(CMD_PARAM_ERROR);
		addr = strtoul(argv[optind+1],0,0);
		src = (char *)strtoul(argv[optind+2],0,0);
		len = (int)strtol(argv[optind+3],0,0);
		nandWriteChunk((char *)addr,src,len);
	}
	else if (strcmp(cmd,"read") == 0) {
		if (argc != optind+4)
			return(CMD_PARAM_ERROR);
		addr = strtoul(argv[optind+1],0,0);
		dest = (char *)strtoul(argv[optind+2],0,0);
		len = (int)strtol(argv[optind+3],0,0);
		nandReadChunk((char *)addr,dest,len);
	}
#ifdef FLASHRAM_BASE
	else if (strcmp(cmd,"tfsload") == 0) {
	}
	else if (strcmp(cmd,"tfsstore") == 0) {
	}
	else if (strcmp(cmd,"tfserase") == 0) {
	}
	else if (strcmp(cmd, "tfsls") == 0) {
		int ftot;
		char *addr;
		TFILE tfshdr, *fp;

		ftot = 0;
		fp = &tfshdr;
		addr = (char *)BASE_OF_NAND;
		while(addr < (char *)END_OF_NAND) {
			char fbuf[32], *flags;

			if ((rc = nandReadChunk(addr,(char *)fp,TFSHDRSIZ)) < 0) {
				printf("nandReadChunk failed %d\n",rc);
				break;
			}
			if (fp->hdrsize == 0xffff)
				break;
			if (TFS_FILEEXISTS(fp)) {
				if (ftot == 0)
					printf(" Name                        Size    Offset    Flags  Info\n");
				ftot++;
				flags = tfsflagsbtoa(TFS_FLAGS(fp),fbuf);
				if ((!flags) || (!fbuf[0]))
					flags = " ";
				printf(" %-23s  %7ld  0x%08lx  %-5s  %s\n",TFS_NAME(fp),
					TFS_SIZE(fp),(unsigned long)(addr+TFSHDRSIZ),
					flags,TFS_INFO(fp));
			}
			addr += TFS_SIZE(fp);
			addr += TFSHDRSIZ;
			while((long)addr & 0xf) addr++;
		}
	}
	else if (strcmp(cmd, "tfsrm") == 0) {
		char *addr;
		TFILE tfshdr, *fp;
		char *arg2 = argv[optind+1];

		fp = &tfshdr;
		addr = (char *)BASE_OF_NAND;
		while(addr < (char *)END_OF_NAND) {
			if ((rc = nandReadChunk(addr,(char *)fp,TFSHDRSIZ)) < 0) {
				printf("nandReadChunk failed %d\n",rc);
				break;
			}
			if (fp->hdrsize == 0xffff) {
				printf("%s not found\n",arg2);
				break;
			}
			if (strcmp(TFS_NAME(fp),arg2) == 0) {
				if (TFS_FILEEXISTS(fp)) {
					fp->flags &= ~TFS_ACTIVE;
					if ((rc = nandWriteChunk(addr,(char *)fp,TFSHDRSIZ)) < 0)
						printf(" write_hdr failed %d\n",rc);
					break;
				}
			}
			addr += TFS_SIZE(fp);
			addr += TFSHDRSIZ;
			while((long)addr & 0xf) addr++;
		}
	}
	else if (strcmp(cmd, "tfsadd") == 0) {
		int size;
		long	bflags;
		TFILE tfshdr, *fp;
		char *addr;
		char *src, *name, *info;
		char *arg2 = argv[optind+1];
		char *arg3 = argv[optind+2];
		char *arg4 = argv[optind+3];
		char *icomma, *fcomma;

		info = "";
		bflags = 0;
		name = arg2;
		addr = (char *)BASE_OF_NAND;

		/* The incoming arguments can be either just the filename (in which
		 * case we assume the source is the file in TFS with the same name),
		 * or the filename, source address and size...
		 */
		if (argc == optind+2) {			// Just filename?
			if ((fp = tfsstat(name)) == (TFILE *)0) {
				printf("File '%s' not in TFS\n",name);
				return(CMD_FAILURE);
			}
			name = fp->name;
			info = fp->info;
			bflags = fp->flags;
			size = fp->filsize;
			src = (char *)(fp + 1);
			fp = &tfshdr;
			memset((char *)fp,0,TFSHDRSIZ);
		}
		else if (argc == optind+4) {	// Filename with addr and len
			// Extract flags and info fields (if any) from the name...
			fcomma = strchr(name,',');
			if (fcomma) {
				icomma = strchr(fcomma+1,',');
				if (icomma) {
					*icomma = 0;
					info = icomma+1;
				}
				*fcomma = 0;
				bflags = tfsctrl(TFS_FATOB,(long)(fcomma+1),0);
			}
	
			fp = &tfshdr;
			memset((char *)fp,0,TFSHDRSIZ);
			size = (int)strtol(arg4,0,0);
			src = (char *)strtoul(arg3,0,0);
		}
		else
			return(CMD_PARAM_ERROR);

		while(addr < (char *)END_OF_NAND) {
			if ((rc = nandReadChunk(addr,(char *)fp,TFSHDRSIZ)) < 0)
				break;
			if (fp->hdrsize == 0xffff) {
				unsigned long nextfileaddr;

				/* We're at the address in NAND where we can add the new
				 * file, but first we need to make sure there's enough
				 * room...
				 */
				if ((TFSHDRSIZ + size + 16) >= ((char *)END_OF_NAND - addr)) {
					printf(" not enough space\n");
					return(CMD_FAILURE);
				}

				/* Copy name and info data to header.
				 */
				strcpy(fp->name, name);
				strcpy(fp->info, info);
				fp->hdrsize = TFSHDRSIZ;
				fp->hdrvrsn = TFSHDRVERSION;
				fp->filsize = size;
				fp->flags = bflags;
				fp->flags |= (TFS_ACTIVE | TFS_NSTALE);
				fp->filcrc = crc32((unsigned char *)src,size);
				fp->modtime = tfsGetLtime();
#if TFS_RESERVED
				{
				int	rsvd;
				for(rsvd=0;rsvd<TFS_RESERVED;rsvd++)
					fp->rsvd[rsvd] = 0xffffffff;
				}
#endif
				fp->next = 0;
				fp->hdrcrc = 0;
				fp->hdrcrc = crc32((unsigned char *)fp,TFSHDRSIZ);
				nextfileaddr = NAND_TFSRAM_BASE - NAND_TFS_BASE + (long)addr + TFSHDRSIZ + size;
				if (nextfileaddr & 0xf)
					nextfileaddr = (nextfileaddr | 0xf) + 1;
				
				fp->next = (TFILE *)nextfileaddr;

				printf(" writing %s...\n",arg2);
				if ((rc = nandWriteChunk(addr,(char *)fp,TFSHDRSIZ)) < 0)
					printf(" write_hdr failed %d\n",rc);

				if ((rc = nandWriteChunk(addr+TFSHDRSIZ,src,size)) < 0)
					printf(" write_file failed %d\n",rc);
				break;
			}
			if (strcmp(TFS_NAME(fp),arg2) == 0) {
				if (TFS_FILEEXISTS(fp)) {
					printf(" removing %s...\n",arg2);
					fp->flags &= ~TFS_ACTIVE;
					if ((rc = nandWriteChunk(addr,(char *)fp,TFSHDRSIZ)) < 0)
						printf(" write_hdr failed %d\n",rc);
				}
			}
			addr += TFS_SIZE(fp);
			addr += TFSHDRSIZ;
			while((long)addr & 0xf) addr++;
		}
	}
	else if (strcmp(cmd, "tfsstat") == 0) {
		char *addr, *oaddr;
		TFILE tfshdr, *fp;
		unsigned long meminuse, memdead;

		fp = &tfshdr;
		meminuse = memdead = 0;
		addr = (char *)BASE_OF_NAND;
		while(addr < (char *)END_OF_NAND) {
			if ((rc = nandReadChunk(addr,(char *)fp,TFSHDRSIZ)) < 0) {
				printf("nandReadChunk failed %d\n",rc);
				break;
			}
			if (fp->hdrsize == 0xffff)
				break;

			oaddr = addr;
			addr += TFS_SIZE(fp);
			addr += TFSHDRSIZ;
			while((long)addr & 0xf) addr++;

			if (TFS_FILEEXISTS(fp))
				meminuse += addr - oaddr;
			else
				memdead += addr - oaddr;
		}
		printf("Total: 0x%x, used: 0x%x, dead: 0x%x, avail: 0x%x\n",
			SIZE_OF_NAND, meminuse, memdead,
			SIZE_OF_NAND - (meminuse + memdead));
	}
#endif
	else
		return(CMD_PARAM_ERROR);

	return(CMD_SUCCESS);
}
#endif
