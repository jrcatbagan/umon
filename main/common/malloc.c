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
 * malloc():
 *
 * A simple memory allocator useful for embedded systems to provide 
 * some level of debug, plus the ability to increase heap space into
 * additional memory that is not contiguous with the intial statically
 * allocated array of memory.  The reason for supporting the ability to
 * allocate from 2 distinct blocks of memory is so that the monitor can
 * be built with some reasonable amount of heap space allocated to it; then
 * if the application is going to also use this malloc, it can do so
 * by simply extending the heap.  The monitor would not have to be
 * specifically built to support the large heap allocation.
 *	
 * The allocator data structures are part of the memory space used for
 * the allocation.  To test for memory overruns (using memory after the
 * end of the allocated space) or underruns (using memory prior to the
 * beginning of the allocated space), the data structure starts and ends
 * with a fixed tag that is always checked upon entry into malloc()
 * or free().
 * When the memory is freed, the next and previous block is checked to
 * determine if this newly freed block can be combined with a neighboring
 * block.  This provides some level of defragmentation.  Note, at
 * this point, that the blocks are only combined if they are found to be
 * contiguous.  This correctly implies that neighboring free blocks need
 * not be within contiguous memory space.
 * A function called GetMemory() must be provided as the underlying resource
 * of the memory used by the allocator.
 *
 * NOTE THAT THERE IS ABSOLUTELY NO CONCERN FOR SPEED IN THIS MEMORY
 * ALLOCATOR, IT IS SLOW!  Every call to malloc/free does a sanity check
 * on all allocation structures, so it is fairly good at detecting illegal
 * use of the allocated memory.
 *	
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#if INCLUDE_MALLOC
#include "genlib.h"
#include "stddefs.h"
#include "cli.h"

#define FNAMESIZE			32

#define PRETAG				0xDEAD
#define POSTTAG				0xBEEF
#define MHDRSIZE			sizeof(struct mhdr)

extern	char *GetMemory(int);
extern	int GetMemoryLeft(void);
extern	int extendHeap(char *,int);
extern	void unExtendHeap(void);
extern	char *getExtHeapBase(void);


/* mhdr:
   The control structure used by the memory allocator.
*/
struct	mhdr {
	ushort	pretag;			/* Fixed value used as an overrun sanity
							 * check for the previous memory block.
							 */
	int	size;				/* Size of useable memory block.  Size is
							 * positive if block is free and negative if
							 * block is not free.
							 */
	struct mhdr *next;		/* Points to next mhdr structure (not
							 * necessarily in contiguous memory space).
							 */
	struct mhdr *prev;		/* Points to previous mhdr structure (not
							 * necessarily in contiguous memory space).
							 */
	ushort	posttag;		/* Fixed value used as an underrun sanity
							 * check for this memory block.
							 */
#ifdef MALLOC_DEBUG
	char	fname[FNAMESIZE];
	int		fline;
#endif
};

/* mcalls, fcalls & mfails:
 *	Used to keep track of malloc and free calls.  Plus keep track
 *	of the number of times malloc is called, but it returns 0.
 */
static int mcalls, rcalls, fcalls, mfails;

/* mtot, ftot & highwater:
 *	Keep track of total amount of memory allocated.
 */
static int mtot, ftot, highwater;

/* mquiet:
 * If set (by heap -q), then the MALLOC ERROR messages are not
 * printed at runtime when an error is detected.
 */
static char mquiet;

/* mtrace:
 * If set (by heap -V), then each call to malloc or free will include
 * a printed message.
 */
static char mtrace;

/* heapbase:
 *	Initially zero, this pointer is set to the base of the heap on
 *	the first call to malloc().
 */
static struct mhdr	*heapbase;

static void
heapinit(void)
{
	mcalls = rcalls = fcalls = mfails = 0;
	mtot = ftot = highwater = 0;
	heapbase = (struct mhdr *)GetMemory(ALLOCSIZE);
	heapbase->pretag = PRETAG;
	heapbase->posttag = POSTTAG;
	heapbase->size = ALLOCSIZE - MHDRSIZE;
	if (heapbase->size < 0)
		printf("heapinit(): ALLOCSIZE too small!\n");
	heapbase->next = (struct mhdr *)0;
	heapbase->prev = (struct mhdr *)0;
}

