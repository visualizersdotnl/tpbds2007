
// tpbds -- metaballs (original version: 2001, SSE2/3-optimized in 2009, requires SSE3!)

// to do:
// - review vertex cache indexing, it seems excessive :)
// - blob edge detection needs a rewrite (see below for a list of reasons why)
// - fix stack overflow proneness
// - move grid buffers (s_isoValues & s_gridCache) into the class?
// - double-check cache-friendliness (a quick VTune session yields no serious problems)

// addled with the following bugs (Xbox port)
// - edge detect logic
// - page faults on corners (number of cubes = grid res -2!)

// also optimized more shit :)

#include "main.h"
#include "blobs.h"
#include "Tables.h"

// for SSE intrinsics
// #include <xmmintrin.h>
// #include <intrin.h>
// ^^ Included by core (main.h).

const unsigned int kMaxGridDepth = 64;
const unsigned int kMaxTriangles = 65536 * 2; // Should be sufficient.
const size_t kVertexBufferByteSize = 1024 * 1024 / 2;
const unsigned int kMaxVertices  = kVertexBufferByteSize / sizeof(BlobVertex);

#define CACHE_ALIGN __declspec(align(16))

FIX_ME // Move the following two into the Blobs class?
static float    CACHE_ALIGN s_isoValues[kMaxGridDepth * kMaxGridDepth * kMaxGridDepth];
static uint32_t CACHE_ALIGN s_gridCache[kMaxGridDepth * kMaxGridDepth * kMaxGridDepth * 3];

#define PACK_GRID_XYZ(gX, gY, gZ) (gZ) << 16 | (gY) << 8 | gX

Blobs::Blobs(Renderer &renderer, unsigned int numBlob4s, unsigned int gridDepth, float spaceSize) :
	m_pBlob4s(static_cast<Blob4 *>(AlignedAlloc(numBlob4s * sizeof(Blob4), 16))),
	m_renderer(renderer),
	m_numBlob4s(numBlob4s),
	m_gridDepth(gridDepth),
	m_gridDepthSqr(gridDepth * gridDepth),
	m_gridSize(spaceSize),
	m_gridOffs(spaceSize * 0.5f),
	m_gridStep(spaceSize / (float) gridDepth),
	m_pCube(static_cast<float *>(AlignedAlloc(8 * sizeof(float), 16))),
	m_flexVtx(renderer, BlobVertex::GetFlexFlags()),
	m_pVB(NULL), m_pIB(NULL),
	m_geomGenerated(false)
{
	TPB_ASSERT(kMaxVertices <= 0xffffff); // I have 24 bits reserved as index cache.
	TPB_ASSERT(gridDepth <= kMaxGridDepth);

	// Make sure the native integer size isn't larger than 32-bits.
	TPB_ASSERT(sizeof(int) == 4); 

	m_cubeIndices[0] = 0; 
	m_cubeIndices[1] = 1; 
	m_cubeIndices[2] = 1 + m_gridDepthSqr;
	m_cubeIndices[3] = m_gridDepthSqr; 
	m_cubeIndices[4] = m_gridDepth; 
	m_cubeIndices[5] = 1 + m_gridDepth;
	m_cubeIndices[6] = 1 + m_gridDepth + m_gridDepthSqr; 
	m_cubeIndices[7] = m_gridDepth + m_gridDepthSqr;

	m_cubeOffsets[0] = Vector3(       0.f,        0.f,        0.f);
	m_cubeOffsets[1] = Vector3(m_gridStep,        0.f,        0.f);
	m_cubeOffsets[2] = Vector3(m_gridStep,        0.f, m_gridStep);
	m_cubeOffsets[3] = Vector3(       0.f,        0.f, m_gridStep);
	m_cubeOffsets[4] = Vector3(       0.f, m_gridStep,        0.f);
	m_cubeOffsets[5] = Vector3(m_gridStep, m_gridStep,        0.f);	
	m_cubeOffsets[6] = Vector3(m_gridStep, m_gridStep, m_gridStep);
	m_cubeOffsets[7] = Vector3(       0.f, m_gridStep, m_gridStep);
}

Blobs::~Blobs()
{
	AlignedFree(m_pBlob4s);
	AlignedFree(m_pCube);

	m_renderer.DestroyVertexBuffer(m_pVB);
	m_renderer.DestroyIndexBuffer(m_pIB);
}

