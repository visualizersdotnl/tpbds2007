
// tpbds -- Boost-style scoped array.

#ifndef _SCOPED_ARR_H_
#define _SCOPED_ARR_H_

template<typename T> class ScopedArr : public NoCopy
{
public:
	explicit ScopedArr(T *pT) :
		m_pT(pT) {}

	explicit ScopedArr(std::auto_ptr<T> pT) :
		m_pT(pT.release()) {}
	
	~ScopedArr() 
	{
		delete[] m_pT;
	}

	T *Get() const { return m_pT; }
	T *Extract() { T *pT = m_pT; m_pT = NULL; return pT; }

	T & operator [] (size_t index) const
	{
		TPB_ASSERT(m_pT);
		return m_pT[index];
	}

	operator bool () const { return m_pT != NULL; }
	bool operator ! () const { return m_pT == NULL; }

private:
	T  *m_pT;	
};

#endif // _SCOPED_ARR_H_
