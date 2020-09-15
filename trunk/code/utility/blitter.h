
// tpbds -- blitter base

// Any Blitter implementation is supposed to execute a blit: copy a source image to the current
// render target using a particular technique. The RenderTarget class integrates this to perform
// exactly that task. Scroll down to learn more, it's pretty straightforward.

#ifndef _BLITTER_H_
#define _BLITTER_H_

// Base class.
class Blitter
{
public:
	// Some need a persistant instance, some don't: little hassle just to standardize it.
	Blitter(Renderer &renderer) :
		m_renderer(renderer) {}

	// The Blit() function should take care of all essential state.
	// The blendMode parameter is a subset of the polygon flags (e.g. kPolyFlagAdditive).
	virtual void Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const = 0;

	// Some blitters have a Filter() function which allows usage of a custom source image.
	// This isn't base class material, except for AssertFilterParameters().

	void AssertBlitParameters(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const
	{
		TPB_ASSERT(pTexture != NULL);
		AssertFilterParameters(blendMode, alpha);
	}

	void AssertFilterParameters(uint32_t blendMode, float alpha) const
	{
		TPB_ASSERT(blendMode == (blendMode & kPolyFlagBlendMask)); // Strict!
		TPB_ASSERT(alpha >= 0.f && alpha <= 1.f); 
	}

protected:
	Renderer &m_renderer;
};

// Tiling blitter.
class TileBlitter : public Blitter
{
public:
	TileBlitter(Renderer &renderer, float xTile, float yTile) :
		Blitter(renderer),
		m_xTile(xTile),
		m_yTile(yTile) {}

	virtual void Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const;

private:
	float m_xTile, m_yTile;
};

// Mosaik blitter (simply employs point sampling).
class MosaikBlitter : public Blitter
{
public:
	MosaikBlitter(Renderer &renderer, DWORD vtxColorRGB) : 
		Blitter(renderer),
		m_vtxColorRGB(vtxColorRGB) {} // For colorization.
	
	~MosaikBlitter() {}
	
	virtual void Blit(const Renderer::Texture *pTexture, uint32_t blendMode, float alpha) const;

private:
	DWORD m_vtxColorRGB;
};

// Advanced blitters.
#include "colormatrix.h"
#include "desaturate.h"
#include "kawase.h"
#include "hypnoglow.h"

#endif // _BLITTERS_H_
