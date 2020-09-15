
// tpbds -- low-level Direct3D 9 renderer

#include "main.h"

// -- resource types --

void *Renderer::Texture::Lock(int &pitch, unsigned int iLevel /* = 0 */)
{
	TPB_ASSERT(m_pD3DTex != NULL && m_type != RENDER_TARGET && !m_isLocked);
	D3DLOCKED_RECT lockedRect;
	TPB_VERIFY(D3D_OK == m_pD3DTex->LockRect(iLevel, &lockedRect, NULL, (m_type == DYNAMIC) ? D3DLOCK_DISCARD : 0));
	m_isLocked = true;
	pitch = lockedRect.Pitch;
	return lockedRect.pBits;
}

void Renderer::Texture::Unlock(unsigned int iLevel /* = 0 */)
{
	TPB_ASSERT(m_isLocked);
	m_pD3DTex->UnlockRect(iLevel);
	m_isLocked = false;
}

bool Renderer::Texture::Create(IDirect3DDevice9 &d3dDev)
{
	TPB_ASSERT(m_pD3DTex == NULL && m_pD3DSurf == NULL);

	switch (d3dDev.CreateTexture(m_xRes, m_yRes, m_mipLevels, m_Usage, m_Format, m_Pool, &m_pD3DTex, 0))
	{
	case D3D_OK:
		if (m_type == RENDER_TARGET)
		{
			m_pD3DTex->GetSurfaceLevel(0, &m_pD3DSurf);
			TPB_ASSERT(m_pD3DSurf != NULL);
		}

		return true;

	case D3DERR_INVALIDCALL: 
		TPB_ASSERT(0);
	
	case D3DERR_OUTOFVIDEOMEMORY: 
	case E_OUTOFMEMORY:
		{
			FIX_ME // Must include format in the descriptions below!
			const std::string resDesc("(" + ToString(m_xRes) + "*" + ToString(m_yRes) + ")");
			switch (m_type)
			{
			case STATIC:
				SetLastError("Can't create a Direct3D texture " + resDesc);
				break;

			case DYNAMIC:
				SetLastError("Can't create a dynamic Direct3D texture " + resDesc);
				break;

			case RENDER_TARGET:
				SetLastError("Can't create a Direct3D texture to use as render target " + resDesc);
				break;
			}
		}
	}

	return false;
}

void Renderer::Texture::OnDeviceLost()
{
	if (m_Pool == D3DPOOL_DEFAULT)
	{
		SAFE_RELEASE(m_pD3DSurf);
		SAFE_RELEASE(m_pD3DTex);
	}
}

bool Renderer::Texture::OnDeviceReset(IDirect3DDevice9 &d3dDev)
{
	if (m_Pool == D3DPOOL_DEFAULT)
	{
		if (Create(d3dDev))
		{
			return true;
		}
		
		// Failure.
		return false;
	}

	// Managed by Direct3D: skip!
	return true;
}

bool Renderer::VertexBuffer::Create(IDirect3DDevice9 &d3dDev)
{
	TPB_ASSERT(m_pBuffer == NULL);
	IDirect3DVertexBuffer9 *pVB;
	if (d3dDev.CreateVertexBuffer(m_numBytes, m_Usage | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &pVB, NULL) == D3D_OK)
	{
		m_pBuffer = pVB;
		return true;
	}

	SetLastError("Can not create a Direct3D vertex buffer (" + ToString(m_numBytes) + " bytes).");
	return false;
}

bool Renderer::IndexBuffer::Create(IDirect3DDevice9 &d3dDev)
{
	TPB_ASSERT(m_pBuffer == NULL);
	IDirect3DIndexBuffer9 *pIB;
	if (d3dDev.CreateIndexBuffer(m_numBytes, m_Usage | D3DUSAGE_WRITEONLY, (m_is32B) ? D3DFMT_INDEX32 : D3DFMT_INDEX16, D3DPOOL_DEFAULT, &pIB, NULL) == D3D_OK)
	{
		m_pBuffer = pIB;
		return true;
	}

	if (m_is32B)
		SetLastError("Can not create a 32-bit Direct3D index buffer (" + ToString(m_numBytes) + " bytes).");
	else
		SetLastError("Can not create a 16-bit Direct3D index buffer (" + ToString(m_numBytes) + " bytes).");
	
	return false;
}

// -- device --

DWORD Renderer::s_numSimultaneousRTs = 1; // Default, if you don't bother to call DoCompatibilityTests().

/* static */ bool Renderer::DoCompatibilityTests(UINT iAdapter, IDirect3D9 &d3dObj, const D3DCAPS9 &caps)
{
	// Supported by SetVertexBuffer() and at least used by ScratchVertexBuffer.
	if (!(caps.DevCaps2 & D3DDEVCAPS2_STREAMOFFSET))
	{
		SetLastError("Display adapter does not support stream source offsetting (D3DDDEVCAPS2_STREAMOFFSET).");
		return false;
	}

	s_numSimultaneousRTs = caps.NumSimultaneousRTs;

	return true;	
}