/* heapcheck():
 *	Called with an mhdr pointer (or NULL).  This function just steps through
 *	the heap control structures to make sure there is some level of sanity.
 *	If the incoming mhdr pointer is non-zero, then it will also verify that
 *	the pointer is a valid control pointer in the heap.
 */
int
heapcheck(struct mhdr *mp,char *msg)
{
	int	i, mpok;
	register struct	mhdr *mptr;

	mpok = 0;
	if (!heapbase)
		heapinit();

	mptr = heapbase;
	for(i=0;mptr;i++,mptr=mptr->next) {
		if (mp == mptr)
			mpok = 1;
		if ((mptr->pretag != PRETAG) || (mptr->posttag != POSTTAG)) {
			if (!mquiet) {
				printf("\007MALLOC ERROR: heap corrupted at entry %d",i);
				if (msg)
					printf(" (%s)",msg);
				printf("\n");
			}
			return(-1);
		}
	}
	if ((mp) && (!mpok)) {
		if (!mquiet) {
			printf("\007MALLOC ERROR: 0x%lx (mem @ 0x%lx) invalid heap pointer",
		   	 (ulong)mp,(ulong)(mp+1));
			if (msg)
				printf(" (%s)",msg);
			printf("\n");
		}
		return(-1);
	}
	return(0);
}

static char *
_malloc(int size)
{
	register struct	mhdr *mptr, *mptr1;

	if (mtrace)
		printf("malloc(%d) = ",size);

	if (size <= 0) {
		if (mtrace)
			printf("0\n");
		return(0);
	}

	/* Start by checking sanity of heap. */
	if (heapcheck(0,0) < 0) {
		if (mtrace)
			printf("00\n");
		return((char *)0);
	}

	/* Keep track of number of calls to malloc for debug. */
	mcalls++;

	/* Make size divisible by 4: */
	if (size & 3) {
		size += 4;
		size &= 0xfffffffc;
	}

	mptr = (struct mhdr *)heapbase;
	while(1) {
		/* If request size is equal to the free block size or
		 * if the free block size is only slightly larger than the
		 * request size, then just use that free block as is.
		 * If the request size is at least "MHDRSIZE * 2"
		 * bytes smaller than free block size, then break
		 * that block up into 2 smaller chunks with one of the chunks
		 * being the size of the request and the size of the other chunk
		 * being whatever is left over.
		 */
		if (mptr->size >= size) {
			if (mptr->size <= (int)(size + (MHDRSIZE * 2))) {
				mtot += mptr->size;
				if ((mtot - ftot) > highwater)
					highwater = (mtot - ftot);
				mptr->size = -mptr->size;
			}
			else {
				mptr1 = (struct mhdr *)((char *)(mptr+1) + size);
				mptr1->pretag  = PRETAG;
				mptr1->posttag = POSTTAG;
				mptr1->next = mptr->next;
				mptr->next = mptr1;
				if (mptr1->next)
					mptr1->next->prev = mptr1;
				mptr1->prev = mptr;
				mptr1->size = (mptr->size - size) - MHDRSIZE;
				mptr->size = -size;
				mtot += size;
				if ((mtot - ftot) > highwater)
					highwater = (mtot - ftot);
			}
			if (mtrace)
				printf("0x%lx\n",(long)(mptr+1));
			return((char *)(mptr+1));
		}
		if (mptr->next == (struct mhdr *)0) {
			struct mhdr *moremem;
			int		getsize;

			getsize = size + MHDRSIZE;
			moremem = (struct mhdr *)GetMemory(getsize);
			if (!moremem) {
				mfails++;
				if (!mquiet)
					printf("\007MALLOC ERROR: no more memory\n");
				if (mtrace)
					printf("000\n");
				return((char *)0);
			}
			mptr->next = moremem;
			mptr->next->prev = mptr;
			mptr = mptr->next;
			mptr->next = (struct mhdr *)0;
			mptr->pretag = PRETAG;
			mptr->posttag = POSTTAG;
			mptr->size = getsize - MHDRSIZE;
		}
		else
			mptr = mptr->next;
	}
}

#ifdef MALLOC_DEBUG
#undef malloc

