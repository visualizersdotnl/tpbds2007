
// tpbds -- std. vertex shader constants

// matrices for rigid geometry
float4x4 mWorld : register(vs, c0);   // Upload manually.
float4x4 mView : register(vs, c4);    // Manual or uploaded by View::UploadMatrices().
float4x4 mProj : register(vs, c8);    // Upload manually (View::m_mProj).
float4x4 mFull : register(vs, c12);   // Manual or uploaded by View::UploadMatrices().
float4x4 mCustom : register(vs, c16); // Upload manually.

// world space position and RGB color of single directional light (SS_LIT3D)
float3 vStockLightPos : register(vs, c20);
float3 vStockLightColor : register(vs, c21);

// offset values to compensate for Direct3D 9 texel alignment artifact
// automatically uploaded by Renderer::SetRenderTarget()
float2 vTexelAlignD3D9 : register(vs, c22);

// compensate 2D position for Direct3D 9 texel alignment artifact
const float4 TexelCorrectPosition2D(const float4 position)
{
	float4 correctedPos = position;
	correctedPos.xy += vTexelAlignD3D9;
	return correctedPos;
}

// first user constant index
#define kFirstUserReg c23