Renderer::Renderer(IDirect3DDevice9 &d3dDev, bool softVP, float renderAspectRatio) :
	m_transientHeap(static_cast<Byte *>(AlignedAlloc(kTransientHeapSize, kTransientHeapAlignment)), kTransientHeapSize),
	m_scratchVtxAllocator(kScratchVertexHeapSize),
	m_d3dDev(d3dDev),
	m_softVP((softVP) ? D3DUSAGE_SOFTWAREPROCESSING : 0),
	m_renderAspectRatio(renderAspectRatio),
	m_isInitialized(false),
	m_devLost(false),
	m_isInFrame(false),
	m_pSprite2D(CreateShaderPair(g_vs30_core_ss_sprite2D, g_ps30_core_ss_sprite2D, flexBitsSprite2D)),
	m_pUnlit3D(CreateShaderPair(g_vs30_core_ss_unlit3D, g_ps30_core_ss_unlit3D, flexBitsUnlit3D)),
	m_pLit3D(CreateShaderPair(g_vs30_core_ss_lit3D, g_ps30_core_ss_lit3D, flexBitsLit3D)),
	m_pDefTexture(NULL), m_pBlackTexture(NULL),
	m_pBackBuffer(NULL), m_pDepthStencil(NULL),
	m_pScratchVB(NULL),
	m_scratchLockCount(0),
	m_pQuadIB(NULL),
	m_pCurrentRenderTarget(new const Renderer::Texture *[s_numSimultaneousRTs])
{
}

Renderer::~Renderer()
{
	AlignedFree(m_transientHeap.GetHeap());
	DestroyShaderPair(m_pSprite2D);
	DestroyShaderPair(m_pUnlit3D);
	DestroyShaderPair(m_pLit3D);
	DestroyTexture(m_pDefTexture);
	DestroyTexture(m_pBlackTexture);
	SAFE_RELEASE(m_pBackBuffer);
	SAFE_RELEASE(m_pDepthStencil);
	DestroyVertexBuffer(m_pScratchVB);
	DestroyIndexBuffer(m_pQuadIB);
	delete m_pCurrentRenderTarget;

	if (!m_resources.empty())
	{
		// Renderer resource leak!
		TPB_ASSERT(0);

		// Dump leaked resource(s).
		for (std::list<Resource *>::const_iterator iEntry = m_resources.begin(); iEntry != m_resources.end(); ++iEntry)
		{
			delete *iEntry;
		}
		
		// And issue an error.
		SetLastError("Renderer resource(s) leaked! Amount: " + ToString(m_resources.size()) + ".");
	}
}

bool Renderer::Initialize(unsigned int xRes, unsigned int yRes, float displayAspectRatio)
{
	TPB_ASSERT(!m_devLost && !m_isInitialized);

	// -- create tiny black & white textures (4*4) --
	
	m_pDefTexture = CreateTexture(4, 4, 1, true, false);
	if (m_pDefTexture == NULL)
	{	
		return false;
	}
	
	m_pBlackTexture = CreateTexture(4, 4, 1, true, false);
	if (m_pBlackTexture == NULL)
	{
		return false;
	}

	int pitch;
	uint8_t *pPixelsWhite = static_cast<uint8_t *>(m_pDefTexture->Lock(pitch));
	uint8_t *pPixelsBlack = static_cast<uint8_t *>(m_pBlackTexture->Lock(pitch));
	for (unsigned int iY = 0; iY < 4; ++iY)
	{
		memset(pPixelsWhite, 255, 4 * 4);
		memset(pPixelsBlack,   0, 4 * 4);
		pPixelsWhite += pitch;
		pPixelsBlack += pitch;
	}
	m_pDefTexture->Unlock();
	m_pBlackTexture->Unlock();

	// fetch primary surfaces (default render target)
	if (!GetPrimarySurfaces())
	{
		return false;
	}

	// create dynamic buffer for scratch vertices
	m_pScratchVB = CreateVertexBuffer(kScratchVertexHeapSize, true);
	if (m_pScratchVB == NULL)
	{
		return false;
	}

	// -- create 16-bit index buffer for DrawQuadList() --

	m_pQuadIB = CreateIndexBuffer(16384 * 6, false, false);
	if (m_pQuadIB == NULL)
	{
		return false;
	}

	uint16_t *pIndices = static_cast<uint16_t *>(m_pQuadIB->Lock());
	unsigned int iIndex = 0;
	for (unsigned int iQuad = 0; iQuad < 16384; ++iQuad)
	{
		// Vertex layout (ABCD) is documented in the header.
		const unsigned int A = iQuad << 2;
		const unsigned int B = A + 1;
		const unsigned int C = A + 2;
		const unsigned int D = A + 3;
		pIndices[iIndex++] = C; // 2
		pIndices[iIndex++] = B; // 1
		pIndices[iIndex++] = D; // 3
		pIndices[iIndex++] = A; // 0
		pIndices[iIndex++] = B; // 1
		pIndices[iIndex++] = C; // 2
	}
	m_pQuadIB->Unlock();

	// -- define native (output) & derived viewports  --

	m_nativeVP.Width = xRes;
	m_nativeVP.Height = yRes;
	m_nativeVP.X = 0;
	m_nativeVP.Y = 0;
	m_nativeVP.MaxZ = 1.f;
	m_nativeVP.MinZ = 0.f;

	unsigned int xResAdj, yResAdj;
	if (displayAspectRatio < m_renderAspectRatio)
	{
		xResAdj = xRes;
		yResAdj = (unsigned int) ((float) xRes / m_renderAspectRatio);
	}
	else if (displayAspectRatio > m_renderAspectRatio)
	{
		xResAdj = (unsigned int) ((float) yRes * m_renderAspectRatio);
		yResAdj = yRes;
	}
	else // ==
	{
		xResAdj = xRes;
		yResAdj = yRes;
	}

	m_fullVP.Width = xResAdj;
	m_fullVP.Height = yResAdj;
	m_fullVP.X = (xRes - xResAdj) >> 1;	
	m_fullVP.Y = (yRes - yResAdj) >> 1;	
	m_fullVP.MaxZ = 1.f;
	m_fullVP.MinZ = 0.f;

	m_quarterVP = m_fullVP;
	m_quarterVP.Width >>= 1;
	m_quarterVP.Height >>= 1;

	// done!
	m_isInitialized = true;

	// prepare for rendering
	ResetState();

	return true;
}

