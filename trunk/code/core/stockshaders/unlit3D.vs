
// tpbds -- stock shader: unlit 3D

#include "../stdvs.inc"

struct VS_INPUT
{
	float3 position3 : POSITION;
	float4 diffuse : COLOR0;
	float2 UV : TEXCOORD0;
};

struct VS_OUTPUT 
{
	float4 position : POSITION;
	float4 diffuse : COLOR0;
	float2 UV : TEXCOORD0;
	float1 viewZ : TEXCOORD1;
};

VS_OUTPUT core_ss_unlit3D(in VS_INPUT input)
{
	VS_OUTPUT output;
	output.position = mul(float4(input.position3, 1.f), mFull);;
	output.diffuse = input.diffuse;
	output.UV = input.UV;
	output.viewZ = output.position.w;
	return output;
}
