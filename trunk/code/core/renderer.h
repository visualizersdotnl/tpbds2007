
// tpbds -- low-level Direct3D 9 renderer

#ifndef _RENDERER_H_
#define _RENDERER_H_

class Renderer;

// essentials
#include "Allocator.h"
#include "polyflags.h"
#include "textureflags.h"
#include "flexvertex.h"
#include "colorutil.h"
#include "stdaspectratios.h"

// standard shader includes
#include "stdvs.h"
#include "stdps.h"

// stock shaders
#include "stockshaders/sprite2D.h"
#include "stockshaders/unlit3D.h"
#include "stockshaders/lit3D.h"

// sampler bounds
const unsigned int kMaxPixelSamplers = 16; // roughly valid for VS/PS 2.0
const unsigned int kMaxVertexSamplers = 4; // D3D spec. allows 256 stages; 4 seems sensible

// transient heap size & base alignment
const size_t kTransientHeapSize = 1024 * 1024;
const size_t kTransientHeapAlignment = 32;

// scratch vertex heap size
const size_t kScratchVertexHeapSize = 1024 * 1024;

class Renderer : public NoCopy
{
public:
	// -- resource types --

	// Resources are contained by and registered with the Renderer instance.
	// If you add a type, please adhere to the following rules:
	// - Inherit from Resource and implement what's necessary.
	// - Befriend your class with Renderer and keep everything except client functionality at least protected!
	// - Ensure that the destructor takes care of both local and Direct3D resources.

	// Generic interface (all types must inherit from this class).
	class Resource : public NoCopy
	{
		friend class Renderer;
	
	private:
		enum Type
		{
			SHADERPAIR,
			TEXTURE,
			VERTEX_BUFFER,
			INDEX_BUFFER
		};
		
		Resource(Type type, bool isResettable) :
			m_type(type), 
			m_isResettable(isResettable) {}

		Type GetType() const { return m_type; }
		bool IsResettable() const { return m_isResettable; }
		
		// If the assertions trigger, a type is flagged as resettable and should implement these functions!
		virtual void OnDeviceLost() { TPB_ASSERT(!m_isResettable); }
		virtual bool OnDeviceReset(IDirect3DDevice9 &d3dDev) { TPB_ASSERT(!m_isResettable); return true; }

		Type m_type;
		bool m_isResettable;
	};

	class ShaderPair : public Resource
	{
		friend class Renderer;

	private:		
		ShaderPair() :
			Resource(SHADERPAIR, false),
			m_pVS(NULL),
			m_pPS(NULL) {}
	
		~ShaderPair() 
		{
			SAFE_RELEASE(m_pVS);
			SAFE_RELEASE(m_pPS);
		}

		IDirect3DVertexShader9 *m_pVS;
		IDirect3DPixelShader9 *m_pPS;
		int m_reqFlexVtxBits;
	};

	class Texture : public Resource
	{
		friend class Renderer;

	public:
		void *Lock(int &pitch, unsigned int iLevel = 0);
		void Unlock(unsigned int iLevel = 0);
		
		const Vector2 GetResolution() const 
		{ 
			return Vector2((float) m_xRes, (float) m_yRes); 
		}
		
	private:		
		enum Type
		{
			STATIC,
			DYNAMIC,
			RENDER_TARGET
		};

		Texture(Type type, D3DPOOL Pool, unsigned int xRes, unsigned int yRes, unsigned int mipLevels, D3DFORMAT Format, DWORD Usage) :
			Resource(TEXTURE, Pool != D3DPOOL_MANAGED), 
			m_type(type),
			m_pD3DTex(NULL),
			m_pD3DSurf(NULL),
			m_isLocked(false),
			m_Pool(Pool),
			m_xRes(xRes),
			m_yRes(yRes),
			m_mipLevels(mipLevels),
			m_Format(Format),
			m_Usage(Usage) {}

