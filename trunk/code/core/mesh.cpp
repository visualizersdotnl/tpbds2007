
// tpbds -- static triangle mesh (CW-order) class with basic 3DS support

#include "main.h"
#include "3ds.h"

Mesh::Mesh(Renderer &renderer, uint32_t flexVertexBits, bool localOnly) :
	m_renderer(renderer),
	m_flexVtx(renderer, flexVertexBits),
	m_localOnly(localOnly),
	m_numVertices(0),
	m_numPrimitives(0),
	m_pVertices(NULL),
	m_pIndices(NULL),
	m_pVB(NULL),
	m_pIB(NULL)
{
	TPB_ASSERT(m_flexVtx.HasElements(FV_POSITION));
}

Mesh::~Mesh()
{
	Evict();
}

void Mesh::Evict()
{
	EvictLocalMesh();

	m_renderer.DestroyVertexBuffer(m_pVB);
	m_renderer.DestroyIndexBuffer(m_pIB);
	m_pVB = NULL;
	m_pIB = NULL;

	m_numVertices = 0;
	m_numPrimitives = 0;
}

void Mesh::EvictLocalMesh()
{
	delete[] m_pVertices;
	m_pVertices = NULL;
	m_flexVtx.SetStream(NULL);

	delete[] m_pIndices;
	m_pIndices = NULL;
}

bool Mesh::LoadFrom3DS(const std::string &path, float scaleTo, unsigned int objIdx /* = 0 */)
{
	const OpaqueData fileData = LoadFile(path);
	if (fileData.IsValid())
	{
		return LoadFrom3DSInMemory(fileData, scaleTo, objIdx);
	}
	
	return false;
}

