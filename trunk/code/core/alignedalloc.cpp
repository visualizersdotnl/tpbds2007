
// tpbds -- Aligned memory allocation.

// There are 2 versions: using a Microsoft CRT extension and my own reinvention of this wheel.

#include "main.h"

#if 0 // Using Microsoft CRT extension.

void *AlignedAlloc(size_t numBytes, size_t alignTo) { return _aligned_malloc(numBytes, alignTo); }
void AlignedFree(void *pMem) { _aligned_free(pMem); }

#else // Local implementation.

const uint32_t kMagicVal = 0xa110ca;

struct Header
{
	uint32_t magicVal;
	size_t offset;
};

void *AlignedAlloc(size_t numBytes, size_t alignTo)
{
	alignTo = std::max<size_t>(4, alignTo); // Min. 4 bytes.
	TPB_ASSERT(!(alignTo & 1)); // Must be a power of 2.

	const size_t bytesNeeded = alignTo + numBytes + sizeof(Header);
	void *pMem = malloc(bytesNeeded);
	if (pMem != NULL)
	{
		char *pStart = static_cast<char *>(pMem) + sizeof(Header);
		const size_t offset = alignTo - (reinterpret_cast<uintptr_t>(pStart) & alignTo - 1);
		pStart += offset;
		Header *pHeader = reinterpret_cast<Header *>(pStart - sizeof(Header));
		pHeader->magicVal = kMagicVal;
		pHeader->offset = offset + sizeof(Header);
		return pStart;
	}
	
	return NULL;
}

void AlignedFree(void *pMem)
{
	if (pMem != NULL)
	{
		char *pStart = static_cast<char *>(pMem);
		const Header *pHeader = reinterpret_cast<Header *>(pStart - sizeof(Header));
		TPB_ASSERT(pHeader->magicVal == kMagicVal);
		free(pStart - pHeader->offset);
	}
}

#endif
