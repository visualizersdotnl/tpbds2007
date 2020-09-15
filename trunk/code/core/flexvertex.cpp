
// tpbds -- flexible vertex

#include "main.h"
#include "flexvertex.h"

const size_t FlexVertex::s_elemSizes[FV_NUM_ELEMENTS] =
{
	3 * sizeof(float), // POSITION
	sizeof(D3DCOLOR),  // COLOR
	2 * sizeof(float), // UV
	3 * sizeof(float), // NORMAL,
	sizeof(float),     // POINT_SIZE
	3 * sizeof(float), // TANGENT
	3 * sizeof(float)  // BINORMAL
};

const D3DVERTEXELEMENT9 FlexVertex::s_elemToD3D[FV_NUM_ELEMENTS + 1] = 
{
	{ 0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
	{ 0,  0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
	{ 0,  0, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
	{ 0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
	{ 0,  0, D3DDECLTYPE_FLOAT1,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE,    0 },
	{ 0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,  0 },
	{ 0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
	D3DDECL_END()
};

FlexVertex::FlexVertex(Renderer &renderer, uint32_t flags) :
	m_flags(flags & FV_MASK),
	m_stride(0),
	m_pVertexDecl(NULL),
	m_pStream(NULL),
	m_seqOffs(-1)
{
	// Number of bits available. Could switch to platform integer.
	TPB_ASSERT(FV_NUM_ELEMENTS <= sizeof(uint32_t) * 8);

	unsigned int numElems = 0; // Number of unique elements in this vertex.
	D3DVERTEXELEMENT9 d3dElems[FV_NUM_ELEMENTS + 1];

	for (unsigned int iElem = 0; iElem < FV_NUM_ELEMENTS; ++iElem)
	{
		if (flags & (1 << iElem))
		{
			d3dElems[numElems] = s_elemToD3D[iElem];
			d3dElems[numElems].Offset = m_stride;
			m_elemOffsets[iElem] = m_stride;
			m_stride += s_elemSizes[iElem];			
			++numElems;
		}
		else
		{
			m_elemOffsets[iElem] = -1;
		}
	}
	
	d3dElems[numElems] = s_elemToD3D[FV_NUM_ELEMENTS];
	TPB_VERIFY(renderer.GetAPI().CreateVertexDeclaration(d3dElems, &m_pVertexDecl) == D3D_OK);
}
