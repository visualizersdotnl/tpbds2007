
// tpbds -- scratch vertex buffers (hardware & local RAM)

#ifndef _SCRATCH_VERTICES_H_
#define _SCRATCH_VERTICES_H_

// Hardware version.
class ScratchVertices : public NoCopy
{
public:
	ScratchVertices(Renderer &renderer, uint32_t flexVertexBits, unsigned int numVertices) :
		m_renderer(renderer),
		m_flexVtx(renderer, flexVertexBits),
		m_numVertices(numVertices),
		m_scratchVerts(renderer.LockScratchVertices(numVertices * GetStride(), GetStride()))
	{
		m_flexVtx.SetStream(m_scratchVerts.m_pVertices);
	}

	~ScratchVertices() {}

	FlexVertex::Accessor operator [](unsigned int iVertex)
	{
		TPB_ASSERT(iVertex < m_numVertices);
		return m_flexVtx[iVertex];
	}

	// Unlocks the buffer and locks up the access mechanism.
	void Unlock()
	{
		TPB_ASSERT(m_flexVtx.GetStream() != NULL);
		m_flexVtx.SetStream(NULL);
		m_renderer.UnlockScratchVertices();
	}

	// Binds the buffer. Will call Unlock() first if still locked.
	void SetForDraw(Renderer &renderer)
	{
		if (m_flexVtx.GetStream() != NULL)
		{
			Unlock();
		}
		
		renderer.SetVertexFormat(m_flexVtx);
		renderer.SetScratchVertexBuffer(GetStride(), GetByteOffset());
	}

	const FlexVertex &GetFlexVertex() const { return m_flexVtx; }
	size_t GetStride() const { return m_flexVtx.GetStride(); }
	size_t GetByteOffset() const { return m_scratchVerts.m_byteOffs; }

private:
	Renderer &m_renderer;

	FlexVertex m_flexVtx;
	unsigned int m_numVertices;
	const Renderer::ScratchVertices m_scratchVerts;
};

// Local RAM version.
class ScratchVerticesRAM : public NoCopy
{
public:
	ScratchVerticesRAM(Renderer &renderer, uint32_t flexVertexBits, unsigned int numVertices) :
		m_flexVtx(renderer, flexVertexBits),
		m_numVertices(numVertices)
	{
		m_flexVtx.SetStream(renderer.AllocateScratchMemory<Byte>(numVertices * GetStride(), 32));
	}

	~ScratchVerticesRAM() {}

	FlexVertex::Accessor operator [](unsigned int iVertex)
	{
		TPB_ASSERT(iVertex < m_numVertices);
		return m_flexVtx[iVertex];
	}

	const FlexVertex &GetFlexVertex() const { return m_flexVtx; }
	size_t GetStride() const { return m_flexVtx.GetStride(); }
	Byte *GetBuffer() const { return m_flexVtx.GetStream(); }

private:
	FlexVertex m_flexVtx;
	unsigned int m_numVertices;
};

#endif // _SCRATCH_VERTICES_H_