char *
malloc(int size, char *fname, int fline)
{
	char *cp;
	struct	mhdr	*mptr;

	cp = _malloc(size);
	if (cp) {
		mptr = (struct mhdr *)cp;
		mptr--;
		strncpy(mptr->fname,fname,FNAMESIZE-1);
		mptr->fline = fline;
		mptr->fname[FNAMESIZE-1] = 0;
		mptr->fline = fline;
	}
	return(cp);
}

#else
char *
malloc(int size)
{
	return(_malloc(size));
}
#endif

void
free(void *vp)
{
	char	*cp	 = vp;
	struct	mhdr	*mptr;

	if (mtrace)
		printf("free(0x%lx)\n",(long)cp);

	/* Keep track of number of calls to free for debug. */
	fcalls++;

	mptr = (struct mhdr *)cp - 1;

	/* Start by checking sanity of heap and make sure that the incoming
	 * pointer corresponds to a valid entry in the heap.
	 */
	if (heapcheck(mptr,0) < 0)
		return;

	/* The first thing to do to free the block is to make the size
	 * positive.
	 */
	mptr->size = abs(mptr->size);

	/* Keep track of how much memory is freed for debug. */
	ftot += mptr->size;

	/* To defragment the memory, see if previous and/or
	 * next block is free; if yes, then join them into one larger
	 * block. Note that the blocks will only be joined if they are in
	 * contiguous memory space.
	 */
	if (mptr->next) {
		if ((mptr->next->size > 0)  &&
		    (mptr->next == (struct mhdr *)
		    ((char *)mptr + mptr->size + MHDRSIZE))) {
			mptr->size += mptr->next->size + MHDRSIZE;
			mptr->next = mptr->next->next;
			if (mptr->next)
				mptr->next->prev = mptr;
		}
	}
	if (mptr->prev) {
		if ((mptr->prev->size > 0) &&
		    (mptr->prev == (struct mhdr *)
		    ((char *)mptr-mptr->prev->size - MHDRSIZE))) {
			if (mptr->next)
				mptr->next->prev = mptr->prev;
			mptr->prev->next = mptr->next;
			mptr->prev->size += mptr->size + MHDRSIZE;
		}
	}
}

/* calloc():
 *	Allocate space for an array of nelem elements of size elsize.
 *	Initialize the space to zero.
 */
static char *
_calloc(int nelem, int elsize)
{
	register char	*cp, *end;
	char	*base;
	int		size;

	size = nelem * elsize;
	base = _malloc(size);
	if (base) {
		cp = base;
		end = base+size;
		while(cp<end)
			*cp++ = 0;
	}
	return(base);
}

#ifdef MALLOC_DEBUG
char *
calloc(int nelem, int elsize, char *fname, int fline)
{
	char *cp;
	struct	mhdr	*mptr;

	cp = _calloc(nelem, elsize);
	if (cp) {
		mptr = (struct mhdr *)cp;
		mptr--;
		strncpy(mptr->fname,fname,FNAMESIZE-1);
		mptr->fline = fline;
		mptr->fname[FNAMESIZE-1] = 0;
		mptr->fline = fline;
	}
	return(cp);
}

#else

char *
calloc(int nelem, int elsize)
{
	return(_calloc(nelem,elsize));
}
#endif


