
namespace RainbowRoad
{
	const unsigned int kMatrixResX = 1280 / 32, kMatrixResY = 720 / 32;
	const unsigned int kNumQuads = kMatrixResX * kMatrixResY;
	const float kDotSizeX = 2.f / (float) kMatrixResX, kDotSizeY = 2.f / (float) kMatrixResY;

	struct Dot
	{
		float timeOfBirth;
		float timeToLive;
		float timeToFade;
		D3DCOLOR RGB;
	} dotMatrix[kNumQuads];
	
	unsigned int randomLine[kMatrixResX * 4];
	float scrollSpeeds[kMatrixResY];
	unsigned int scrollOffsets[kMatrixResY];
	D3DCOLOR finColors[kNumQuads];

	FlexVertex *pFlexVtx = NULL;
	Renderer::VertexBuffer *pMatrixVB = NULL;

	ResourceHub::Texture *pDotTexture;
	PixelEffect marcher("code/demo/testhack/rainbowroad.ps");

	void RequestResources(ResourceHub &resHub)
	{
		pDotTexture = resHub.RequestTexture("content/from_1995/env2.jpg", true, false);
		marcher.Request(resHub);
	}

	bool Create()
	{
		pFlexVtx = new FlexVertex(*s_pRenderer, FV_POSITION | FV_COLOR | FV_UV);
		
		const size_t bufferSize = kNumQuads * pFlexVtx->GetStride() * 4;
		pMatrixVB = s_pRenderer->CreateVertexBuffer(bufferSize, true);

		for (unsigned int iX = 0; iX < kMatrixResX * 4; ++iX)
			randomLine[iX] = rand() % 255;
		
		srand(18985344);
		for (unsigned int iY = 0; iY < kMatrixResY; ++iY)
		{
			scrollSpeeds[iY] = 0.5f + randf(1.5f);
			scrollOffsets[iY] = rand() % kMatrixResX;
		}

		return true;
	}

	void Destroy()
	{
		delete pFlexVtx;
		s_pRenderer->DestroyVertexBuffer(pMatrixVB);
	}

	void Render(float time)
	{
		static bool firstFrame = true;

		for (unsigned int iDot = 0, iY = 0; iY < kMatrixResY; ++iY)
		{
			for (unsigned int iX = 0; iX < kMatrixResX; ++iX, ++iDot)
			{
				Dot &dot = dotMatrix[iDot];
			
				if (time >= dot.timeOfBirth + dot.timeToLive)
				{
					dot.timeToLive = 8.f + randf(20.f);
					dot.timeToFade = 2.f + randf(2.f);
					TPB_ASSERT(dot.timeToLive >= dot.timeToFade * 2.f);
					dot.RGB = 0x7f7fff;

					if (!firstFrame)
					{
						dot.timeOfBirth = time;
					}
					else
					{
						dot.timeOfBirth = time - randf(dot.timeToLive * 0.9f);
					}
				}
				
				const float lifeTime = time - dot.timeOfBirth;
				float intensity;
				if (lifeTime < dot.timeToFade) 
				{
					intensity = lifeTime * (1.f / dot.timeToFade);
				}
				else if (lifeTime > dot.timeToLive - dot.timeToFade) 
				{
					intensity = (dot.timeToLive - lifeTime) * (1.f / dot.timeToFade);
				}
				else
				{
					intensity = 1.f;
				}
				
				float scrollOffset = (float) (scrollOffsets[iY] + iX);
				scrollOffset += time * scrollSpeeds[iY];
				const float scrollDelta = scrollOffset - floorf(scrollOffset);
				const unsigned int iRandom = (unsigned int) floorf(scrollOffset);
				const float randomA = (float) randomLine[iRandom % (kMatrixResX * 4)] / 255.f;
				const float randomB = (float) randomLine[(iRandom + 1) % (kMatrixResX * 4)] / 255.f;
				const float random = lerp(randomA, randomB, scrollDelta);
				intensity = intensity * (0.7f + random * 0.3f);
			
				finColors[iDot] = AlphaAndRGBToD3DCOLOR(intensity, dot.RGB);
			}
		}

		FIX_ME // FlexVertex bypassed.
		struct DotVertex
		{
			float X, Y, Z;
			D3DCOLOR color;
			float U, V;
		};

		DotVertex *pVerts = static_cast<DotVertex *>(pMatrixVB->Lock());
		float yPos = 1.f;
		for (unsigned int iDot = 0, iY = 0; iY < kMatrixResY; ++iY)
		{
			float xPos = -1.f;
			for (unsigned int iX = 0; iX < kMatrixResX; ++iX, ++iDot)
			{
				const D3DCOLOR color = finColors[iDot];
			
				pVerts->X = xPos;
				pVerts->Y = yPos;
				pVerts->Z = 1.f;
				pVerts->color = color;
				pVerts->U = 0.f;
				pVerts->V = 0.f;
				++pVerts;

				pVerts->X = xPos + kDotSizeX;
				pVerts->Y = yPos;
				pVerts->Z = 1.f;
				pVerts->color = color;
				pVerts->U = 1.f;
				pVerts->V = 0.f;
				++pVerts;

				pVerts->X = xPos;
				pVerts->Y = yPos - kDotSizeY;
				pVerts->Z = 1.f;
				pVerts->color = color;
				pVerts->U = 0.f;
				pVerts->V = 1.f;
				++pVerts;

				pVerts->X = xPos + kDotSizeX;
				pVerts->Y = yPos - kDotSizeY;
				pVerts->Z = 1.f;
				pVerts->color = color;
				pVerts->U = 1.f;
				pVerts->V = 1.f;
				++pVerts;
				
				xPos += kDotSizeX;
			}
			
			yPos -= kDotSizeY;
		}
		pMatrixVB->Unlock();

		s_pRenderer->SetPolyFlags(kPolyFlagAlphaMod);
		s_pRenderer->SetTexture(0, pDotTexture->Get(), kTexFlagDef);
		s_pRenderer->SetStockShaders(Renderer::SS_SPRITE2D);
		s_pRenderer->SetVertexFormat(*pFlexVtx);
		s_pRenderer->SetVertexBuffer(pMatrixVB, pFlexVtx->GetStride());
		s_pRenderer->DrawQuadList(kNumQuads);

//		s_pRenderer->SetTexture(0, m_pTexture->Get(), kTexFlagDef);
		marcher.Draw(kPolyFlagOpaque);		

		firstFrame = false;
	}
};
