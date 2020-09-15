
// tpbds -- static triangle mesh (CW-order) class with basic 3DS support

#ifndef _MESH_H_
#define _MESH_H_

// To do (for materials & animation, likely useful for LightWave objects):
// - Add support to split the mesh up into PrimGroups (batches).
// - Create a PrimGroupMaterial class.

class Mesh : public NoCopy
{
public:
	Mesh(Renderer &renderer, uint32_t flexVertexBits, bool localOnly);
	~Mesh();

	void Evict();
	void EvictLocalMesh();

	bool LoadFrom3DS(const std::string &path, float scaleTo = 1.f, unsigned int objIdx = 0);
	bool LoadFrom3DSInMemory(const OpaqueData &fileData, float scaleTo = 1.f, unsigned int objIdx = 0);
	bool LoadFromLWOInMemory(const OpaqueData &fileData);
	bool GeneratePlane(const Vector2 &dimensions, const Vector2 &UVscale, unsigned int tesselation, bool centerUV = false);

	void Draw() const;
	void DrawLocalMesh() const;
	void DrawNormals() const; // Debug feature.

	const FlexVertex &GetFlexVertex() const
	{ 
		TPB_ASSERT(LocalMeshValid());
		return m_flexVtx;
	}
	
	unsigned int GetNumVertices() const { return m_numVertices; }
	unsigned int GetNumIndices() const  { return m_numPrimitives * 3; }

//	void UpdateMesh();

	bool LocalMeshValid() const
	{
		return m_pVertices != NULL && m_pIndices != NULL;
	} 
	
	bool RendererMeshValid() const
	{
		return m_pVB != NULL && m_pIB != NULL;
	}

	const Vector3 GetDimensions() const { return m_meshDim; }

private:
	void AllocateVertices(unsigned int numVertices);
	void AllocateIndices(unsigned int numFaces);
	void CalculateNormals();
	void CalculateTangentsAndBinormals();
	bool CreateRendererBuffers();

	Renderer &m_renderer;		
	FlexVertex m_flexVtx;
	const bool m_localOnly;

	unsigned int m_numVertices, m_numPrimitives;
	Byte *m_pVertices;
	uint16_t *m_pIndices;

	Renderer::VertexBuffer *m_pVB;
	Renderer::IndexBuffer *m_pIB;

	Vector3 m_meshDim;
};

#endif // _MESH_H_