		// Only for non-D3DPOOL_DEFAULT!
		Texture(Type type, IDirect3DTexture9 *pD3DTex) :
			Resource(TEXTURE, false),
			m_type(type),
			m_pD3DTex(pD3DTex),
			m_pD3DSurf(NULL),
			m_isLocked(false)
		{
			TPB_ASSERT(m_pD3DTex != NULL);
			
			D3DSURFACE_DESC surfDesc;
			m_pD3DTex->GetLevelDesc(0, &surfDesc);
			
			// Correct.
			m_xRes = surfDesc.Width;
			m_yRes = surfDesc.Height;
			m_mipLevels = m_pD3DTex->GetLevelCount();

			// Safe to assume that all surface levels share the following properties.
			TPB_ASSERT(surfDesc.Pool != D3DPOOL_DEFAULT);
			m_Pool = surfDesc.Pool;
			m_Format = surfDesc.Format;
			m_Usage = surfDesc.Usage;
		}

		~Texture() 
		{ 
			// Can't call OnDeviceLost() because it, correctly, ignores D3DPOOL_MANAGED.
			SAFE_RELEASE(m_pD3DSurf);
			SAFE_RELEASE(m_pD3DTex);
		}

		bool Create(IDirect3DDevice9 &d3dDev);

		virtual void OnDeviceLost();
		virtual bool OnDeviceReset(IDirect3DDevice9 &d3dDev);

		const Type m_type;		
		IDirect3DTexture9 *m_pD3DTex;
		bool m_isLocked;

		// For RENDER_TARGET.
		IDirect3DSurface9 *m_pD3DSurf; 
		bool m_nonRefSize;

		// Create() + OnDeviceReset()
		D3DPOOL m_Pool;
		unsigned int m_xRes, m_yRes;
		unsigned int m_mipLevels;
		D3DFORMAT m_Format;
		DWORD m_Usage;
	};	

	template<typename T> class Buffer : public Resource
	{
	public:
		FIX_ME // Lock failure is not properly handled, proper functioning is merely asserted.

		// Fully locks the buffer (dynamic buffers discard any previous content).
		void *Lock()
		{
			TPB_ASSERT(m_pBuffer != NULL && m_pLocked == NULL);
			const DWORD Usage = (m_isDynamic) ? D3DLOCK_DISCARD : 0;
			TPB_VERIFY(D3D_OK == m_pBuffer->Lock(0, 0, &m_pLocked, Usage));
			return m_pLocked;
		}

		// Locks the buffer partially.
		// Try not to overwrite any memory that might be in use (D3DLOCK_NOOVERWRITE).
		void *LockRegion(size_t byteOffs, size_t bytesToLock, bool doFullDiscard)
		{
			TPB_ASSERT(m_isDynamic); // This function is intended to be used with dynamic buffers only.
			TPB_ASSERT(m_pBuffer != NULL && m_pLocked == NULL);
			
			if (byteOffs + bytesToLock > m_numBytes)
			{
				// No space left.
				TPB_ASSERT(0);
				return NULL;
			}		

			TPB_VERIFY(D3D_OK == m_pBuffer->Lock(byteOffs, bytesToLock, &m_pLocked, (doFullDiscard) ? D3DLOCK_DISCARD : D3DLOCK_NOOVERWRITE));
			return m_pLocked;
		}

		void Unlock(bool updateSysCopy = true)
		{
			TPB_ASSERT(m_pLocked != NULL);
			
			// Update local copy (for non-dynamic buffer) if requested.
			if (m_pSysCopy != NULL && updateSysCopy) 
			{
				memcpy(m_pSysCopy, m_pLocked, m_numBytes);
			}
			
			TPB_VERIFY(D3D_OK == m_pBuffer->Unlock());
			m_pLocked = NULL;
		}

		void Fill(const void *pSrc, size_t numBytes)
		{
			TPB_ASSERT(numBytes <= m_numBytes);
			void *pDest = Lock();
			if (pDest != NULL)
			{
				memcpy(pDest, pSrc, numBytes);
				
				if (m_pSysCopy != NULL)
				{
					// update local copy directly
					memcpy(m_pSysCopy, pSrc, numBytes);
				}
				
				Unlock(false);
			}
			else 
			{
				TPB_ASSERT(0);
			}
		}

		void Fill(const void *pSrc)
		{
			Fill(pSrc, m_numBytes);
		}
		
	protected: // This can't be private since this class is not a friend of Renderer.
		Buffer(Type type, size_t byteSize, bool isDynamic, DWORD Usage) :
			Resource(type, true),
			m_numBytes(byteSize),
			m_isDynamic(isDynamic),
			m_Usage(Usage),
			m_pSysCopy((!isDynamic) ? malloc(byteSize) : NULL),
			m_pBuffer(NULL),
			m_pLocked(NULL) {}

		virtual ~Buffer()
		{
			free(m_pSysCopy);
			OnDeviceLost();
		}

		const size_t m_numBytes;
		const bool m_isDynamic;
		const DWORD m_Usage;
		void *m_pSysCopy;

		T *m_pBuffer;
		void *m_pLocked;

	private:
		virtual bool Create(IDirect3DDevice9 &d3dDev) = 0;

		virtual void OnDeviceLost()
		{
			if (m_pLocked != NULL) Unlock();
			SAFE_RELEASE(m_pBuffer);
			m_pBuffer = NULL;
		}

		virtual bool OnDeviceReset(IDirect3DDevice9 &d3dDev)
		{
			if (Create(d3dDev))
			{
				if (m_pSysCopy)
				{
					memcpy(Lock(), m_pSysCopy, m_numBytes);
					Unlock(false);
				}
				
				return true;
			}
			
			return false;
		}
	};

