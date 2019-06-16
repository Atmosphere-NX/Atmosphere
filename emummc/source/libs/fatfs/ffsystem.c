/*------------------------------------------------------------------------*/
/* Sample Code of OS Dependent Functions for FatFs                        */
/* (C) ChaN, 2018                                                          */
/* (C) CTCaer, 2018                                                       */
/*------------------------------------------------------------------------*/


#include "ff.h"
#include "../../utils/types.h"
#include <stdlib.h>


#if FF_USE_LFN == 3	/* Dynamic memory allocation */

/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/

void* ff_memalloc (	/* Returns pointer to the allocated memory block (null if not enough core) */
	UINT msize		/* Number of bytes to allocate */
)
{
	return malloc(msize);	/* Allocate a new memory block with POSIX API */
}


/*------------------------------------------------------------------------*/
/* Free a memory block                                                    */
/*------------------------------------------------------------------------*/

void ff_memfree (
	void* mblock	/* Pointer to the memory block to free (nothing to do if null) */
)
{
	free(mblock);	/* Free the memory block with POSIX API */
}

#endif