void Renderer::OnDeviceLost()
{
	TPB_ASSERT(!m_devLost);
	m_devLost = true;
	m_isInFrame = false; // Terminate frame.

	SAFE_RELEASE(m_pBackBuffer);
	SAFE_RELEASE(m_pDepthStencil);

	for (std::list<Resource *>::const_iterator iEntry = m_resources.begin(); iEntry != m_resources.end(); ++iEntry)
	{
		(*iEntry)->OnDeviceLost();
	}
}

bool Renderer::OnDeviceReset()
{
	TPB_ASSERT(m_devLost);

	for (std::list<Resource *>::const_iterator iEntry = m_resources.begin(); iEntry != m_resources.end(); ++iEntry)
	{
		if (!(*iEntry)->OnDeviceReset(m_d3dDev))
		{
			return false;
		}
	}

	// this function can be called prior to Initialize()
	if (m_isInitialized)
	{
		if (!GetPrimarySurfaces())
		{
			return false;
		}
	}
	
	// Back in business!
	// I chose to split these calls up because of a "prioritized" assertion in BeginFrame() -> !m_devLost.
	m_devLost = false;

	if (m_isInitialized)
	{
		ResetState();
	}

	return true;
}

void Renderer::BeginFrame(D3DCOLOR backBufferClearColor /* = 0 */)
{
	TPB_ASSERT(!m_devLost && m_isInitialized && !m_isInFrame);
	m_isInFrame = true;

	// Reset frame allocators.
	m_transientHeap.GetAllocator().Reset();
	m_scratchVtxAllocator.Reset();

	SetRenderTarget(0, NULL, 0);
	
	m_d3dDev.SetViewport(&m_nativeVP);
	m_d3dDev.Clear(0, NULL, D3DCLEAR_TARGET, 0, 0.f, 0);

	m_d3dDev.SetViewport(&m_fullVP);
	m_d3dDev.Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, backBufferClearColor, 1.f, 255);
	m_d3dDev.BeginScene();

	SetPixelShaderConstantV(PS_VCONSTANTS_CI, Vector4(m_renderAspectRatio, 0.f, 0.f, 0.f));
}

void Renderer::EndFrame()
{
	TPB_ASSERT(m_isInFrame);
	m_isInFrame = false;
	
	m_d3dDev.EndScene();
	m_d3dDev.Present(NULL, NULL, NULL, NULL);

	FIX_ME // Use this check to determine and react to (problematicly) low VRAM situations?
//	const UINT availTexMem = m_d3dDev.GetAvailableTextureMem();
//	const std::string texMemStr = "GetAvailableTextureMem() = " + ToString(availTexMem) + "\n";
//	DebugPrint(texMemStr);
}

const Renderer::ScratchVertices Renderer::LockScratchVertices(size_t numBytes, size_t vtxStride)
{
	TPB_ASSERT(m_isInFrame); // In a BeginFrame()-EndFrame()-cycle?
	TPB_ASSERT(vtxStride != 0);

	// Allocate slot.
	const size_t offset = m_scratchVtxAllocator.Allocate(numBytes, vtxStride); // Align to vertex stride.
	TPB_ASSERT(offset != ALLOCATOR_FAIL); // Heap exhausted?

	// Place lock.
	// Multiple locks are allowed but they must all be freed before drawing.
	// It's a special case that circumvents the Buffer object's functionality.
	IDirect3DVertexBuffer9 *m_pBuffer = m_pScratchVB->m_pBuffer;
	void *pLocked = NULL;
	const DWORD lockFlags = (offset == 0) ? D3DLOCK_DISCARD : D3DLOCK_NOOVERWRITE;
	TPB_VERIFY(D3D_OK == m_pBuffer->Lock(offset, numBytes, &pLocked, lockFlags));
	m_pScratchVB->m_pLocked = (void *) ++m_scratchLockCount; // Increase lock count and update m_pLocked.
	return ScratchVertices(pLocked, offset);
}

void Renderer::UnlockScratchVertices()
{
	if (m_scratchLockCount > 0)
	{
		m_pScratchVB->Unlock();
		m_pScratchVB->m_pLocked = (void *) --m_scratchLockCount; // Decrease lock count and update m_pLocked.
	}
	else TPB_ASSERT(0); // Nothing left to unlock?
}