	class VertexBuffer : public Buffer<IDirect3DVertexBuffer9>
	{
		friend class Renderer;
	
	private:
		VertexBuffer(size_t byteSize, bool isDynamic, DWORD Usage) :
			Buffer(VERTEX_BUFFER, byteSize, isDynamic, Usage) {}
		
		~VertexBuffer() {}

		virtual bool Create(IDirect3DDevice9 &d3dDev);
	};

	class IndexBuffer : public Buffer<IDirect3DIndexBuffer9> 
	{
		friend class Renderer;
		
	private:
		IndexBuffer(size_t byteSize, bool isDynamic, DWORD Usage, bool is32B) :
			Buffer(INDEX_BUFFER, byteSize, isDynamic, Usage),
			m_is32B(is32B) {}
		
		~IndexBuffer() {}
		
		virtual bool Create(IDirect3DDevice9 &d3dDev);
		
		const bool m_is32B;
	};

	// -- device --
	
	static bool DoCompatibilityTests(UINT iAdapter, IDirect3D9 &d3dObj, const D3DCAPS9 &caps);

	// renderAspectRatio - Aspect ratio of default viewport.
	Renderer(IDirect3DDevice9 &d3dDev, bool softVP, float renderAspectRatio);
	~Renderer();

	// xRes, yRes          - Output resolution.
	// dissplayAspectRatio - Output aspect ratio (typically yRes/xRes).
	bool Initialize(unsigned int xRes, unsigned int yRes, float displayAspectRatio);

	void OnDeviceLost();
	bool OnDeviceReset();

	void BeginFrame(D3DCOLOR backBufferClearColor = 0);
	void EndFrame();

	// Access to transient heap (main RAM).
	template<typename T> T *AllocateScratchMemory(size_t numBytes, size_t alignTo = 4)
	{
		TPB_ASSERT(m_isInFrame); // In a BeginFrame()-EndFrame()-cycle?
		T *pAlloc = reinterpret_cast<T *>(m_transientHeap.Allocate(numBytes, alignTo));
		TPB_ASSERT(pAlloc != NULL); // Heap exhausted?
		return pAlloc;
	}

	// Small container to return scratch vertex allocations.	
	class ScratchVertices
	{
	public:
		ScratchVertices(void *pVertices, size_t byteOffs) :
			m_pVertices(pVertices),
			m_byteOffs(byteOffs) {}