bool Blobs::AllocateBuffers()
{
	m_pVB = m_renderer.CreateVertexBuffer(kVertexBufferByteSize, true);
	if (m_pVB == NULL)
	{
		return false;
	}
	
	m_pIB = m_renderer.CreateIndexBuffer(kMaxTriangles * 3, true, true);
	if (m_pIB == NULL)
	{
		return false;
	}

	return true;	
}

void Blobs::Generate(float isoThreshold)
{
	m_curThresh = isoThreshold;

	TPB_ASSERT(m_pVB != NULL && m_pIB != NULL);
	m_pVertices = static_cast<BlobVertex *>(m_pVB->Lock());
	m_pFaces = static_cast<BlobFace *>(m_pIB->Lock());

	m_genNumVerts = 0; m_genNumFaces = 0;

	// set all cache bytes to 0xff so I can easily perform checks
	memset(s_isoValues, 0xff, m_gridDepth * m_gridDepthSqr * sizeof(float));
	memset(s_gridCache, 0xff, 3 * m_gridDepth * m_gridDepthSqr * sizeof(int));

	for (unsigned int iBlob4 = 0; iBlob4 < m_numBlob4s; ++iBlob4)
	{
		for (unsigned int iBlob = 0; iBlob < 4; ++iBlob)
		{

			float fGridX = ( (m_gridOffs+m_pBlob4s[iBlob4].X[iBlob]) /m_gridSize)*m_gridDepth;
			float fGridY = ( (m_gridOffs+m_pBlob4s[iBlob4].Y[iBlob]) /m_gridSize)*m_gridDepth;
			float fGridZ = ( (m_gridOffs+m_pBlob4s[iBlob4].Z[iBlob]) /m_gridSize)*m_gridDepth;

			// for now, any ball with it's center outside the grid will cause a page fault: fix this!
			unsigned int gX, gY, gZ;
			gX = (unsigned int) (fGridX);
			gY = (unsigned int) (fGridY);
			gZ = (unsigned int) (fGridZ);
			unsigned int iGrid = gZ*m_gridDepthSqr + gY*m_gridDepth + gX;

			float sX = fGridX;
			sX = floorf(fGridX);
			sX /= m_gridDepth;
			sX *= m_gridSize;
			sX -= m_gridOffs;

			float sY = fGridY;
			sY = floorf(fGridY);
			sY /= m_gridDepth;
			sY *= m_gridSize;
			sY -= m_gridOffs;

			float sZ = fGridZ;
 			sZ = floorf(fGridZ);
			sZ /= m_gridDepth;
			sZ *= m_gridSize;
			sZ -= m_gridOffs;

			// This (old) code is flawed for a number of reasons:
			// 1. If the center of the blob is outside of a grid, a page fault occurs immediately.
			// 2. If a blob isn't fully within the grid, chances are it is not rendered, because
			//    the edge walked towards is only in 1 single direction (++gX).
			//
			// As long as all blobs are fully within the grid: no problem.

			// walk to the center of the blob
//			unsigned int gX = 0, gY = 0, gZ = 0;
//			float sX = -m_gridOffs, sY = -m_gridOffs, sZ = -m_gridOffs;
//			while (sX < m_pBlob4s[iBlob4].X[iBlob]) { sX += m_gridStep; ++gX; }
//			while (sY < m_pBlob4s[iBlob4].Y[iBlob]) { sY += m_gridStep; ++gY; }
//			while (sZ < m_pBlob4s[iBlob4].Z[iBlob]) { sZ += m_gridStep; ++gZ; }
//			unsigned int iGrid = gZ * m_gridDepthSqr + gY * m_gridDepth + gX;

			// and now walk towards the edge
			do
			{
				// must this grid cube still be processed?
				if ((s_gridCache[iGrid] & 0xff000000) != 0xff000000)
				{
					// if not, it is safe to assume that this blob's geometry has been generated
					break;
				}

				// obtain iso values
				for (unsigned int iPoint = 0; iPoint < 8; ++iPoint)
				{
					const unsigned int iCube = iGrid + m_cubeIndices[iPoint];
					if (*((int *) s_isoValues + iCube) != 0xffffffff)
					{
						m_pCube[iPoint] = s_isoValues[iCube];
					}
					else
					{
						const float cX = sX + m_cubeOffsets[iPoint].m_X;
						const float cY = sY + m_cubeOffsets[iPoint].m_Y;
						const float cZ = sZ + m_cubeOffsets[iPoint].m_Z;
						m_pCube[iPoint] = CalculateIsoValue(iCube, cX, cY, cZ);
					}
				}

				// hit the edge?
				if (GetEdgeTableIndex() != 0)
				{
					ProcessCube(iGrid, PACK_GRID_XYZ(gX, gY, gZ));
					break;
				}
			
				sX += m_gridStep; 
				++iGrid;
			}
			while (++gX < m_gridDepth);
		}
	}

	m_pVB->Unlock();
	m_pIB->Unlock();
	
	m_geomGenerated = true;
}