bool Mesh::LoadFrom3DSInMemory(const OpaqueData &fileData, float scaleTo, unsigned int objIdx /* = 0 */)
{
	TPB_ASSERT(!LocalMeshValid());
	TPB_ASSERT(fileData.IsValid());

	// -- parse --

	const Byte *pData = fileData.GetPtr();
	const std::string errPrefix = "Can't load 3DS (" + fileData.GetID() + "): ";

	// filled in by the parser
	uint16_t numVertices = 0, numFaces = 0;
	const Vector3 *pVertices; 
	const Vector2 *pUVs = NULL;
	const uint16_t *pIndices;

	unsigned int cursor = 0, curObjIdx = 0;
	while (cursor < fileData.GetSize())
	{
		const size_t bytesLeft = fileData.GetSize() - cursor;
		if (bytesLeft >= sizeof(Chunk3DS::Header))
		{
			Chunk3DS chunk(pData + cursor);
			if (!chunk.ValidateSize(bytesLeft))
			{
				SetLastError(errPrefix + "invalid chunk size (ID: " + ToString(chunk.ID()) + ").");
				return false;
			}

			// process it
			switch (chunk.ID())
			{
			case Chunk3DS::ID_MAIN:
			case Chunk3DS::ID_EDIT:
			case Chunk3DS::ID_TRIANGLE_MESH:
				cursor += chunk.GetCursor();
				break;
							
			case Chunk3DS::ID_OBJECT:
				if (objIdx == curObjIdx++)
				{
					// parse the ID and child chunks					
					while (chunk.Read<Byte>() && !chunk.EOC()) {}
					cursor += chunk.GetCursor();
				}
				else
				{
					// not interested -- fully skip this chunk
					cursor += chunk.ChunkSize();
				}
				
				break;

			case Chunk3DS::ID_VERTEX_LIST:
				if (chunk.DataSize() >= sizeof(int16_t))
				{
					const uint16_t numVerticesTemp = chunk.Read<uint16_t>();
					TPB_ASSERT(!numVertices || numVerticesTemp == numVertices); // mismatch?
					numVertices = numVerticesTemp;
					const size_t vertexDataSize = numVertices * 3 * sizeof(float);
					if (chunk.BytesLeft() >= vertexDataSize)
					{
						pVertices = chunk.GetDataPtr<Vector3>();
						cursor += chunk.ChunkSize();
						break;
					}
				}
				
				SetLastError(errPrefix + "triangle mesh (object " + ToString(objIdx) + ") has an invalid vertex list.");
				return false;

			case Chunk3DS::ID_UV_LIST:
				if (chunk.DataSize() >= sizeof(int16_t))
				{
					const uint16_t numVerticesTemp = chunk.Read<uint16_t>();
					TPB_ASSERT(!numVertices || numVerticesTemp == numVertices); // mismatch?
					numVertices = numVerticesTemp;
					const size_t vertexDataSize = numVertices * 2 * sizeof(float);
					if (chunk.BytesLeft() >= vertexDataSize)
					{
						pUVs = chunk.GetDataPtr<Vector2>();
						cursor += chunk.ChunkSize();
						break;
					}
				}

				SetLastError(errPrefix + "triangle mesh (object " + ToString(objIdx) + ") has an invalid UV list.");
				return false;
			
			case Chunk3DS::ID_FACE_LIST:
				if (chunk.DataSize() >= sizeof(int16_t))
				{
					numFaces = chunk.Read<uint16_t>();
					const size_t faceDataSize = numFaces * 4 * sizeof(uint16_t);
					if (chunk.BytesLeft() >= faceDataSize)
					{
						pIndices = chunk.GetDataPtr<uint16_t>();
						cursor += chunk.ChunkSize();
						break;
					}
				}

				SetLastError(errPrefix + "triangle mesh (object " + ToString(objIdx) + ") has an invalid face list.");
				return false;
				
			// skip entire chunk
			default:
				cursor += chunk.ChunkSize();
				break;
			}
		}
	}

	// -- build local mesh --
	
	if (!numVertices || !numFaces)
	{
		SetLastError(errPrefix + "object " + ToString(objIdx) + " does not have a vertex and/or face list.");
		return false;
	}		

	Vector3 boxMin(0.f), boxMax(0.f);
	for (unsigned int iVertex = 0; iVertex < numVertices; ++iVertex)
	{
		boxMin.m_X = std::min(pVertices[iVertex].m_X, boxMin.m_X);
		boxMin.m_Y = std::min(pVertices[iVertex].m_Y, boxMin.m_Y);
		boxMin.m_Z = std::min(pVertices[iVertex].m_Z, boxMin.m_Z);
		boxMax.m_X = std::max(pVertices[iVertex].m_X, boxMax.m_X);
		boxMax.m_Y = std::max(pVertices[iVertex].m_Y, boxMax.m_Y);
		boxMax.m_Z = std::max(pVertices[iVertex].m_Z, boxMax.m_Z);
	}

	const Vector3 boxDim = boxMax - boxMin;
	const Vector3 boxScale = Vector3(1.f) / (boxDim * (1.f / scaleTo));
	const Vector3 toCenter = Vector3(boxMin + boxDim * 0.5f);

	m_meshDim = boxDim.Scale(boxScale);
	
	AllocateVertices(numVertices);
	AllocateIndices(numFaces);

	const bool hasUV = m_flexVtx.HasElements(FV_UV);
	const bool hasColor = m_flexVtx.HasElements(FV_COLOR);
	const bool hasNormal = m_flexVtx.HasElements(FV_NORMAL);
	
	for (unsigned int iVertex = 0; iVertex < numVertices; ++iVertex)
	{
		m_flexVtx[iVertex].Position() = (pVertices[iVertex] - toCenter).Scale(boxScale);
		if (hasColor) m_flexVtx[iVertex].Color() = 0xffffffff;
		if (hasUV) m_flexVtx[iVertex].UV() = (pUVs) ? *pUVs++ : Vector2(0.f);
	}	

	for (unsigned int iFace = 0, iIndex = 0; iFace < numFaces; ++iFace)
	{
		m_pIndices[iIndex++] = *pIndices++;
		m_pIndices[iIndex++] = *pIndices++;
		m_pIndices[iIndex++] = *pIndices++;
		++pIndices;
	}

	if (hasNormal)
	{
		CalculateNormals();
	}

	if (m_flexVtx.HasElements(FV_UV | FV_NORMAL | FV_TANGENT | FV_BINORMAL))
	{
		CalculateTangentsAndBinormals();
	}

	return (m_localOnly) ? true : CreateRendererBuffers();
}

