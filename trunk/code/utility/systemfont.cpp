
// tpbds -- System font.

#include "main.h"
#include "resources.h"
#include "systemfont.h"
#include "draw2D.h"

static const ResourceHub::Texture *s_pSysFontTex = NULL;

void SystemFont_RequestTexture(ResourceHub &resHub)
{
	s_pSysFontTex = resHub.RequestTexture("content/sys_88font.png", true, false);
}

// Number of rows (number of columns (X) is kNumRows multiplied by aspect ratio).
const unsigned int kNumRows = 40;

// Size of a character in texture space (16x16 pixels).
const float kCharDeltaUV = 1.f / 16.f;

// Character-to-UV.
inline const Vector2 CharacterUV(char character)
{
	const unsigned int row = character / 16;
	const unsigned int column = character - row*16;
	return Vector2((float) column * kCharDeltaUV, (float) row * kCharDeltaUV);
}

void SystemFont_Draw(Renderer &renderer, const std::string &text, unsigned int xPos, unsigned int yPos)
{
	TPB_ASSERT(s_pSysFontTex != NULL);

	const float numRows = kNumRows;
	const float numColumns = renderer.GetAspectRatio() * numRows;
	const float charSizeX = 1.f / numColumns;
	const float charSizeY = 1.f / numRows;

	Vector2 curPos((float) xPos * charSizeX, (float) yPos * charSizeY);

	renderer.SetPolyFlags(kPolyFlagAdditive | kPolyFlagNoZBuffer);
	renderer.SetTexture(0, s_pSysFontTex->Get(), kTexFlagPointSamplingBi);

	const size_t numChars = text.length();
	for (size_t iChar = 0; iChar < numChars; ++iChar)
	{
		const Vector2 UV = CharacterUV(text[iChar]);
		FontQuad(renderer, curPos, Vector2(charSizeX, charSizeY), 0.f, UV.m_X, UV.m_Y, kCharDeltaUV, kCharDeltaUV);
		curPos.m_X += charSizeX;
	}

	renderer.SetTextureFlags(0, kTexFlagDef);
}
