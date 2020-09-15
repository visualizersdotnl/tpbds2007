
// tpbds -- misc. color conversion functions

#ifndef _COLORUTIL_H_
#define _COLORUTIL_H_

// D3DCOLOR to Vector4 (range [0, 1], RGBA)
// useful for shader constants
inline Vector4 D3DCOLORToVector4(D3DCOLOR ARGB)
{
	return Vector4(
		(float) (ARGB >> 16 & 255) / 255.f,
		(float) (ARGB >> 8  & 255) / 255.f,
		(float) (ARGB       & 255) / 255.f,
		(float) (ARGB >> 24 & 255) / 255.f);
}

// floating point alpha [0, 1] + 24-bit integer RGB to D3DCOLOR
inline D3DCOLOR AlphaAndRGBToD3DCOLOR(float alpha, unsigned int RGB)
{
	TPB_ASSERT(alpha >= 0.f && alpha <= 1.f);
	TPB_ASSERT(RGB >> 24 == 0);
	const uint8_t byteAlpha = (uint8_t) (alpha * 255.f);
	return byteAlpha << 24 | RGB;
}

#endif // _COLORUTIL_H_
