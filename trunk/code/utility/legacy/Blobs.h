
// tpbds -- metaballs (original version: 2001, SSE2/3-optimized in 2009, requires SSE3!)

#ifndef _LEGACY_BLOBS_H_
#define _LEGACY_BLOBS_H_

// include stock shaders for convenience
#include "../stockshaders/blobs.h"

// blobs go by 4 (limitation brought on by the SSE optimization)
struct Blob4
{
	float X[4];
	float Y[4];
	float Z[4];
};

FIX_ME // FlexVertex bypassed.
struct BlobVertex // Vertex maps on FlexVertex as constructed through GetFlexFlags().
{
	static uint32_t GetFlexFlags() { return FV_POSITION | FV_NORMAL; }

	float pX, pY, pZ; // FV_POSITION
	float nX, nY, nZ; // FV_NORMAL
}; 

struct BlobFace
{
	uint32_t A, B, C;
};

// This is very bare-bones -- it generates geometry and Draw() sets up the bare minimum calls.
// Arrange the shaders yourself.
class Blobs
{
public:
	Blobs(Renderer &renderer, unsigned int numBlob4s, unsigned int gridDepth, float spaceSize);
	~Blobs();

	bool AllocateBuffers();

	void Generate(float isoThreshold);
	void Draw();

	Blob4 *m_pBlob4s;

private:
	Renderer &m_renderer;

	const unsigned int m_numBlob4s;
	const unsigned int m_gridDepth, m_gridDepthSqr;
	const float m_gridSize, m_gridOffs, m_gridStep;

	float *m_pCube; // used as a temporary container (address aligned to 16 bytes for MOVAPS)
	unsigned int m_cubeIndices[8];
	Vector3 m_cubeOffsets[8];

	FlexVertex m_flexVtx;
	Renderer::VertexBuffer *m_pVB;
	Renderer::IndexBuffer *m_pIB;

	bool m_geomGenerated;

	// Temporaries used by Generate()/Draw().
	// After Generate(), m_genNumVerts and m_genNumFaces match the generated geometry.
	float m_curThresh;
	BlobVertex *m_pVertices;
	BlobFace *m_pFaces;
	unsigned int m_genNumVerts, m_genNumFaces;

	float CalculateIsoValue(unsigned int iGrid, float gridX, float gridY, float gridZ);
	unsigned int GetEdgeTableIndex();
	void ProcessCube(unsigned int iGrid, unsigned int iXYZ);
	void Triangulate(unsigned int iGrid, float gridX, float gridY, float gridZ, unsigned int iEdgeTab, unsigned int edgeBits);
};

#endif // _LEGACY_BLOBS_H_