		void *m_pVertices;
		const size_t m_byteOffs; // Offset in dyn. vertex buffer.
	};

	// Multiple locks are allowed, but always free them with UnlockScratchVertices().
	// Drawing can only take place if all locks are freed.
	const ScratchVertices LockScratchVertices(size_t numBytes, size_t vtxStride);
	void UnlockScratchVertices();

	// Use Direct3D D3DCLEAR_* flags for clearBits!
	void SetRenderTarget(unsigned int iTarget, const Texture *pTexture, DWORD clearBits, D3DCOLOR clearColor = 0); 
	
	const Texture *GetRenderTarget(unsigned int iTarget) const
	{ 
		TPB_ASSERT(iTarget < s_numSimultaneousRTs); 
		return m_pCurrentRenderTarget[iTarget]; 
	}

	// Returns the current render target's resolution in image space.
	// Useful for certain shaders.
	const Vector2 GetRenderTargetResolution(unsigned int iTarget) const
	{ 
		TPB_ASSERT(iTarget < s_numSimultaneousRTs);
		
		if (iTarget == 0)
		{
			return Vector2((float) m_curVP.Width, (float) m_curVP.Height); 
		}
		else
		{
			if (m_pCurrentRenderTarget[iTarget] != NULL)
			{
				return m_pCurrentRenderTarget[iTarget]->GetResolution();
			}
			else
			{
				TPB_ASSERT(0);
				return Vector2(0.f);
			}
		}
	}

	void SetPolyFlags(uint32_t flags);
	void SetTexture(unsigned int iSampler, const Texture *pTexture, uint32_t flags);
	void SetTextureFlags(unsigned int iSampler, uint32_t flags);
	void SetVertexTexture(unsigned int iSampler, const Texture *pTexture, uint32_t flags);
	void SetVertexTextureFlags(unsigned int iSampler, uint32_t flags);
	void SetShaders(const ShaderPair *pShaderPair);
	void SetVertexFormat(const FlexVertex &flexVtx);
	void SetVertexBuffer(const VertexBuffer *pVB, size_t vtxStride, size_t byteOffs = 0);
	void SetIndexBuffer(const IndexBuffer *pIB);

	enum StockShaders
	{
		SS_SPRITE2D,
		SS_UNLIT3D,
		SS_LIT3D
	};

	void SetStockShaders(StockShaders stockPair)
	{
		switch (stockPair)
		{
		case SS_SPRITE2D:
			SetShaders(m_pSprite2D);
			break;
		
		case SS_UNLIT3D:
			SetShaders(m_pUnlit3D);
			break;

		case SS_LIT3D:
			SetShaders(m_pLit3D);
			break;
		
		default:
			TPB_ASSERT(0);
		};
	}

	// Set scratch vertex buffer. Always supply correct offset!
	void SetScratchVertexBuffer(size_t vtxStride, size_t byteOffs)
	{
		SetVertexBuffer(m_pScratchVB, vtxStride, byteOffs);		
	}

	// Fog settings:
	// - Uploaded to pixel shader constant registers on SetPolyFlags(), enable with kPolyFlagFog.
	// - Supported by all 3D stock shaders.
	// - For more details: stdps.h & stdps.inc.
	void SetFogParameters(float nearViewSpaceZ, float viewSpaceRange);
	void SetFogColor(D3DCOLOR XRGB);

	// List of supported primitive types.
	// Expand as needed.
	enum PrimType
	{
		PT_POINT_LIST = D3DPT_POINTLIST,
		PT_LINE_LIST = D3DPT_LINELIST,
		PT_LINE_STRIP = D3DPT_LINESTRIP,
		PT_TRIANGLE_LIST = D3DPT_TRIANGLELIST,
		PT_TRIANGLE_STRIP = D3DPT_TRIANGLESTRIP
	};

