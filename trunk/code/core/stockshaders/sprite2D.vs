
// tpbds -- stock shader: 2D

#include "../stdvs.inc"

struct VS_INPUT
{
	float3 position3 : POSITION;
	float4 color : COLOR0;
	float2 UV : TEXCOORD0;
};

struct VS_OUTPUT 
{
	float4 position : POSITION;
	float4 color : COLOR0;
	float2 UV : TEXCOORD0;
};

VS_OUTPUT core_ss_sprite2D(in VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = TexelCorrectPosition2D(float4(input.position3, 1));
	output.color = input.color;
	output.UV = input.UV;
	return output;
}
