
// tpbds -- Opaque data container.

// Ownership is transferred on assignment or copy construction.

#ifndef _OPAQUE_DATA_H_
#define _OPAQUE_DATA_H_

class OpaqueData
{
public:
	OpaqueData() :
		m_ID("NULL"),
		m_pData(NULL),
		m_size(0),
		m_isOwner(true) {}

	// Copy constructor needs to use assignment operator.
	OpaqueData(OpaqueData &opaqueData) { *this = opaqueData; }

	OpaqueData(const std::string &ID, const Byte *pData, size_t size, bool isOwner = true) :
		m_ID(ID),
		m_pData(pData),
		m_size(size),
		m_isOwner(isOwner) {}

	~OpaqueData() 
	{
		if (m_isOwner)
		{
			delete[] m_pData;
		}
	}

	bool IsValid() const { return m_pData != NULL; }
	bool IsOwner() const { return m_isOwner; }
	
	const std::string &GetID() const { return m_ID; }
	const Byte *GetPtr() const { return m_pData; }
	size_t GetSize() const { return m_size; }

	// Can only copy non-constant OpaqueData instances.
	OpaqueData & operator = (OpaqueData &RHS)
	{
		m_ID = RHS.m_ID;
		m_pData = RHS.m_pData;
		m_size = RHS.m_size;

		if (RHS.m_isOwner)
		{
			// Claim ownership.
			RHS.m_isOwner = false;
			m_isOwner = true;
		}
		else 
		{
			m_isOwner = false;
		}
		
		return *this; 
	}
	
private:
	std::string m_ID;
	const Byte *m_pData;
	size_t m_size;
	bool m_isOwner;
};

#endif // _OPAQUE_DATA_H_
