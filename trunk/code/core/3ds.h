
// tpbds -- classic AutoDesk .3DS chunk

#ifndef _3DS_H_
#define _3DS_H_

class Chunk3DS
{
public:
	// Unique 16-bit chunk IDs.
	enum ID
	{
		ID_MAIN = 0x4d4d,
		  ID_EDIT = 0x3d3d,
		    ID_OBJECT = 0x4000,
		      ID_TRIANGLE_MESH = 0x4100,
		        ID_VERTEX_LIST = 0x4110,
		        ID_VERTEX_OPTIONS = 0x4111,
		        ID_UV_LIST = 0x4140,
		        ID_FACE_LIST = 0x4120
	};	

	#pragma pack(push, 1)

	struct Header
	{
		uint16_t ID;       // Maps to ID.
		uint32_t byteSize; // Size of chunk.
	};

	#pragma pack(pop)
	
	Chunk3DS(const Byte *pOffset) :
		m_pOffset(pOffset),
		m_cursor(sizeof(Header)) 
	{
		TPB_ASSERT(pOffset != NULL);
		m_header = *reinterpret_cast<const Chunk3DS::Header *>(pOffset);
	}

	~Chunk3DS() {}

	// bytesLeft - Number of bytes accessible relative to the chunk's address.
	bool ValidateSize(size_t bytesLeft) const
	{
		return m_header.byteSize >= sizeof(Header) && m_header.byteSize <= bytesLeft;
	}

	uint16_t ID() const { return m_header.ID; }
	size_t ChunkSize() const { return m_header.byteSize; }
	size_t DataSize() const { return ChunkSize() - sizeof(Header); }

	template<typename T> const T *GetDataPtr() const 
	{ 
		TPB_ASSERT(m_cursor < ChunkSize());
		return reinterpret_cast<const T *>(m_pOffset + m_cursor); 
	}

	template<typename T> const T Read() 
	{
		TPB_ASSERT(sizeof(T) <= BytesLeft());
		const T *pT = GetDataPtr<T>();
		m_cursor += sizeof(T);
		return *pT;
	}
	
	size_t GetCursor() const { return m_cursor; }
	size_t BytesLeft() const { return ChunkSize() - m_cursor; }
	bool EOC() const { return m_cursor >= ChunkSize(); }
	
private:
	Header m_header;
	const Byte *m_pOffset;
	
	size_t m_cursor;
};

#endif // _3DS_H_
