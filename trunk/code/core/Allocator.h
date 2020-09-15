
// tpbds -- generic block allocator

// This is a very basic first-fit block allocator, used by Renderer.
// It uses size_t for (64-bit) compatibility.

#if !defined(ALLOCATOR_H)
#define ALLOCATOR_H

#define ALLOCATOR_FAIL -1

class Allocator : public NoCopy
{
public:
	Allocator(size_t heapSize);
	~Allocator();

	size_t Allocate(size_t numUnits, size_t alignTo);
	void Free(size_t offset);

	void Reset();
	bool RunCheck();
	
private:
	class Block
	{
	public:
		bool m_isFree;
		size_t m_offset, m_size;
		Block *m_pPrev, *m_pNext;
	
		Block(bool isFree, size_t offset, size_t size) :
			m_isFree(isFree),
			m_offset(offset),
			m_size(size),
			m_pPrev(NULL),
			m_pNext(NULL) {}
		
		~Block() {}
		
		void DestructChildren()
		{
			if (m_pNext)
			{
				m_pNext->DestructChildren();
			}
			
			delete this;
		}
	};

	const size_t m_heapSize;
	Block *m_pHead, *m_pFree;

	// Statistics derived by RunCheck()
	unsigned int m_numBlocks;
	unsigned int m_numBlocksFree;
	unsigned int m_numFragments; // Free blocks enclosed by allocated blocks.
	unsigned int m_fragmentedUnits;
};

class AllocatorClient
{
public:
	AllocatorClient(Byte *pHeap, size_t heapSize) :
		m_baseAddr(pHeap),
		m_allocator(heapSize) {}
	
	~AllocatorClient() {}

	void *Allocate(size_t numBytes, size_t alignTo)
	{
		const size_t offset = m_allocator.Allocate(numBytes, alignTo);
		if (offset == ALLOCATOR_FAIL)
		{
			TPB_ASSERT(0);
			return 0;
		}
		else
		{
			return static_cast<void *>(m_baseAddr + offset);
		}
	}
	
	void Free(void *pAlloc)
	{
		Byte *byteAddr = reinterpret_cast<Byte *>(pAlloc);
		TPB_ASSERT(byteAddr != NULL && byteAddr >= m_baseAddr);
		const size_t offset = byteAddr - m_baseAddr;
		m_allocator.Free(offset);
	}

	Byte *GetHeap() const { return m_baseAddr; }
	Allocator &GetAllocator() { return m_allocator; }
		
private:
	Byte *m_baseAddr;
	Allocator m_allocator;
};
	
#endif // ALLOCATOR_H
