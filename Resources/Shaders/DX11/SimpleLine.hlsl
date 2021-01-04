// DX11/SimpleLine.hlsl

#include "../HLSL/Structures.hlsl"
#include "RenderPass.hlsl"

///////////////////////////////////// SHADERS /////////////////////////////////////

VertexPos4Color3 VSMain(VertexPos3Color3 input)
{
	VertexPos4Color3 result;

	result.Position = mul(ViewProjectionMatrix, float4(input.Position, 1.0f));
	result.VertexColor = input.VertexColor;

	return result;
}

float4 PSMain(VertexPos4Color3 input) : SV_TARGET
{
	return float4(input.VertexColor, 1.0f);
}