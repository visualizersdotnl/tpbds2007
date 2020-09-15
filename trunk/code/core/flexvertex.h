
// tpbds -- flexible vertex

#ifndef _FLEXVERTEX_H_
#define _FLEXVERTEX_H_

enum FlexVertexElement 
{
	FV_POSITION_ID   = 0,
	FV_COLOR_ID      = 1,
	FV_UV_ID         = 2,
	FV_NORMAL_ID     = 3,
	FV_POINT_SIZE_ID = 4,
	FV_TANGENT_ID    = 5,
	FV_BINORMAL_ID   = 6,
	FV_NUM_ELEMENTS  = 7
};

// Use these flags to define a vertex.
enum FlexVertexFlags
{
	FV_POSITION   = 1 << FV_POSITION_ID,
	FV_COLOR      = 1 << FV_COLOR_ID,
	FV_UV         = 1 << FV_UV_ID,
	FV_NORMAL     = 1 << FV_NORMAL_ID,
	FV_POINT_SIZE = 1 << FV_POINT_SIZE_ID,
	FV_TANGENT    = 1 << FV_TANGENT_ID,
	FV_BINORMAL   = 1 << FV_BINORMAL_ID,
	FV_MASK       = FV_POSITION | FV_COLOR | FV_UV | FV_NORMAL | FV_POINT_SIZE | FV_TANGENT | FV_BINORMAL
};

class FlexVertex : public NoCopy
{
public:
	// To test if a vertex contains the required element(s).
	static bool FlagTest(uint32_t flags, uint32_t reqFlags)
	{
		return (reqFlags & flags) == reqFlags; 
	}

	FlexVertex(Renderer &renderer, uint32_t flags);
	
	~FlexVertex() 
	{
		SAFE_RELEASE(m_pVertexDecl);
	}	

	// For format compatibility tests.
	bool HasElements(uint32_t reqFlags) const { return FlagTest(m_flags, reqFlags); }

	uint32_t GetFlags() const { return m_flags; }
	size_t GetStride() const { return m_stride; }
	IDirect3DVertexDeclaration9 *GetVertexDecl() const { return m_pVertexDecl; }

	void SetStream(void *pStream) { m_pStream = static_cast<Byte *>(pStream); }	
	Byte *GetStream() const { return m_pStream; }

	// Maps a vertex element to the correct memory address.
	class Accessor
	{
		friend class FlexVertex;

	public:
		~Accessor() {}

		template<typename T> T &Elem(FlexVertexElement element)
		{
			TPB_ASSERT(element < FV_NUM_ELEMENTS && m_offsTab[element] != -1 && FlexVertex::s_elemSizes[element] == sizeof(T))
			return *reinterpret_cast<T *>(m_pVertex + m_offsTab[element]);
		}
		
		Vector3  &Position()  { return Elem<Vector3>(FV_POSITION_ID); }
		D3DCOLOR &Color()     { return Elem<D3DCOLOR>(FV_COLOR_ID); }
		Vector2  &UV()        { return Elem<Vector2>(FV_UV_ID); }
		Vector3  &Normal()    { return Elem<Vector3>(FV_NORMAL_ID); }
		float    &PointSize() { return Elem<float>(FV_POINT_SIZE_ID); }
		Vector3  &Tangent()   { return Elem<Vector3>(FV_TANGENT_ID); }
		Vector3  &Binormal()  { return Elem<Vector3>(FV_BINORMAL_ID); }
	
	private:
		Accessor(Byte *pVertex, const int *offsTab) :
			m_pVertex(pVertex),
			m_offsTab(offsTab) {}
		
		Byte *m_pVertex;
		const int *m_offsTab;
	};

	// Regular access mechanism.
	// Use the sequential mechanism below if possible.
	Accessor operator [](unsigned int iVertex) const
	{
		TPB_ASSERT(m_pStream != NULL);
		return Accessor(m_pStream + m_stride * iVertex, m_elemOffsets); // Contains multiplication; not efficient!
	}

	// Sequential access mechanism. Use with caution.
	// There still is *some* overhead but the multiplication is gone so a solid optimizer should do the trick.
	void Begin()       const { TPB_ASSERT(m_seqOffs == -1); m_seqOffs = 0; }
	Accessor Current() const { TPB_ASSERT(m_seqOffs != -1); return Accessor(m_pStream + m_seqOffs, m_elemOffsets); }
	void Next()        const { TPB_ASSERT(m_seqOffs != -1); m_seqOffs += m_stride; }
	void End()         const { TPB_ASSERT(m_seqOffs != -1); m_seqOffs = -1; }

	// Both mechanisms are usable on constant objects since the FlexVertex merely maps the stream.
	// This object is not responsible for the content of the mapped stream.

private:
	static const size_t s_elemSizes[FV_NUM_ELEMENTS];
	static const D3DVERTEXELEMENT9 s_elemToD3D[FV_NUM_ELEMENTS + 1];
	
	const uint32_t m_flags;
	size_t m_stride;
	int m_elemOffsets[FV_NUM_ELEMENTS];

	IDirect3DVertexDeclaration9 *m_pVertexDecl;

	// Stream properties.
	Byte *m_pStream;
	mutable size_t m_seqOffs;
};

#endif // _FLEXVERTEX_H_
