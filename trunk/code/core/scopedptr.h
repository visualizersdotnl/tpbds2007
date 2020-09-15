
// tpbds -- Boost-style scoped pointer.

#ifndef _SCOPED_PTR_H_
#define _SCOPED_PTR_H_

template<typename T> class ScopedPtr : public NoCopy
{
public:
	explicit ScopedPtr(T *pT) :
		m_pT(pT) {}

	explicit ScopedPtr(std::auto_ptr<T> pT) :
		m_pT(pT.release()) {}
	
	~ScopedPtr() 
	{
		delete m_pT;
	}

	T *Get() const { return m_pT; }
	T *Extract() { T *pT = m_pT; m_pT = NULL; return pT; }
	
	T * operator -> () const
	{
		TPB_ASSERT(m_pT != NULL);
		return m_pT;
	}

	T & operator * () const
	{
		TPB_ASSERT(m_pT != NULL);
		return *m_pT;
	}

	operator bool () const { return m_pT != NULL; }
	bool operator ! () const { return m_pT == NULL; }

private:
	T  *m_pT;	
};

#endif // _SCOPED_PTR_H_
