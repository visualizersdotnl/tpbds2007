
// tpbds -- Aligned memory allocation.

#ifndef _ALIGNED_ALLOC_H_
#define _ALIGNED_ALLOC_H_

void *AlignedAlloc(size_t numBytes, size_t alignTo);
void AlignedFree(void *);

#endif // _ALIGNED_ALLOC_H_