void Blobs::Draw()
{
	TPB_ASSERT(m_pVB != NULL && m_pIB != NULL && m_geomGenerated);
	m_renderer.SetVertexFormat(m_flexVtx);
	m_renderer.SetVertexBuffer(m_pVB, sizeof(BlobVertex));
	m_renderer.SetIndexBuffer(m_pIB);
	m_renderer.DrawIndexed(Renderer::PT_TRIANGLE_LIST, m_genNumFaces, m_genNumVerts);
}

float Blobs::CalculateIsoValue(unsigned int iGrid, float gridX, float gridY, float gridZ)
{
/*
	const __m128 gridXXXX = _mm_load1_ps(&gridX);
	const __m128 gridYYYY = _mm_load1_ps(&gridY);
	const __m128 gridZZZZ = _mm_load1_ps(&gridZ);

	__m128 isoValues = _mm_setzero_ps();
	for (unsigned int iBlob4 = 0; iBlob4 < m_numBlob4s; ++iBlob4)
	{
		// this does exactly the same as the ref. implementation below, but for 4 blobs in parallel
		// each register carries a single component (X, Y or Z) for each blob -- transformations are done in vertical fashion
		const __m128 xDeltas = _mm_sub_ps(gridXXXX, _mm_load_ps(m_pBlob4s[iBlob4].X));
		const __m128 yDeltas = _mm_sub_ps(gridYYYY, _mm_load_ps(m_pBlob4s[iBlob4].Y));
		const __m128 zDeltas = _mm_sub_ps(gridZZZZ, _mm_load_ps(m_pBlob4s[iBlob4].Z));
		const __m128 xDeltasSq = _mm_mul_ps(xDeltas, xDeltas);
		const __m128 yDeltasSq = _mm_mul_ps(yDeltas, yDeltas);
		const __m128 zDeltasSq = _mm_mul_ps(zDeltas, zDeltas);
		const __m128 oneOverRadSqr = _mm_rcp_ps(_mm_add_ps(_mm_add_ps(xDeltasSq, yDeltasSq), zDeltasSq));
		isoValues = _mm_add_ps(isoValues, oneOverRadSqr);
	}

	// "collapse" -- in 2 passes, add all 4 floats together and store the final result in the first one (SSE3)
	isoValues = _mm_hadd_ps(isoValues, isoValues);
	isoValues = _mm_hadd_ps(isoValues, isoValues);

	_mm_store_ss(s_isoValues + iGrid, isoValues);
	return s_isoValues[iGrid];			
*/
#if 1 // ref. implementation
	float isoValue = 0.f;
	for (unsigned int iBlob4 = 0; iBlob4 < m_numBlob4s; ++iBlob4)
	{
		for (unsigned int iBlob = 0; iBlob < 4; ++iBlob)
		{
			const float dX = gridX - m_pBlob4s[iBlob4].X[iBlob];
			const float dY = gridY - m_pBlob4s[iBlob4].Y[iBlob];
			const float dZ = gridZ - m_pBlob4s[iBlob4].Z[iBlob];
			
//			float bla=dX*dX+dY*dY+dZ*dZ;
//		if (bla == 0.f) __asm int 3
			
			isoValue += 1.f / (dX * dX + dY * dY + dZ * dZ);
		}
	}
	
	return s_isoValues[iGrid] = isoValue;
#endif
}

