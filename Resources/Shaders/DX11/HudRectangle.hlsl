// DX11/HudRectangle.hlsl

#include "../HLSL/Structures.hlsl"

struct RectangleData
{
	float2 PositionInCs : TEXCOORD1;
	float2 SizeInCs     : TEXCOORD2;
	float4 Color        : TEXCOORD3;
};

struct OutVertexData
{
	float4 Position	   : SV_POSITION;
	float4 VertexColor : TEXCOORD0;
};

OutVertexData VSMain(VertexPos3TextCoordNormal vertex, RectangleData instance)
{
	OutVertexData output = (OutVertexData)0;

	float2 position = instance.PositionInCs + vertex.Position.xy * instance.SizeInCs * 0.5;

	output.Position    = float4(position, 0.01f, 1.0f);
	output.VertexColor = instance.Color;

	return output;
}

float4 PSMain(OutVertexData input) : SV_TARGET
{
	return input.VertexColor;
}
