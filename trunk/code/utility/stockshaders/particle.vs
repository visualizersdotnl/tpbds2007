
// tpbds -- simple particle shader

#include "../../core/stdvs.inc"

struct VS_INPUT
{
	float3 position3 : POSITION;
	float intensity : PSIZE;
	float2 UV : TEXCOORD0;
};

struct VS_OUTPUT 
{
	float4 position : POSITION;
	float4 color : COLOR0;
	float2 UV : TEXCOORD0;
};

VS_OUTPUT utility_ss_particle(in VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = mul(float4(input.position3, 1), mCustom);
	output.color = input.intensity;
	output.UV = input.UV;
	return output;
}
