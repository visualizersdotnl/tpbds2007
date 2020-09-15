
// tpbds -- generic block allocator

#include "main.h" 
#include "Allocator.h" 

// Whatever you consider the maximum size of a fragment.
// Used by Allocator::RunCheck().
const size_t kMaxFragmentSize = 2048; // 2KB

Allocator::Allocator(size_t heapSize) :
	m_heapSize(heapSize),
	m_pHead(new Block(true, 0, heapSize)),
	m_pFree(m_pHead)
{
}

Allocator::~Allocator()
{
	Block *pBlock = m_pHead;
	while (pBlock != NULL)
	{
		Block *pNext = pBlock->m_pNext;
		delete pBlock;
		pBlock = pNext;
	}
}

size_t Allocator::Allocate(size_t numUnits, size_t alignTo)
{
	TPB_ASSERT(!(alignTo & 1)); // Must be a power of 2.

	size_t alignUnits;

	Block *pBlock = m_pFree;
	while (pBlock != NULL)
	{
		const size_t remainder = pBlock->m_offset & alignTo - 1;
		alignUnits = (!remainder) ? 0 : alignTo - remainder;
		if (pBlock->m_isFree && alignUnits + numUnits <= pBlock->m_size)
		{
			numUnits += alignUnits;
			break; // Fit!
		}

		pBlock = pBlock->m_pNext;
	}

	if (pBlock)
	{
		const size_t remainder = pBlock->m_size - numUnits;
		if (remainder)
		{
			// Insert new block to carry remainder.
			Block *pNewBlock = new Block(true, pBlock->m_offset + numUnits, remainder);
			pNewBlock->m_pPrev = pBlock;
			pNewBlock->m_pNext = pBlock->m_pNext;

			if (pBlock->m_pNext != NULL)
			{
				pBlock->m_pNext->m_pPrev = pNewBlock;
			}

			pBlock->m_pNext = pNewBlock;			
		}
		
		pBlock->m_isFree = false;
		pBlock->m_size -= remainder;

		// Shift and add remainder to previous block.
		if (alignUnits)
		{
			TPB_ASSERT(pBlock->m_pPrev != NULL);
			pBlock->m_pPrev->m_size += alignUnits;
			pBlock->m_offset += alignUnits;				
		}

		// Was this the first free block?
		if (pBlock == m_pFree)
		{
			if (pBlock->m_pNext != NULL && pBlock->m_pNext->m_isFree)
			{
				// The next one is free.
				m_pFree = pBlock->m_pNext;
			}
			else
			{
				// We need to find a new first free block further along.
				m_pFree = NULL;
				Block *pSearch = pBlock->m_pNext;
				while (pSearch != NULL)
				{
					if (pSearch->m_isFree)
					{
						m_pFree = pSearch;
						break;
					}
				}
			}
		}

		return pBlock->m_offset;
	}
	else
	{
		return ALLOCATOR_FAIL;
	}
}

void Allocator::Free(size_t offset)
{
	Block *pBlock = m_pHead;
	while (pBlock != NULL)
	{
		if (pBlock->m_offset == offset)
		{
			pBlock->m_isFree = true;
			
			// Attach to previous block?
			if (pBlock->m_pPrev != NULL && pBlock->m_pPrev->m_isFree)
			{
				Block *pPrevBlock = pBlock->m_pPrev;
				pPrevBlock->m_size += pBlock->m_size;   
				pPrevBlock->m_pNext = pBlock->m_pNext;
				
				if (pBlock->m_pNext != NULL)
				{
					pBlock->m_pNext->m_pPrev = pPrevBlock;
				}
				
				delete pBlock;
				pBlock = pPrevBlock;
			}
			
			// Usurp next block?
			if (pBlock->m_pNext != NULL && pBlock->m_pNext->m_isFree)
			{
				Block *pNextBlock = pBlock->m_pNext;   // I'm not fooled by the blocks that you've got -- I'm still, I'm still next on the block.  
				pBlock->m_size += pNextBlock->m_size;  // Used to have a little, now I have a lot!
				pBlock->m_pNext = pNextBlock->m_pNext; 
				
				if (pNextBlock->m_pNext != NULL)
				{
					pNextBlock->m_pNext->m_pPrev = pBlock;
				}
				
				delete pNextBlock;	
			}

			if (m_pFree != NULL)
			{
				if (m_pFree->m_offset > pBlock->m_offset)
				{
					// First free block.
					m_pFree = pBlock;
				}
			}
			else
			{
				// New free block.
				m_pFree = pBlock;
			}
			
			break;
		}
		
		pBlock = pBlock->m_pNext;
	}
	
	if (pBlock == NULL)
	{
		TPB_ASSERT(0); // Not found.
	}
}

void Allocator::Reset()
{
	m_pHead->DestructChildren();
	m_pHead = m_pFree = new Block(true, 0, m_heapSize);
}

bool Allocator::RunCheck()
{
	m_numBlocks = m_numBlocksFree = m_numFragments = m_fragmentedUnits = 0;

	Block *pBlock = m_pHead;
	while (pBlock != NULL)
	{
		++m_numBlocks;
		if (pBlock->m_isFree)
		{
			++m_numBlocksFree;
		}
	
		if (pBlock->m_pNext != NULL)
		{
			if (pBlock->m_pPrev != NULL)
			{
				if (pBlock->m_isFree && !pBlock->m_pNext->m_isFree && !pBlock->m_pPrev->m_isFree)
				{
					// This is an enclosed block.
					if (pBlock->m_size < kMaxFragmentSize)
					{
						// And a small one at that, so I guess this is a "fragment".
						++m_numFragments;
						m_fragmentedUnits += pBlock->m_size;
					}
				}
			}
			
			if (pBlock->m_pNext->m_pPrev != pBlock)
			{
				return false;
			}
		}

		pBlock = pBlock->m_pNext;				
	}
	
	return true;
}