unsigned int Blobs::GetEdgeTableIndex()
{
	// flip a bit for each of the 8 corners that lie within the iso surface
	const __m128 cubePts1 = _mm_load_ps(m_pCube);
	const __m128 cubePts2 = _mm_load_ps(m_pCube + 4);
	const __m128 threshold = _mm_load1_ps(&m_curThresh);
	const __m128 cmpMask1 = _mm_cmplt_ps(cubePts1, threshold);
	const __m128 cmpMask2 = _mm_cmplt_ps(cubePts2, threshold);
	return _mm_movemask_ps(cmpMask1) | _mm_movemask_ps(cmpMask2) << 4;
}

void Blobs::ProcessCube(unsigned int iGrid, unsigned int iXYZ)
{
	// must this grid cube still be processed (i.e. upper 8 bits set)?
	if ((s_gridCache[iGrid] & 0xff000000) == 0xff000000)
	{
		// unpack gXYZ and calculate grid space values
		const unsigned int iX = iXYZ & 255;
		const unsigned int iY = iXYZ >> 8 & 255;
		const unsigned int iZ = iXYZ >> 16;
		const float gridX = -m_gridOffs + (float) iX * m_gridStep;
		const float gridY = -m_gridOffs + (float) iY * m_gridStep;
		const float gridZ = -m_gridOffs + (float) iZ * m_gridStep;

		// flag as processed (erase upper 8 bits)
		s_gridCache[iGrid] &= 0xffffff;

		// obtain iso values
		for (unsigned int iPoint = 0; iPoint < 8; ++iPoint)
		{
			const unsigned int iCube = iGrid + m_cubeIndices[iPoint];
			if (*reinterpret_cast<int *>(s_isoValues + iCube) != 0xffffffff)
			{
				// iso value in cache
				m_pCube[iPoint] = s_isoValues[iCube];
			}
			else
			{
				const float cubeX = gridX + m_cubeOffsets[iPoint].m_X;
				const float cubeY = gridY + m_cubeOffsets[iPoint].m_Y;
				const float cubeZ = gridZ + m_cubeOffsets[iPoint].m_Z;
				m_pCube[iPoint] = CalculateIsoValue(iCube, cubeX, cubeY, cubeZ);
			}
		}
		
		const unsigned int iEdgeTab = GetEdgeTableIndex();
		if (iEdgeTab != 0)
		{
			const unsigned int edgeBits = kEdgeTable[iEdgeTab];

			// triangulate this cube
			Triangulate(iGrid, gridX, gridY, gridZ, iEdgeTab, edgeBits);
			
			// now recurse to neighbouring cubes if needed
			// this is done in an order that is most cache-friendly
			
			if (edgeBits & 2 + 32 + 512 + 1024)
			{
				// right neighbour
				if (iX + 1 < m_gridDepth)
				{
					ProcessCube(iGrid + 1, PACK_GRID_XYZ(iX + 1, iY, iZ));
				}
			}
			
			if (edgeBits & 8 + 128 + 256 + 2048)
			{
				// left neighbour
				if (iX != 0)
				{
					ProcessCube(iGrid - 1, PACK_GRID_XYZ(iX - 1, iY, iZ));
				}
			}
			
			if (edgeBits & 16 + 32 + 64 + 128)
			{
				// top neighbour
				if (iY + 1 < m_gridDepth)
				{
					ProcessCube(iGrid + m_gridDepth, PACK_GRID_XYZ(iX, iY + 1, iZ));
				}
			}
			
			if (edgeBits & 1 + 2 + 4 + 8)
			{
				// bottom neighbour
				if (iY != 0)
				{
					ProcessCube(iGrid - m_gridDepth, PACK_GRID_XYZ(iX, iY - 1, iZ));
				}
			}
			
			if (edgeBits & 4 + 64 + 1024 + 2048)
			{
				// front neighbour
				if (iZ + 1 < m_gridDepth)
				{
					ProcessCube(iGrid + m_gridDepthSqr, PACK_GRID_XYZ(iX, iY, iZ + 1));
				}
			}
			
			if (edgeBits & 1 + 16 + 256 + 512)
			{
				// back neighbour
				if (iZ != 0)
				{
					ProcessCube(iGrid - m_gridDepthSqr, PACK_GRID_XYZ(iX, iY, iZ - 1));
				}
			}
		}
	}
}

