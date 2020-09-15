
// tpbds -- Render targets (wrapper & stack).

#ifndef _RENDER_TARGETS_H_
#define _RENDER_TARGETS_H_

// Uses Blitter.
#include "blitter.h"

// RenderTarget:
// - Proportionally sized 32-bit ARGB render target.
// - If not allocated, it will represent the back buffer (see Renderer::SetRenderTarget()).
// - Blits using any Blitter.
class RenderTarget : public NoCopy
{
public:
	RenderTarget(Renderer &renderer, unsigned int mipLevel) :
		m_renderer(renderer),
		m_mipLevel(mipLevel), // Relative to back buffer's size.
		m_pRenderTarget(NULL) {}

	~RenderTarget()
	{
		m_renderer.DestroyTexture(m_pRenderTarget);
		m_pRenderTarget = NULL;
	}

	bool AllocateSurface();

	void Set(bool clearTarget, bool clearDepthStencil, D3DCOLOR clearColor = 0) const;
	void Blit(const Blitter &blitter, uint32_t blendMode, float alpha) const; // See Blitter::Blit().

	const Renderer::Texture *GetTexture()
	{
		TPB_ASSERT(m_pRenderTarget != NULL);
		return m_pRenderTarget;
	}

private:
	Renderer &m_renderer;
	const unsigned int m_mipLevel;

	Renderer::Texture *m_pRenderTarget;
};

// Simple RenderTarget stack.
// Supports non-allocated instances (treated as back buffer).
class RenderTargetStack : public NoCopy
{
public:
	void Push(RenderTarget &renderTarget, bool clearTarget, bool clearDepthStencil, D3DCOLOR clearColor = 0);
	void Pop(bool clearDepthStencil);

	size_t GetSize() const { return m_stack.size(); }

private:
	std::vector<RenderTarget *> m_stack;
};

#endif // _RENDER_TARGETS_H_