static char *
_realloc(char *cp,int newsize)
{
	char			*new;
	int				asize, delta;
	struct	mhdr	*mptr, tmphdr;

	rcalls++;

	/* If incoming pointer is NULL, then do a regular malloc. */
	if (!cp)
		return(_malloc(newsize));

	/* If newsize is zero and pointer is not null, then do a free. */
	if (!newsize) {
		free(cp);
		return((char *)0);
	}

	/* Set the mhdr pointer to the header attached to the incoming 
	 * char pointer.  We assume here that the incoming pointer is the
	 * base address of the block of memory that is being reallocated.
	 */
	mptr = (struct mhdr *)cp - 1;

	/* Start by checking sanity of heap and make sure that the incoming
	 * pointer corresponds to a valid entry in the heap.
	 */
	if (heapcheck(mptr,0) < 0)
		return((char *)0);

	/* Recall that mptr->size is negative since the block is not free, so
	 * use the absolute value of mptr->size...
	 */
	asize = abs(mptr->size);

	/* Make requested size divisible by 4:
	 */
	if (newsize & 3) {
		newsize += 4;
		newsize &= 0xfffffffc;
	}

	/* If newsize is less than or equal to current size, just return with
	 * the same pointer.  At some point, this should be improved so that
	 * the memory made made available by this reallocation is put back in
	 * the pool.
	 */
	if (newsize <= asize) 
		return(cp);

	/* Now we do the actual reallocation...
	 * If there is a fragment after this one (next != NULL) AND it is
	 * available (size > 0) AND the combined size of the next fragment
	 * along with the current fragment exceeds the request, then we can
	 * reallocate quickly.
	 * Otherwise, we have to just malloc a whole new block and copy the
	 * old buffer to the new larger space.
	 */
	if ((mptr->next) && (mptr->next->size > 0) &&
		((asize + mptr->next->size + MHDRSIZE) > newsize)) {

		/* At this point we know we have the space to reallocate without
		 * the malloc/free step.  Now we need to add the necessary space
		 * to the current fragment, and take that much away from the next
		 * fragment...
		 */
		delta = newsize - asize;
		/* next line used to be: tmphdr = *mptr->next... */
		memcpy((char *)&tmphdr,(char *)mptr->next, sizeof(struct mhdr));
		mptr->size = -newsize;
		mptr->next = (struct mhdr *)(delta + (int)(mptr->next));
		mptr->next->size = (abs(tmphdr.size) - delta);
		mptr->next->pretag  = PRETAG;
		mptr->next->posttag = POSTTAG;
		mptr->next->next = tmphdr.next;
		mptr->next->prev = tmphdr.prev;

		/* Keep track of totals and highwater:
		 */
		mtot += (newsize - asize);
		if ((mtot - ftot) > highwater)
			highwater = (mtot - ftot);
		return(cp);
	}

	/* If the next fragment is not large enough, then malloc new space,
	 * copy the existing data to that block, free the old space and return
	 * a pointer to the new block.
	 */
	new = _malloc(newsize);
	if (!new)
		return((char *)0);

	memcpy(new,cp,asize);
	free(cp);
	return(new);
}

#ifdef MALLOC_DEBUG
#undef realloc
char *
realloc(char *buf, int newsize, char *fname, int fline)
{
	char *cp;
	struct	mhdr	*mptr;

	cp = _realloc(buf, newsize);
	if (cp) {
		mptr = (struct mhdr *)cp;
		mptr--;
		strncpy(mptr->fname,fname,FNAMESIZE-1);
		mptr->fline = fline;
		mptr->fname[FNAMESIZE-1] = 0;
		mptr->fline = fline;
	}
	return(cp);
}

#else

char *
realloc(char *buf, int newsize)
{
	return(_realloc(buf,newsize));
}
#endif


void
heapdump(int verbose)
{
	register struct	mhdr *mptr;
	char	freenow;
	int		i, alloctot, freetot, size;

	heapcheck(0,0);

	mptr = heapbase;
	i=0;
	freetot = 0;
	alloctot = 0;
	if (verbose)
		printf("        addr     size free?    mptr        nxt        prv      ascii@addr\n");
	else
		printf("Heap summary:\n");
	for(i=0;mptr;i++) {
		if (mptr->size < 0) {
			freenow = 'n';
			size = -mptr->size;
			alloctot += size;
		}
		else {
			freenow = 'y';
			size = mptr->size;
			freetot += size;
		}
		if (verbose) {
			printf("%3d: 0x%08lx %5d   %c   0x%08lx 0x%08lx 0x%08lx  ",
				i,(ulong)(mptr+1),size,freenow,(ulong)mptr,
				(ulong)(mptr->next),(ulong)(mptr->prev));
			prascii((unsigned char *)(mptr+1),size > 16 ? 16 : size);
			putchar('\n');
#ifdef MALLOC_DEBUG
			if (freenow == 'n')
				printf("     %s %d\n",mptr->fname,mptr->fline);
#endif
		}
		mptr = mptr->next;
	}
	if (verbose)
		putchar('\n');
	printf("  Malloc/realloc/free calls:  %d/%d/%d\n",mcalls,rcalls,fcalls);
	printf("  Malloc/free totals: %d/%d\n",mtot,ftot);
	printf("  High-water level:   %d\n",highwater);
	printf("  Malloc failures:    %d\n",mfails);
	printf("  Bytes overhead:     %d\n",i * MHDRSIZE);
	printf("  Bytes currently allocated:   %d\n",alloctot);
	printf("  Bytes free on current heap:  %d\n",freetot);
	printf("  Bytes left in allocation pool:  %d\n",GetMemoryLeft());
}