void Blobs::Triangulate(unsigned int iGrid, float gridX, float gridY, float gridZ, unsigned int iEdgeTab, unsigned int edgeBits)
{
	unsigned int vtxIndices[12];
	
	for (int i = 0; i < 12; ++i) vtxIndices[i]=0xdeadf00d;

	for (unsigned int iEdge = 0; iEdge < 12; ++iEdge)
	{
		// need to generate a vertex?
		if (edgeBits & (1 << iEdge))
		{
			// have it in cache?
			const unsigned int iCache = 3 * (iGrid + m_cubeIndices[kCubeEdgeToVertexCache[iEdge][0]]) + kCubeEdgeToVertexCache[iEdge][1];
			unsigned int iVertex = s_gridCache[iCache] & 0xffffff;
			if (iVertex == 0xffffff) // if lower 24 bits are set, vertex is not yet cached
			{
				// calculate iso surface intersection point on edge A -> B
				const int iCubeEdgeA = kCubeEdgeIndices[iEdge][0];
				const int iCubeEdgeB = kCubeEdgeIndices[iEdge][1];
				const float aX = m_cubeOffsets[iCubeEdgeA].m_X + gridX;
				const float aY = m_cubeOffsets[iCubeEdgeA].m_Y + gridY;
				const float aZ = m_cubeOffsets[iCubeEdgeA].m_Z + gridZ;
				const float bX = m_cubeOffsets[iCubeEdgeB].m_X + gridX;
				const float bY = m_cubeOffsets[iCubeEdgeB].m_Y + gridY;
				const float bZ = m_cubeOffsets[iCubeEdgeB].m_Z + gridZ;
				const float isoA = m_pCube[iCubeEdgeA]; 
				const float isoB = m_pCube[iCubeEdgeB];
				const float distance = (m_curThresh - isoA) / (isoB - isoA);
				const float pX = lerpf(aX, bX, distance);
				const float pY = lerpf(aY, bY, distance);
				const float pZ = lerpf(aZ, bZ, distance);

				// calculate normal
				// works much like CalculateIsoValue() -- look at the ref. implementation for the algorithm
				
				const __m128 pXXXX = _mm_load1_ps(&pX);
				const __m128 pYYYY = _mm_load1_ps(&pY);
				const __m128 pZZZZ = _mm_load1_ps(&pZ);

				__m128 normalXXXX = _mm_setzero_ps(), normalYYYY = _mm_setzero_ps(), normalZZZZ = _mm_setzero_ps();
				for (unsigned int iBlob4 = 0; iBlob4 < m_numBlob4s; ++iBlob4)
				{
					const __m128 xDeltas = _mm_sub_ps(pXXXX, _mm_load_ps(m_pBlob4s[iBlob4].X));
					const __m128 yDeltas = _mm_sub_ps(pYYYY, _mm_load_ps(m_pBlob4s[iBlob4].Y));
					const __m128 zDeltas = _mm_sub_ps(pZZZZ, _mm_load_ps(m_pBlob4s[iBlob4].Z));
					const __m128 xDeltasSq = _mm_mul_ps(xDeltas, xDeltas);
					const __m128 yDeltasSq = _mm_mul_ps(yDeltas, yDeltas);
					const __m128 zDeltasSq = _mm_mul_ps(zDeltas, zDeltas);
					const __m128 radSqr = _mm_add_ps(_mm_add_ps(xDeltasSq, yDeltasSq), zDeltasSq);
					const __m128 oneOverRadSqr = _mm_rcp_ps(_mm_mul_ps(radSqr, radSqr));
					normalXXXX = _mm_add_ps(normalXXXX, _mm_mul_ps(oneOverRadSqr, xDeltas));
					normalYYYY = _mm_add_ps(normalYYYY, _mm_mul_ps(oneOverRadSqr, yDeltas));
					normalZZZZ = _mm_add_ps(normalZZZZ, _mm_mul_ps(oneOverRadSqr, zDeltas));
				}

				// collapse accumulated normal components into a single vector (SSE3)
				const __m128 nX12X34Y12Y34 = _mm_hadd_ps(normalXXXX, normalYYYY);    // | X1+X2 | X3+X4 | Y1+Y2 | Y3+Y4 |
				const __m128 nXYZ12Z34     = _mm_hadd_ps(nX12X34Y12Y34, normalZZZZ); // | X     | Y     | Z1+Z2 | Z3+Z4 |
				const __m128 nXYZZ         = _mm_shuffle_ps(nXYZ12Z34, _mm_hadd_ps(nXYZ12Z34, nXYZ12Z34), _MM_SHUFFLE(1, 1, 1, 0)); 
				// nXYZZ = X | Y | Z | Z

				// normalize X, Y and Z of nXYZZ (the not-so-elegant SSE way)
				const __m128 normSqr = _mm_mul_ps(nXYZZ, nXYZZ);
				__m128 oneOverNormLen = _mm_rsqrt_ss(_mm_add_ss(normSqr, _mm_add_ss(_mm_shuffle_ps(normSqr, normSqr, _MM_SHUFFLE(0, 0, 0, 1)), _mm_shuffle_ps(normSqr, normSqr, _MM_SHUFFLE(0, 0, 0, 2)))));
				oneOverNormLen = _mm_shuffle_ps(oneOverNormLen, oneOverNormLen, 0);
				const __m128 normal = _mm_mul_ps(nXYZZ, oneOverNormLen);

				// store vertex and normal
				// important: this is (most likely) an AGP write, so it must remain sequential!
				m_pVertices[m_genNumVerts].pX = pX;
				m_pVertices[m_genNumVerts].pY = pY;
				m_pVertices[m_genNumVerts].pZ = pZ;
				m_pVertices[m_genNumVerts].nX = normal.m128_f32[0];
				m_pVertices[m_genNumVerts].nY = normal.m128_f32[1];
				m_pVertices[m_genNumVerts].nZ = normal.m128_f32[2];

#if 0 // ref. implementation
				// calculate normal
				float nX = 0.f, nY = 0.f, nZ = 0.f;
				for (unsigned int iBlob4 = 0; iBlob4 < m_numBlob4s; ++iBlob4)
				{
					for (unsigned int iBlob = 0; iBlob < 4; ++iBlob)
					{
						const float dX = pX - m_pBlob4s[iBlob4].X[iBlob];
						const float dY = pY - m_pBlob4s[iBlob4].Y[iBlob];
						const float dZ = pZ - m_pBlob4s[iBlob4].Z[iBlob];
						const float radSqr = dX * dX + dY * dY + dZ * dZ;
						const float reciprocal = 1.f / (radSqr * radSqr);
						nX += dX * reciprocal;
						nY += dY * reciprocal;
						nZ += dZ * reciprocal;
					}
				}
				
				// store vertex
				m_pVertices[m_genNumVerts].pX = pX;
				m_pVertices[m_genNumVerts].pY = pY;
				m_pVertices[m_genNumVerts].pZ = pZ;

				// normalize and store normal
				const float oneOverNormalLen = 1.f / sqrtf(nX * nX + nY * nY + nZ * nZ);
				m_pVertices[m_genNumVerts].nX = nX * oneOverNormalLen;
				m_pVertices[m_genNumVerts].nY = nY * oneOverNormalLen;
				m_pVertices[m_genNumVerts].nZ = nZ * oneOverNormalLen;
#endif
				
				// store and cache index
				vtxIndices[iEdge] = iVertex = m_genNumVerts++;
				TPB_ASSERT(m_genNumVerts < kMaxVertices);
				s_gridCache[iCache] = (s_gridCache[iCache] & 0xff000000) | iVertex;
			}
			else
			{
				// this vertex has already been generated, so use it
				vtxIndices[iEdge] = iVertex;
			}
		}
	}
 
	for (int iIndex = 0; kTriangleTable[iEdgeTab][iIndex] != -1; iIndex += 3)
	{
		// build face -- reverse order for CW face
		// sequential read -> sequenti\ al write
		const int *pTriIndices = &kTriangleTable[iEdgeTab][iIndex];
		const unsigned int idxC = vtxIndices[*pTriIndices++];
		const unsigned int idxB = vtxIndices[*pTriIndices++];
		const unsigned int idxA = vtxIndices[*pTriIndices];

		if (idxA==0xdeadf00d) __asm int 3
		if (idxB==0xdeadf00d) __asm int 3
		if (idxC==0xdeadf00d) __asm int 3

		m_pFaces[m_genNumFaces].A = idxA;
		m_pFaces[m_genNumFaces].B = idxB;
		m_pFaces[m_genNumFaces].C = idxC;
		
		++m_genNumFaces;
		TPB_ASSERT(m_genNumFaces <= kMaxTriangles);
	}
}