bool Mesh::GeneratePlane(const Vector2 &dimensions, const Vector2 &UVscale, unsigned int tesselation, bool centerUV /* = false */)
{
	TPB_ASSERT(!LocalMeshValid() && tesselation > 0);

	const unsigned int tessPlusOne = tesselation + 1;
	const unsigned int numVertices = tessPlusOne * tessPlusOne;
	const unsigned int numFaces = tesselation * tesselation * 2;
	
	AllocateVertices(numVertices);
	AllocateIndices(numFaces);

	const bool hasUV = m_flexVtx.HasElements(FV_UV);
	const bool hasColor = m_flexVtx.HasElements(FV_COLOR);
	const bool hasNormal = m_flexVtx.HasElements(FV_NORMAL);

	const float fCenterUV = (centerUV) ? 0.f : 0.5f;
	const float step = 1.f / tesselation;
	
	Vector3 curVec(0.f, -0.5f, 1.f);
	for (unsigned int iVertex = 0, iY = 0; iY < tessPlusOne; ++iY)
	{
		curVec.m_X = -0.5f;
		for (unsigned int iX = 0; iX < tessPlusOne; ++iX)
		{
			m_flexVtx[iVertex].Position() = curVec.Scale(Vector3(dimensions));
			if (hasUV)     m_flexVtx[iVertex].UV() = Vector2(curVec.m_X + fCenterUV, -curVec.m_Y + fCenterUV).Scale(UVscale);
			if (hasColor)  m_flexVtx[iVertex].Color() = 0xffffffff;
			if (hasNormal) m_flexVtx[iVertex].Normal() = Vector3(0.f, 0.f, 1.f);
			++iVertex;	
		
			curVec.m_X += step;
		}
		
		curVec.m_Y += step;
	}

	unsigned int iIndex = 0, yOffs = 0;
	for (unsigned int iY = 0; iY < tesselation; ++iY)
	{
		for (unsigned int iX = 0; iX < tesselation; ++iX)
		{
			const unsigned int yOffsPlusOne = yOffs + tessPlusOne;
			const unsigned int A = yOffs + iX;
			const unsigned int B = yOffs + iX + 1;
			const unsigned int C = yOffsPlusOne + iX;
			const unsigned int D = yOffsPlusOne + iX + 1;

			m_pIndices[iIndex++] = A;
			m_pIndices[iIndex++] = D;
			m_pIndices[iIndex++] = B;
			m_pIndices[iIndex++] = C;
			m_pIndices[iIndex++] = D;
			m_pIndices[iIndex++] = A;
		}

		yOffs += tessPlusOne;
	}

	m_meshDim = Vector3(dimensions);

	return (m_localOnly) ? true : CreateRendererBuffers();
}

bool Mesh::LoadFromLWOInMemory(const OpaqueData &fileData)
{
	TPB_IMPLEMENT
	return false;
}

void Mesh::Draw() const
{
	TPB_ASSERT(RendererMeshValid());
	m_renderer.SetVertexFormat(m_flexVtx);
	m_renderer.SetVertexBuffer(m_pVB, m_flexVtx.GetStride());
	m_renderer.SetIndexBuffer(m_pIB);
	m_renderer.DrawIndexed(Renderer::PT_TRIANGLE_LIST, m_numPrimitives, m_numVertices);
}

void Mesh::DrawLocalMesh() const
{
	TPB_ASSERT(LocalMeshValid());
	m_renderer.SetVertexFormat(m_flexVtx);
	m_renderer.DrawIndexedDirect(Renderer::PT_TRIANGLE_LIST, m_numPrimitives, m_numVertices, m_pVertices, m_flexVtx.GetStride(), m_pIndices);
}

void Mesh::DrawNormals() const
{
	TPB_ASSERT(LocalMeshValid());
	TPB_IMPLEMENT
}

void Mesh::AllocateVertices(unsigned int numVertices)
{
	TPB_ASSERT(!m_numVertices && numVertices < 65536 && m_pVertices == NULL);
	m_numVertices = numVertices;
	m_pVertices = new Byte[numVertices * m_flexVtx.GetStride()];
	m_flexVtx.SetStream(m_pVertices);
}

void Mesh::AllocateIndices(unsigned int numFaces)
{
	TPB_ASSERT(!m_numPrimitives && m_pIndices == NULL);
	m_numPrimitives = numFaces;
	m_pIndices = new uint16_t[numFaces * 3];
}