	// The parameters here are mostly copied from the Direct3D SDK.
	// numPrimitives - Number of entire primitives to draw (data requirements differ per type).
	// numVertices   - Number of vertices involved.
	// vertexStride  - Size, in bytes, of a vertex.
	// Some of these calls can be modifed to accept more parameters (for offsetting) if need be.
	void Draw(PrimType primType, unsigned int numPrimitives);
	void DrawDirect(PrimType primType, unsigned int numPrimitives, const void *pVertices, size_t vertexStride);
	void DrawIndexed(PrimType primType, unsigned int numPrimitives, unsigned int numVertices);
	void DrawIndexedDirect(PrimType primType, unsigned int numPrimitives, unsigned int numVertices, const void *pVertices, size_t vertexStride, const uint16_t *pIndices);

	// The QuadLists are an extension, but I might consider just making them a PrimType.
	//
	// The vertex order should look like this:
	// A ------------- B (Y+)
	// |               |
	// |               |
	// |               |
	// |               |
	// |               |
	// C ------------- D (Y-)

	void DrawQuadList(unsigned int numPrimitives);
	void DrawQuadListDirect(unsigned int numPrimitives, const void *pVertices, size_t vertexStride);

	// Shader constant upload.
	void SetVertexShaderConstantF(unsigned int iReg, unsigned int numConstants, const float *pData);
	void SetVertexShaderConstantI(unsigned int iReg, unsigned int numConstants, const int *pData);
	void SetVertexShaderConstantB(unsigned int iReg, unsigned int numConstants, const BOOL *pData);
	void SetPixelShaderConstantF(unsigned int iReg, unsigned int numConstants, const float *pData);
	void SetPixelShaderConstantI(unsigned int iReg, unsigned int numConstants, const int *pData);
	void SetPixelShaderConstantB(unsigned int iReg, unsigned int numConstants, const BOOL *pData);

	void SetVertexShaderConstantV(unsigned int iReg, unsigned int numVectors, const Vector4 *pV)
	{
		for (unsigned int iVector = 0; iVector < numVectors; ++iVector)
			SetVertexShaderConstantF(iReg, 1, pV[iVector].GetData());
	}
	
	void SetVertexShaderConstantM4x4(unsigned int iReg, unsigned int numMatrices, const Matrix4x4 *pM)
	{
		for (unsigned int iMatrix = 0; iMatrix < numMatrices; ++iMatrix)
			SetVertexShaderConstantF(iReg, 4, pM[iMatrix].GetData());
	}
	
	void SetPixelShaderConstantV(unsigned int iReg, unsigned int numVectors, const Vector4 *pV)
	{
		for (unsigned int iVector = 0; iVector < numVectors; ++iVector)
			SetPixelShaderConstantF(iReg, 1, pV[iVector].GetData());
	}

	void SetPixelShaderConstantM4x4(unsigned int iReg, unsigned int numMatrices, const Matrix4x4 *pM)
	{
		for (unsigned int iMatrix = 0; iMatrix < numMatrices; ++iMatrix)
			SetPixelShaderConstantF(iReg, 4, pM[iMatrix].GetData());
	}

	// Easy to use pass-by-reference versions of the 4 functions above.
	void SetVertexShaderConstantV(unsigned int iReg, const Vector4 &V) { SetVertexShaderConstantV(iReg, 1, &V); }
	void SetVertexShaderConstantM4x4(unsigned int iReg, const Matrix4x4 &M) { SetVertexShaderConstantM4x4(iReg, 1, &M); }
	void SetPixelShaderConstantV(unsigned int iReg, const Vector4 &V) { SetPixelShaderConstantV(iReg, 1 ,&V); }
	void SetPixelShaderConstantM4x4(unsigned int iReg, const Matrix4x4 &M) { SetPixelShaderConstantM4x4(iReg, 1, &M); }

	// Need an unexposed Direct3D feature?
	IDirect3DDevice9 &GetAPI() const { return m_d3dDev; }
	
