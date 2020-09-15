
// tpbds -- oldschool metaball shader

#include "../../core/stdvs.inc"

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
	float3 normal : NORMAL;
};

struct VS_OUTPUT 
{
	float4 position : POSITION;
	float4 color : COLOR0;
	float2 UV : TEXCOORD0;
};

VS_OUTPUT utility_ss_blobs(in VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = mul(float4(input.position3, 1), mFull);
	output.color = 0.25 + float4(VL_CalculateDiffuseTerm(input.position3, input.normal, vStockLightPos, vStockLightColor), 1);
	float3 rotNormal = mul(input.normal, mWorld);
	output.UV = rotNormal.xy * 0.5 + 0.5;
	return output;
}
