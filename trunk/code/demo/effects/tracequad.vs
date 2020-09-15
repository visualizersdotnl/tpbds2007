
// For screen-space tracers (copy of SS_SPRITE2D).

#include "../../core/stdvs.inc"

struct VS_INPUT
{
	float3 position3 : POSITION;
	float2 UV : TEXCOORD0;
};

struct VS_OUTPUT 
{
	float4 position : POSITION;
	float2 UV : TEXCOORD0;
};

VS_OUTPUT main(in VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = TexelCorrectPosition2D(float4(input.position3, 1));
	output.UV = input.UV;
	return output;
}