/* releaseExtendedHeap():
 *	If memory has been allocated through the extended heap established
 *	by the heap -x{start,size} command, this function will attempt
 *	to "undo" that.  It can only be un-done if there is no currently active
 *	allocations in that range.
 *
 *	This function is accessible by the application through monlib.
 */
int
releaseExtendedHeap(int verbose)
{
	int	i;
	struct	mhdr *mptr, *extbase;

	extbase = (struct mhdr *)getExtHeapBase();
	if (!extbase) {
		if (verbose)
			printf("Heap extension not set\n");
		return(-1);
	}
	
	heapcheck(0,0);
	mptr = heapbase;
	for(i=0;mptr;i++) {
		if (mptr->next == extbase) {
			if (mptr->next->next == (struct mhdr *)0) {
				mptr->next = (struct mhdr *)0;
				unExtendHeap();
				if (verbose)
					printf("Heap extension cleared\n");
				return(0);
			}
			else if (verbose)
				printf("Extended heap space is in use.\n");
			break;
		}
		mptr = mptr->next;
	}
	if (!mptr) {		/* Heap was extended, but not used. */
		unExtendHeap();	/* Remove the extension. */
		if (verbose)
			printf("Heap extension cleared.\n");
	}
	return(0);
}


char *HeapHelp[] = {
	"Display heap statistics.",
	"-[cf:m:qtvX:x]",
#if INCLUDE_VERBOSEHELP
	"Options:",
	" -c        clear high-water level and malloc/free totals",
	" -f{ptr}   free block @ 'ptr'",
	" -m{size}  malloc 'size' bytes",
	" -q        quiet runtime (don't print MALLOC ERROR msgs)",
	" -v        verbose (more detail)",
	" -t        toggle runtime malloc/free trace",
	" -X{base,size}",
	"           extend heap by 'size' bytes starting at 'base'",
	" -x        clear heap extension",
#endif
	0
};

int
Heap(int argc,char *argv[])
{
	char *establish_extended_heap, buf[32];
	int	verbose, release_extended_heap, showheap, opt;

	showheap = 1;
	establish_extended_heap = (char *)0;
	release_extended_heap = verbose = 0;
	while((opt=getopt(argc,argv,"cf:m:qtvX:x")) != -1) {
		switch(opt) {
		case 'c':
			mcalls = fcalls = 0;
			mtot = ftot = highwater = 0;
			showheap = 0;
			break;
		case 'f':
			free((char *)strtoul(optarg,0,0));
			showheap = 0;
			break;
		case 'm':
			shell_sprintf("MALLOC","0x%lx",
				(ulong)_malloc(strtoul(optarg,0,0)));
			if (verbose)
				printf("%s\n",buf);
			showheap = 0;
			break;
		case 'q':
			showheap = 0;
			mquiet = 1;
			break;
		case 't':
			mtrace = ~mtrace;
			printf("Runtime trace: %sabled\n",
				mtrace ? "en" : "dis");
			return(CMD_SUCCESS);
		case 'v':
			verbose = 1;
			break;
		case 'X':
			establish_extended_heap = optarg;
			showheap = 0;
			break;
		case 'x':
			release_extended_heap = 1;
			showheap = 0;
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}
	if (release_extended_heap)
		releaseExtendedHeap(verbose);

	if (establish_extended_heap) {
		int  size, rc;
		char *comma, *begin;

		comma = strchr(establish_extended_heap,',');
		if (!comma)
			return(CMD_PARAM_ERROR);
		*comma = 0;
		begin = (char *)strtoul(establish_extended_heap,0,0);
		size = (int)strtoul(comma+1,0,0);
		rc = extendHeap(begin,size);
		if (rc == -1) {
			printf("Extended heap already exists @ 0x%lx\n",
				(ulong)getExtHeapBase());
		}
		if (rc < 0)
			return(CMD_FAILURE);
	}

	if (!showheap)
		return(CMD_SUCCESS);

	if (optind == argc)
		heapdump(verbose);

	return(CMD_SUCCESS);
}
#else
char *
malloc(int size)
{
	return(0);
}

void
free(char *buf)
{
}

char *
realloc(char *buf, int newsize, char *fname, int fline)
{
	return(0);
}

#endif