void Renderer::SetRenderTarget(unsigned int iTarget, const Texture *pTexture, DWORD clearBits, D3DCOLOR clearColor /* = 0 */)
{
	TPB_ASSERT(m_isInFrame);
	TPB_ASSERT(iTarget < s_numSimultaneousRTs);

	if (m_pCurrentRenderTarget[iTarget] != pTexture)
	{
		IDirect3DSurface9 *pSurf = NULL;
		if (iTarget == 0) // First slot works differently: the VP is adjusted (and cached) and the back buffer is supported.
		{
			D3DVIEWPORT9 curVP;
			if (pTexture != NULL)
			{
				TPB_ASSERT(pTexture->m_type == Texture::RENDER_TARGET);
				pSurf = pTexture->m_pD3DSurf;
				curVP = m_fullVP;
				curVP.X = curVP.Y = 0;
				curVP.Width = pTexture->m_xRes;
				curVP.Height = pTexture->m_yRes;
			}
			else
			{
				pSurf = m_pBackBuffer;
				curVP = m_fullVP;
			}
			
			// Cache.
			m_curVP = curVP;

			m_d3dDev.SetRenderTarget(iTarget, pSurf);
			TPB_VERIFY(D3D_OK == m_d3dDev.SetViewport(&curVP));
				
			// Used to offset screen space (range: [-1, 1], [1, -1]) 2D positions to correct for half-pixel sampler oddity.
			const float texelOffsets[] = { -1.f / (float) curVP.Width, 1.f / (float) curVP.Height, 0.f, 0.f };
			m_d3dDev.SetVertexShaderConstantF(VS_TEXEL_ALIGN_D3D9_CI, texelOffsets, 1);
		}
		else // Any other stage can only be set to a user-allocated target that's at least as big as the primary target.
		{
			if (pTexture != NULL)
			{
				TPB_ASSERT(pTexture->m_type == Texture::RENDER_TARGET);
				pSurf = pTexture->m_pD3DSurf;

				// Viewport will not be modified for any slot other than the first.
				// It's important to assert though that the target's dimensions suffice.
				TPB_ASSERT(m_curVP.Width >= pTexture->m_xRes && m_curVP.Height >= pTexture->m_yRes);
			}

			m_d3dDev.SetRenderTarget(iTarget, pSurf);
		}

		m_pCurrentRenderTarget[iTarget] = pTexture;
	}

	// Always perform Clear().
	if (clearBits != 0)
	{
		m_d3dDev.Clear(0, NULL, clearBits, clearColor, 1.f, 0);
	}
}

void Renderer::SetPolyFlags(uint32_t flags)
{
	TPB_ASSERT(m_isInFrame);

	uint32_t changedBits = m_curPolyFlags ^ flags;
	m_curPolyFlags = flags;

	// Fog state may be dirty regardless of changedBits, so upload it every time.
	if (flags & kPolyFlagFog)
	{
		SetPixelShaderConstantV(PS_VFOG_SETUP_CI, Vector4(m_fogNear, m_fogRange, 1.f, 0.f));

		// Upload black instead of the fog color whenever appropriate.
		switch (flags & kPolyFlagBlendMask)
		{
		case kPolyFlagOpaque: 
		case kPolyFlagAlpha:   
		case kPolyFlagColorMask:
		case kPolyFlagInvColorMask:
		case kPolyFlagAlphaMod:
			SetPixelShaderConstantV(PS_VFOG_COLOR_CI, D3DCOLORToVector4(m_fogXRGB));
			break;
		
		case kPolyFlagAdditive:
		case kPolyFlagSubtractive:
			SetPixelShaderConstantV(PS_VFOG_COLOR_CI, D3DCOLORToVector4(0));
			break;
		}
	}
	else
	{
		SetPixelShaderConstantV(PS_VFOG_SETUP_CI, Vector4(0.f));
	}

	// After each evaluation of changedBits, it is possible to return on !changedBits.
	// Jurjen K. once told me that he'd seen a properly inlined SetPolyFlags() function call
	// inlined and cut off at the applicable return statement, when fed a constant value.
	
	if (changedBits & kPolyFlagBlendMask)
	{
		switch (flags & kPolyFlagBlendMask)
		{
		case kPolyFlagOpaque: 
			m_d3dDev.SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			m_d3dDev.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
			break;
		
		case kPolyFlagAdditive:
			m_d3dDev.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_d3dDev.SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			m_d3dDev.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_d3dDev.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			m_d3dDev.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;
		
		case kPolyFlagSubtractive:
			m_d3dDev.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_d3dDev.SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
			m_d3dDev.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_d3dDev.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			m_d3dDev.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;
		
		case kPolyFlagAlpha:   
			m_d3dDev.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_d3dDev.SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			m_d3dDev.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_d3dDev.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			m_d3dDev.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;

		case kPolyFlagColorMask:
			m_d3dDev.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_d3dDev.SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
			m_d3dDev.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			m_d3dDev.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
			m_d3dDev.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;

		case kPolyFlagInvColorMask:
			m_d3dDev.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_d3dDev.SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
			m_d3dDev.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			m_d3dDev.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCCOLOR);
			m_d3dDev.SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			break;
		
		case kPolyFlagAlphaMod:
			m_d3dDev.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_d3dDev.SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			m_d3dDev.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_d3dDev.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
			m_d3dDev.SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
			break;
		}
	
//		changedBits &= ~kPolyFlagBlendMask;
	}
	
	if (changedBits & kPolyFlagNoCull)
	{
		m_d3dDev.SetRenderState(D3DRS_CULLMODE, (flags & kPolyFlagNoCull) ? D3DCULL_NONE : D3DCULL_CCW);
//		changedBits &= ~kPolyFlagNoCull;
	}

	if (changedBits & kPolyFlagNoZTest)
	{
		m_d3dDev.SetRenderState(D3DRS_ZFUNC, (flags & kPolyFlagNoZTest) ? D3DCMP_ALWAYS : D3DCMP_LESSEQUAL);
//		changedBits &= ~kPolyFlagNoZTest;
	}

	if (changedBits & kPolyFlagNoZWrite)
	{
		m_d3dDev.SetRenderState(D3DRS_ZWRITEENABLE, !(flags & kPolyFlagNoZWrite));
//		changedBits &= ~kPolyFlagNoZWrite;
	}

	if (changedBits & kPolyFlagLambert)
	{
		m_d3dDev.SetRenderState(D3DRS_SHADEMODE, (flags & kPolyFlagLambert) ? D3DSHADE_FLAT : D3DSHADE_GOURAUD);
//		changedBits &= ~kPolyFlagLambert;
	}

	if (changedBits & kPolyFlagWires)
	{
		m_d3dDev.SetRenderState(D3DRS_FILLMODE, (flags & kPolyFlagWires) ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
//		changedBits &= ~kPolyFlagWires;
	}

//	if (changedBits & kPolyFlagFog)
//	{
//	}

	if (changedBits & kPolyFlagToSRGB)
	{
		m_d3dDev.SetRenderState(D3DRS_SRGBWRITEENABLE, (flags & kPolyFlagToSRGB) ? TRUE : FALSE);
//		changedBits &= ~kPolyFlagToSRGB;
	}

	if (changedBits & kPolyFlagMaskRGBA)
	{
		// check order (this might be worth noting when switching to DirectX 10)  
		TPB_ASSERT(D3DCOLORWRITEENABLE_RED == kPolyFlagMaskR >> kPolyFlagMaskBitOffs);

		//  *mask* instead of *enable* -- hence the bitwise inversion 
		const DWORD writeMask = ~(flags & kPolyFlagMaskRGBA) >> kPolyFlagMaskBitOffs;
		m_d3dDev.SetRenderState(D3DRS_COLORWRITEENABLE, writeMask);

//		changedBits &= ~kPolyFlagMaskRGBA;
	}

//	TPB_ASSERT(!changedBits);
}