	ShaderPair *CreateShaderPair(const void *pVSBytecode, const void *pPSBytecode, int reqFlexVtxBits);
	void DestroyShaderPair(ShaderPair *pShaderPair);

	Texture *CreateTexture(unsigned int xRes, unsigned int yRes, unsigned int mipLevels, bool hiQual, bool isDynamic);
	Texture *CreateTextureFromFile(const std::string &path, bool generateMipLevels, bool noRoundingToPow2);
	Texture *CreateTextureFromFileInMemory(const OpaqueData &fileData, bool generateMipLevels, bool noRoundingToPow2);

	Texture *CreateRenderTarget(unsigned int xRes, unsigned int yRes, D3DFORMAT format);

	// Create a (32-bit ARGB() render target with viewport-derived dimensions.
	Texture *CreateReferenceRenderTarget(unsigned int mipLevel, D3DFORMAT format = D3DFMT_A8R8G8B8)
	{
		return CreateRenderTarget(m_fullVP.Width >> mipLevel, m_fullVP.Height >> mipLevel, format);
	}

	void DestroyTexture(Texture *pTexture);

	VertexBuffer *CreateVertexBuffer(size_t byteSize, bool isDynamic);
	void DestroyVertexBuffer(VertexBuffer *pVertexBuffer); 
	IndexBuffer *CreateIndexBuffer(unsigned int numIndices, bool isDynamic, bool is32B);
	void DestroyIndexBuffer(IndexBuffer *pIndexBuffer);

	// Rendering aspect ratio.
	float GetAspectRatio() const { return m_renderAspectRatio; }

	// Default textures.
	const Texture *GetWhiteTexture() const { return m_pDefTexture; }
	const Texture *GetBlackTexture() const { return m_pBlackTexture; }

private:
	bool GetPrimarySurfaces();
	void ResetState();

	// taken from D3DCAPS9
	static DWORD s_numSimultaneousRTs;

	// Renderer keeps track of all it's resources using this list.
	std::list<Resource *> m_resources;

	// Heaps.
	AllocatorClient m_transientHeap; // local heap (encapsulates an Allocator instance)
	Allocator m_scratchVtxAllocator; // separate Allocator for scratch vertices (m_pScratchVB)

	// Direct3D device.
	IDirect3DDevice9 &m_d3dDev;
	DWORD m_softVP;

	// Rendering aspect ratio.
	float m_renderAspectRatio;

	// to assert correct use according to device state
	bool m_isInitialized;
	bool m_devLost;
	bool m_isInFrame;

	// default resources
	ShaderPair *m_pSprite2D;
	ShaderPair *m_pUnlit3D;
	ShaderPair *m_pLit3D;
	Texture *m_pDefTexture, *m_pBlackTexture;
	IDirect3DSurface9 *m_pBackBuffer, *m_pDepthStencil;

	// for scratch vertices
	VertexBuffer *m_pScratchVB;
	unsigned int m_scratchLockCount;

	// for DrawQuadList()
	IndexBuffer *m_pQuadIB;

	// Viewports.
	D3DVIEWPORT9 m_nativeVP;	
	D3DVIEWPORT9 m_fullVP;
	D3DVIEWPORT9 m_quarterVP;

	// current render target(s) & VP
	const Renderer::Texture **m_pCurrentRenderTarget;
	D3DVIEWPORT9 m_curVP;

	// current geometry buffers
	const VertexBuffer *m_pCurVB;
	const IndexBuffer *m_pCurIB;

	// basic render state (flags)
	uint32_t m_curPolyFlags;
	uint32_t m_curTexFlags[kMaxPixelSamplers];
//	uint32_t m_curVertexTexFlags[kMaxVertexSamplers];
	uint32_t m_curFlexVertexFlags;
	uint32_t m_reqFlexVertexFlags;

	// Fog settings.
	float m_fogNear;
	float m_fogRange;
	D3DCOLOR m_fogXRGB;
};

// stack-allocated scratch vertex buffers
#include "scratchvertices.h"

#endif // _RENDERER_H_