void Mesh::CalculateNormals()
{
	TPB_ASSERT(LocalMeshValid() && (m_flexVtx.HasElements(FV_NORMAL)));

	for (unsigned int iVertex = 0; iVertex < m_numVertices; ++iVertex)
	{
		m_flexVtx[iVertex].Normal() = Vector3(0.f);
	}

	for (unsigned int iFace = 0, iIndex = 0; iFace < m_numPrimitives; ++iFace)
	{
		const uint16_t iA = m_pIndices[iIndex++];			
		const uint16_t iB = m_pIndices[iIndex++];			
		const uint16_t iC = m_pIndices[iIndex++];

		// Calculate polygon edges (CW).
		const Vector3 BA = m_flexVtx[iB].Position() - m_flexVtx[iA].Position();
		const Vector3 CA = m_flexVtx[iC].Position() - m_flexVtx[iA].Position();

		// Cross product yields a face normal.
		const Vector3 faceNormal = BA.CrossProduct(CA).Normalize();

		// Calculate edge directions (CW).
		const Vector3 edgeDirections[3] = {
			BA.Normalize(),
			(m_flexVtx[iC].Position() - m_flexVtx[iB].Position()).Normalize(),
			(m_flexVtx[iA].Position() - m_flexVtx[iC].Position()).Normalize()
		};
		
		// Calculate respective face angles.
		Vector3 faceAngles(
			edgeDirections[0].Angle(edgeDirections[2]),
			edgeDirections[1].Angle(edgeDirections[0]),
			edgeDirections[2].Angle(edgeDirections[1]));
		faceAngles /= kPI; // A 180 degree radius will yield [-PI, PI], make it [-1, -1].
		
		// Add weighted face normal to vertices.
		m_flexVtx[iA].Normal() += faceNormal * faceAngles.m_X;
		m_flexVtx[iB].Normal() += faceNormal * faceAngles.m_Y;
		m_flexVtx[iC].Normal() += faceNormal * faceAngles.m_Z;
	}

	for (unsigned int iVertex = 0; iVertex < m_numVertices; ++iVertex)
	{
		m_flexVtx[iVertex].Normal() = m_flexVtx[iVertex].Normal().Normalize();
	}
}

// Algorithm provided by Tim van Klooster.
void Mesh::CalculateTangentsAndBinormals()
{
	TPB_ASSERT(m_flexVtx.HasElements(FV_UV | FV_NORMAL | FV_TANGENT | FV_BINORMAL));

	for (unsigned int iVertex = 0; iVertex < m_numVertices; ++iVertex)
	{
		m_flexVtx[iVertex].Tangent() = Vector3(0.f);
		m_flexVtx[iVertex].Binormal() = Vector3(0.f);
	}

	for (unsigned int iFace = 0, iIndex = 0; iFace < m_numPrimitives; ++iFace)
	{
		const uint16_t iA = m_pIndices[iIndex++];
		const uint16_t iB = m_pIndices[iIndex++];
		const uint16_t iC = m_pIndices[iIndex++];

		// Calculate polygon edges (CW).
		const Vector3 polyEdges[2] = {
			m_flexVtx[iB].Position() - m_flexVtx[iA].Position(),
			m_flexVtx[iC].Position() - m_flexVtx[iA].Position()
		};
		
		// Calculate UV edges.
		const Vector2 uvEdges[2] = {
			m_flexVtx[iB].UV()- m_flexVtx[iA].UV(),
			m_flexVtx[iC].UV()- m_flexVtx[iA].UV()
		};

		// ...
		const Vector3 tempTangent = polyEdges[1] * uvEdges[0].m_X - polyEdges[0] * uvEdges[1].m_X;
		const Vector3 tempBinormal = polyEdges[1] * uvEdges[0].m_Y - polyEdges[0] * uvEdges[1].m_Y;
		
		// Vertex A.
		m_flexVtx[iA].Tangent() += tempTangent.CrossProduct(m_flexVtx[iA].Normal()).Normalize();
		m_flexVtx[iA].Binormal() += tempBinormal.CrossProduct(m_flexVtx[iA].Normal()).Normalize();

		// Vertex B.
		m_flexVtx[iB].Tangent() += tempTangent.CrossProduct(m_flexVtx[iB].Normal()).Normalize();
		m_flexVtx[iB].Binormal() += tempBinormal.CrossProduct(m_flexVtx[iB].Normal()).Normalize();

		// Vertex C.
		m_flexVtx[iC].Tangent() += tempTangent.CrossProduct(m_flexVtx[iC].Normal()).Normalize();
		m_flexVtx[iC].Binormal() += tempBinormal.CrossProduct(m_flexVtx[iC].Normal()).Normalize();
	}

	for (unsigned int iVertex = 0; iVertex < m_numVertices; ++iVertex)
	{
		m_flexVtx[iVertex].Tangent() = m_flexVtx[iVertex].Tangent().Normalize();
		m_flexVtx[iVertex].Binormal() = m_flexVtx[iVertex].Binormal().Normalize();
	}
}

bool Mesh::CreateRendererBuffers()
{
	TPB_ASSERT(LocalMeshValid() && !RendererMeshValid());

	m_pVB = m_renderer.CreateVertexBuffer(m_numVertices * m_flexVtx.GetStride(), false);
	m_pIB = m_renderer.CreateIndexBuffer(m_numPrimitives * 3, false, false);
	if (m_pVB != NULL && m_pIB != NULL)
	{
		m_pVB->Fill(m_pVertices);
		m_pIB->Fill(m_pIndices);
		return true;
	}
	
	return false;
}
