
// tpbds -- Render targets (wrapper & stack).

#include "main.h"
#include "rendertargets.h"

// -- Wrapper. --

bool RenderTarget::AllocateSurface()
{
	m_pRenderTarget = m_renderer.CreateReferenceRenderTarget(m_mipLevel);
	if (m_pRenderTarget != NULL)
	{
		return true;
	}
	
	return false;
}

void RenderTarget::Set(bool clearTarget, bool clearDepthStencil, D3DCOLOR clearColor /* = 0 */) const
{
	DWORD clearBits = 0;
	if (clearTarget) clearBits |= D3DCLEAR_TARGET;
	if (clearDepthStencil) clearBits |= D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL; // Performance hit if masked separately.
	m_renderer.SetRenderTarget(0, m_pRenderTarget, clearBits, clearColor);
}

void RenderTarget::Blit(const Blitter &blitter, uint32_t blendMode, float alpha) const
{
	// If m_pRenderTarget is NULL, this target is either uninitialized or the back buffer.
	TPB_ASSERT(m_pRenderTarget != NULL);

	// Can't blit to myself, now can I?
	TPB_ASSERT(m_pRenderTarget != m_renderer.GetRenderTarget(0));

	// Blit!
	blitter.Blit(m_pRenderTarget, blendMode, alpha);
}

// -- Stack. --

void RenderTargetStack::Push(RenderTarget &renderTarget, bool clearTarget, bool clearDepthStencil, D3DCOLOR clearColor /* = 0 */)
{
	renderTarget.Set(clearTarget, clearDepthStencil, clearColor);
	m_stack.push_back(&renderTarget);
}

void RenderTargetStack::Pop(bool clearDepthStencil)
{
	TPB_ASSERT(!m_stack.empty());

	// Pop target.
	m_stack.pop_back();

	if (!m_stack.empty())
	{
		// Restore previous target.
		m_stack.back()->Set(false, clearDepthStencil);
	}
}