void Renderer::SetTexture(unsigned int iSampler, const Texture *pTexture, uint32_t flags)
{
	TPB_ASSERT(m_isInFrame); 
	TPB_ASSERT(iSampler < kMaxPixelSamplers);	

	if (pTexture != NULL)
	{
		m_d3dDev.SetTexture(iSampler, pTexture->m_pD3DTex);
	}
	else
	{
		m_d3dDev.SetTexture(iSampler, m_pDefTexture->m_pD3DTex);
	}

	SetTextureFlags(iSampler, flags);	
}

void Renderer::SetTextureFlags(unsigned int iSampler, uint32_t flags)
{
	TPB_ASSERT(m_isInFrame); 
	TPB_ASSERT(iSampler < kMaxPixelSamplers);

	const uint32_t changedBits = m_curTexFlags[iSampler] ^ flags;
	m_curTexFlags[iSampler] = flags;

	if (changedBits & kTexFlagSamplingMask)
	{
		switch (flags & kTexFlagSamplingMask)
		{
		case kTexFlagLinearSamplingTri:
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
			break;

		case kTexFlagLinearSamplingBi:
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
			break;

		case kTexFlagAnisoSamplingTri:
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
			break;

		case kTexFlagPointSamplingBi:
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
			break;
		}

//		changedBits &= ~kTexFlagSamplingMask;
	}

	if (changedBits & kTexFlagAddressMask)
	{
		switch (flags & kTexFlagAddressMask)
		{
		case kTexFlagAddressWrap:
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
			break;

		case kTexFlagAddressClamp:
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
			break;

		case kTexFlagAddressBorder:
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
			break;

		case kTexFlagAddressMirror:
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
			m_d3dDev.SetSamplerState(iSampler, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
			break;
		}

//		changedBits &= ~kTexFlagAddressMask;
	}

	if (changedBits & kTexFlagSRGBToLinear)
	{
		m_d3dDev.SetSamplerState(iSampler, D3DSAMP_SRGBTEXTURE, (flags & kTexFlagSRGBToLinear) ? 1 : 0);
//		changedBits &= ~kTexFlagSRGBToLinear;
	}

//	TPB_ASSERT(!changedBits);
}

void Renderer::SetVertexTexture(unsigned int iSampler, const Renderer::Texture *pTexture, uint32_t flags)
{
	TPB_ASSERT(m_isInFrame); 
	TPB_ASSERT(iSampler < kMaxVertexSamplers);

	// taken from SDK doc.
	const DWORD iSamplerMod = D3DDMAPSAMPLER + 1 + iSampler;

	if (pTexture != NULL)
	{
		m_d3dDev.SetTexture(iSamplerMod, pTexture->m_pD3DTex);
	}
	else
	{
		// No default texture used.
		m_d3dDev.SetTexture(iSamplerMod, NULL);
	}

	SetVertexTextureFlags(iSampler, flags);	
}

void Renderer::SetVertexTextureFlags(unsigned int iSampler, uint32_t flags)
{
	TPB_ASSERT(m_isInFrame); 
	TPB_ASSERT(iSampler < kMaxVertexSamplers);

//	const uint32_t changedBits = m_curVertexTexFlags[iSampler] ^ flags;
//	m_curVtxTexFlags[iSampler] = flags;

	// taken from SDK doc.
	const DWORD iSamplerMod = D3DDMAPSAMPLER + 1 + iSampler;
	
	// Now actually *set* them...
	TPB_IMPLEMENT
}

void Renderer::SetShaders(const ShaderPair *pShaderPair)
{
	TPB_ASSERT(m_isInFrame); 

	if (pShaderPair != NULL)
	{
		m_d3dDev.SetVertexShader(pShaderPair->m_pVS);
		m_d3dDev.SetPixelShader(pShaderPair->m_pPS);
		m_reqFlexVertexFlags = pShaderPair->m_reqFlexVtxBits;
	}
	else
	{
		m_d3dDev.SetVertexShader(NULL);
		m_d3dDev.SetPixelShader(NULL);
		m_reqFlexVertexFlags = 0;
	}
}

void Renderer::SetVertexFormat(const FlexVertex &flexVtx)
{
	TPB_ASSERT(m_isInFrame); 
	TPB_VERIFY(m_d3dDev.SetVertexDeclaration(flexVtx.GetVertexDecl()) == D3D_OK);
	m_curFlexVertexFlags = flexVtx.GetFlags();
}

void Renderer::SetVertexBuffer(const VertexBuffer *pVB, size_t vtxStride, size_t byteOffs /* = 0 */)
{
	TPB_ASSERT(m_isInFrame); 

	if (pVB != NULL)
	{
		TPB_VERIFY(m_d3dDev.SetStreamSource(0, pVB->m_pBuffer, byteOffs, vtxStride) == D3D_OK);
		m_pCurVB = pVB;
	}
	else
	{
		m_d3dDev.SetStreamSource(0, NULL, 0, 0);
		m_pCurVB = NULL;
	}
}

void Renderer::SetIndexBuffer(const IndexBuffer *pIB)
{
	TPB_ASSERT(m_isInFrame); 

	if (pIB != NULL)
	{
		TPB_VERIFY(m_d3dDev.SetIndices(pIB->m_pBuffer) == D3D_OK);
		m_pCurIB = pIB;
	}
	else
	{
		m_d3dDev.SetIndices(NULL);
		m_pCurIB = NULL;
	}
}

void Renderer::SetFogParameters(float nearViewSpaceZ, float viewSpaceRange)
{
	TPB_ASSERT(m_isInFrame); 
	m_fogNear = nearViewSpaceZ;
	m_fogRange = viewSpaceRange;
}

void Renderer::SetFogColor(D3DCOLOR XRGB)
{
	TPB_ASSERT(m_isInFrame); 
	m_fogXRGB = XRGB;
}

void Renderer::Draw(PrimType primType, unsigned int numPrimitives)
{
	TPB_ASSERT(m_isInFrame); 
	TPB_ASSERT(FlexVertex::FlagTest(m_curFlexVertexFlags, m_reqFlexVertexFlags));
	TPB_ASSERT(m_pCurVB != NULL && m_pCurVB->m_pLocked == NULL);
	m_d3dDev.DrawPrimitive((D3DPRIMITIVETYPE) primType, 0, numPrimitives);
}

void Renderer::DrawDirect(PrimType primType, unsigned int numPrimitives, const void *pVertices, size_t vertexStride)
{
	TPB_ASSERT(m_isInFrame); 
	TPB_ASSERT(pVertices != NULL && FlexVertex::FlagTest(m_curFlexVertexFlags, m_reqFlexVertexFlags));
	m_d3dDev.DrawPrimitiveUP((D3DPRIMITIVETYPE) primType, numPrimitives, pVertices, vertexStride);
}

void Renderer::DrawIndexed(PrimType primType, unsigned int numPrimitives, unsigned int numVertices)
{
	TPB_ASSERT(m_isInFrame); 
	TPB_ASSERT(FlexVertex::FlagTest(m_curFlexVertexFlags, m_reqFlexVertexFlags));
	TPB_ASSERT(m_pCurVB != NULL && m_pCurVB->m_pLocked == NULL);
	TPB_ASSERT(m_pCurIB != NULL && m_pCurIB->m_pLocked == NULL);
	m_d3dDev.DrawIndexedPrimitive((D3DPRIMITIVETYPE) primType, 0, 0, numVertices, 0, numPrimitives);
}

void Renderer::DrawIndexedDirect(PrimType primType, unsigned int numPrimitives, unsigned int numVertices, const void *pVertices, size_t vertexStride, const uint16_t *pIndices)
{
	TPB_ASSERT(m_isInFrame); 
	TPB_ASSERT(pVertices != NULL && pIndices && FlexVertex::FlagTest(m_curFlexVertexFlags, m_reqFlexVertexFlags));
	TPB_ASSERT(numVertices < 65536); // Upper limit for 16-bit indices.
	m_d3dDev.DrawIndexedPrimitiveUP((D3DPRIMITIVETYPE) primType, 0, numVertices, numPrimitives, pIndices, D3DFMT_INDEX16, pVertices, vertexStride);
}

void Renderer::DrawQuadList(unsigned int numPrimitives)
{
	TPB_ASSERT(m_isInFrame);
	TPB_ASSERT(FlexVertex::FlagTest(m_curFlexVertexFlags, m_reqFlexVertexFlags));
	TPB_ASSERT(m_pCurVB != NULL && m_pCurVB->m_pLocked == NULL);

	// The index buffer handles up to 16384 primitives per call.
	SetIndexBuffer(m_pQuadIB);
	INT baseVertexIndex = 0;

	// Draw number of "full" batches.
	const unsigned int numBatches = numPrimitives >> 14;
	for (unsigned int iBatch = 0; iBatch < numBatches; ++iBatch)
	{
		m_d3dDev.DrawIndexedPrimitive(D3DPT_TRIANGLELIST, baseVertexIndex, 0, 16384 * 4, 0, 16384 * 2);
		baseVertexIndex += 16384;
	}

	// Draw remainder.
	const unsigned int numRemainingPrimitives = numPrimitives & 16383;
	m_d3dDev.DrawIndexedPrimitive(D3DPT_TRIANGLELIST, baseVertexIndex, 0, numRemainingPrimitives << 2, 0, numRemainingPrimitives << 1);
}

void Renderer::DrawQuadListDirect(unsigned int numPrimitives, const void *pVertices, size_t vertexStride)
{
	DrawIndexedDirect(Renderer::PT_TRIANGLE_LIST, numPrimitives << 2, numPrimitives << 1, pVertices, vertexStride, static_cast<uint16_t *>(m_pQuadIB->m_pSysCopy));
}

void Renderer::SetVertexShaderConstantF(unsigned int iReg, unsigned int numConstants, const float *pData)
{
	TPB_ASSERT(m_isInFrame); 
	m_d3dDev.SetVertexShaderConstantF(iReg, pData, numConstants);
}

void Renderer::SetVertexShaderConstantI(unsigned int iReg, unsigned int numConstants, const int *pData)
{
	TPB_ASSERT(m_isInFrame); 
	m_d3dDev.SetVertexShaderConstantI(iReg, pData, numConstants);
}

void Renderer::SetVertexShaderConstantB(unsigned int iReg, unsigned int numConstants, const BOOL *pData)
{
	TPB_ASSERT(m_isInFrame); 
	m_d3dDev.SetVertexShaderConstantB(iReg, pData, numConstants);
}

void Renderer::SetPixelShaderConstantF(unsigned int iReg, unsigned int numConstants, const float *pData)
{
	TPB_ASSERT(m_isInFrame); 
	m_d3dDev.SetPixelShaderConstantF(iReg, pData, numConstants);
}

void Renderer::SetPixelShaderConstantI(unsigned int iReg, unsigned int numConstants, const int *pData)
{
	TPB_ASSERT(m_isInFrame); 
	m_d3dDev.SetPixelShaderConstantI(iReg, pData, numConstants);
}

void Renderer::SetPixelShaderConstantB(unsigned int iReg, unsigned int numConstants, const BOOL *pData)
{
	TPB_ASSERT(m_isInFrame); 
	m_d3dDev.SetPixelShaderConstantB(iReg, pData, numConstants);
}

Renderer::ShaderPair *Renderer::CreateShaderPair(const void *pVSBytecode, const void *pPSBytecode, int reqFlexVtxBits)
{
	TPB_ASSERT(!m_devLost);
	ShaderPair *pShaderPair = new ShaderPair();
	TPB_VERIFY(m_d3dDev.CreateVertexShader(static_cast<const DWORD *>(pVSBytecode), &pShaderPair->m_pVS) == D3D_OK);
	TPB_VERIFY(m_d3dDev.CreatePixelShader(static_cast<const DWORD *>(pPSBytecode), &pShaderPair->m_pPS) == D3D_OK);
	pShaderPair->m_reqFlexVtxBits = reqFlexVtxBits;
	m_resources.push_back(pShaderPair);
	return pShaderPair;
}

void Renderer::DestroyShaderPair(ShaderPair *pShaderPair)
{
	if (pShaderPair != NULL)
	{
		m_resources.remove(pShaderPair);
		delete pShaderPair;
	}
}

Renderer::Texture *Renderer::CreateTexture(unsigned int xRes, unsigned int yRes, unsigned int mipLevels, bool hiQual, bool isDynamic)
{
	TPB_ASSERT(!m_devLost);

	DWORD Usage;
	D3DPOOL Pool;
	if (isDynamic)
	{
		Usage = D3DUSAGE_DYNAMIC;
		Pool = D3DPOOL_DEFAULT;
	}
	else
	{
		Usage = 0;
		Pool = D3DPOOL_MANAGED;
	}

	const D3DFORMAT Format = (hiQual) ? D3DFMT_A8R8G8B8 : D3DFMT_A1R5G5B5;

	Texture *pTexture = new Texture((isDynamic) ? Texture::DYNAMIC : Texture::STATIC, Pool, xRes, yRes, mipLevels, Format, Usage);
	if (pTexture->Create(m_d3dDev))
	{
		m_resources.push_back(pTexture);
		return pTexture;
	}

	return NULL;
}

Renderer::Texture *Renderer::CreateTextureFromFile(const std::string &path, bool generateMipLevels, bool noRoundingToPow2)
{
	const OpaqueData fileData = LoadFile(path);
	if (fileData.IsValid())
	{
		return CreateTextureFromFileInMemory(fileData, generateMipLevels, noRoundingToPow2);
	}

	return NULL;
}

Renderer::Texture *Renderer::CreateTextureFromFileInMemory(const OpaqueData &fileData, bool generateMipLevels, bool noRoundingToPow2)
{
	TPB_ASSERT(!m_devLost);
	TPB_ASSERT(fileData.IsValid());

	const UINT sizeDirective = (noRoundingToPow2) ? D3DX_DEFAULT_NONPOW2 : D3DX_DEFAULT;
	IDirect3DTexture9 *pD3DTex;
	switch (D3DXCreateTextureFromFileInMemoryEx(
		&m_d3dDev,
		fileData.GetPtr(),
		(UINT) fileData.GetSize(), // Can safely cast down to 32-bit (if size_t is 64-bit); no texture is *that* large.
		sizeDirective,
		sizeDirective,
		(generateMipLevels) ? D3DX_DEFAULT : 1,
		0,
		D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED,
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		0,
		NULL,
		NULL,
		&pD3DTex))
	{
	case D3D_OK:
		{
			Texture *pTexture = new Texture(Texture::STATIC, pD3DTex);
			m_resources.push_back(pTexture);
			return pTexture;
		}

	case D3DERR_INVALIDCALL:
		TPB_ASSERT(0);
	
	case D3DERR_NOTAVAILABLE:
	case D3DERR_OUTOFVIDEOMEMORY: 
	case D3DXERR_INVALIDDATA:
	case E_OUTOFMEMORY:
		SetLastError("Can't create a managed 32-bit (ARGB) Direct3D texture (" + fileData.GetID() + ").");
	}

	return NULL;
}

Renderer::Texture *Renderer::CreateRenderTarget(unsigned int xRes, unsigned int yRes, D3DFORMAT format)
{
	TPB_ASSERT(!m_devLost);

	const DWORD Usage = D3DUSAGE_RENDERTARGET;
	const D3DPOOL Pool = D3DPOOL_DEFAULT;
	
	Texture *pTexture = new Texture(Texture::RENDER_TARGET, Pool, xRes, yRes, 1, format, Usage);
	if (pTexture->Create(m_d3dDev))
	{
		m_resources.push_back(pTexture);
		return pTexture;
	}

	return NULL;
}

void Renderer::DestroyTexture(Texture *pTexture)
{
	if (pTexture != NULL)
	{
		m_resources.remove(pTexture);
		delete pTexture;
	}
}

Renderer::VertexBuffer *Renderer::CreateVertexBuffer(size_t byteSize, bool isDynamic)
{
	TPB_ASSERT(!m_devLost);

	DWORD Usage = m_softVP;
	if (isDynamic) Usage |= D3DUSAGE_DYNAMIC;
	VertexBuffer *pVertexBuffer = new VertexBuffer(byteSize, isDynamic, Usage);
	if (pVertexBuffer->Create(m_d3dDev))
	{
		m_resources.push_back(pVertexBuffer);
		return pVertexBuffer;
	}
	
	TPB_ASSERT(0);

	delete pVertexBuffer;
	return NULL;	
}

void Renderer::DestroyVertexBuffer(VertexBuffer *pVertexBuffer)
{
	if (pVertexBuffer != NULL)
	{
		m_resources.remove(pVertexBuffer);
		delete pVertexBuffer;
	}
}

Renderer::IndexBuffer *Renderer::CreateIndexBuffer(unsigned int numIndices, bool isDynamic, bool is32B)
{
	TPB_ASSERT(!m_devLost);

	DWORD Usage = m_softVP;
	if (isDynamic) Usage |= D3DUSAGE_DYNAMIC;
	const size_t indexSize = (is32B) ? sizeof(int32_t) : sizeof(int16_t);
	IndexBuffer *pIndexBuffer = new IndexBuffer(numIndices * indexSize, isDynamic, Usage, is32B);
	if (pIndexBuffer->Create(m_d3dDev))
	{
		m_resources.push_back(pIndexBuffer);
		return pIndexBuffer;
	}

	TPB_ASSERT(0);

	delete pIndexBuffer;
	return NULL;
}

void Renderer::DestroyIndexBuffer(IndexBuffer *pIndexBuffer)
{
	if (pIndexBuffer != NULL)
	{
		m_resources.remove(pIndexBuffer);
		delete pIndexBuffer;
	}
}

bool Renderer::GetPrimarySurfaces()
{
	TPB_ASSERT(!m_isInFrame);
	TPB_ASSERT(m_pBackBuffer == NULL && m_pDepthStencil == NULL);

	m_d3dDev.GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
	m_d3dDev.GetDepthStencilSurface(&m_pDepthStencil); 

	if (m_pBackBuffer == NULL || m_pDepthStencil == NULL)
	{
		TPB_ASSERT(0); // This situation warrants an assertion: should never happen!
		SetLastError("Unable to retrieve primary Direct3D device surfaces.");
		return false;
	}

	return true;
}

void Renderer::ResetState()
{
	TPB_ASSERT(!m_isInFrame);

	// Reset native render targets (first slot set to NULL means def. back buffer enabled).
	for (DWORD iTarget = 0; iTarget < s_numSimultaneousRTs; ++iTarget)
	{
		m_pCurrentRenderTarget[iTarget] = NULL;
	}
	
	m_curVP = m_fullVP;

	// Unbind geometry buffers.
	m_d3dDev.SetStreamSource(0, NULL, 0, 0);
	m_d3dDev.SetIndices(NULL);
	m_pCurVB = NULL;
	m_pCurIB = NULL;

	// No vertex format set nor required.
	m_reqFlexVertexFlags = 0;
	m_curFlexVertexFlags = 0;

	// Dense magenta-colored fog.
	m_fogNear = -4.f;
	m_fogRange = 16.f;
	m_fogXRGB = 0x00ff00ff;

	// Turn off fixed pipe. It's turned on by default.
	m_d3dDev.SetRenderState(D3DRS_LIGHTING, FALSE); 

	// Always render anti-aliased lines, if possible.
	// When rendering to a multisample render target, D3DRS_ANTIALIASEDLINEENABLE is ignored and all lines are rendered "as usual".
	m_d3dDev.SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	// Flush a single frame.
	BeginFrame();
	{
		// Select simple opaque primitives.
		m_curPolyFlags = ~kPolyFlagOpaque; // Force state reset!
		SetPolyFlags(kPolyFlagOpaque);

		// For each (per pixel) sampler...
		for (unsigned int iSampler = 0; iSampler < kMaxPixelSamplers; ++iSampler)
		{
			// Set: default texture & trilinear sampling.
			m_curTexFlags[iSampler] = ~kTexFlagDef;
			SetTexture(iSampler, m_pDefTexture, kTexFlagDef);
		}
	}
	EndFrame();
}
