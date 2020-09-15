
// tpbds -- stock shader: lit 3D

#include "../stdvs.inc"

float3 VL_CalculateDiffuseTerm(
	float3 position,
	float3 normal,
	float3 lightPos,
	float3 lightColor)
{
	lightPos = mul(lightPos, mCustom);
	float3 lightVec = normalize(lightPos - position);
	float diffuse = max(dot(normal, lightVec), 0);
	float3 diffuseTerm = diffuse * lightColor;
	return diffuseTerm;
}	

struct VS_INPUT
{
	float3 position3 : POSITION;
	float2 UV : TEXCOORD0;
	float3 normal : NORMAL;
};

struct VS_OUTPUT 
{
	float4 position : POSITION;
	float4 color : COLOR0;
	float2 UV : TEXCOORD0;
	float viewZ : TEXCOORD1;
};

VS_OUTPUT core_ss_lit3D(in VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = mul(float4(input.position3, 1), mFull);
	output.color = float4(VL_CalculateDiffuseTerm(input.position3, input.normal, vStockLightPos, vStockLightColor), 1);
	output.UV = input.UV;
	output.viewZ = output.position.w;
	return output;
}
