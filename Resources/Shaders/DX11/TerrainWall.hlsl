// TerrainWall.hlsl

#include "../HLSL/Structures.hlsl"
#include "RenderPass.hlsl"
#include "TerrainCommon.hlsl"
#include "Line.hlsl"

struct VertexInput
{
	uint2 FieldIndices	: TEXCOORD0;
	uint IsXWall		: TEXCOORD1;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

VertexInput VSMain(VertexInput input)
{
	return input;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ConstantFunc_Output
{
	float Edges[4]			: SV_TessFactor;
	float Inside[2]			: SV_InsideTessFactor;
	uint IsXWall			: TEXCOORD0;
	uint4 FieldIndices		: TEXCOORD1;
	float4x4 CoeffMatrix0	: TEXCOORD2;
	float4x4 CoeffMatrix1	: TEXCOORD6;
};

void GetFieldIndexAndCoeffMatrix(uint fieldIndex1d, out uint2 fieldIndex, out float4x4 coeffMatrix)
{
	if (fieldIndex1d == c_InvalidIndex)
	{
		fieldIndex = uint2(c_InvalidIndex, c_InvalidIndex);
		coeffMatrix = (float4x4)0.0f;
	}
	else
	{
		fieldIndex = GetFieldIndex(fieldIndex1d);
		coeffMatrix = GetCoeffMatrix(fieldIndex);
	}
}

ConstantFunc_Output PatchConstantFunc(InputPatch<VertexInput, 1> input)
{
	ConstantFunc_Output output;

	uint2 fieldIndex0, fieldIndex1;
	float4x4 coeffMatrix0, coeffMatrix1;
	GetFieldIndexAndCoeffMatrix(input[0].FieldIndices.x, fieldIndex0, coeffMatrix0);
	GetFieldIndexAndCoeffMatrix(input[0].FieldIndices.y, fieldIndex1, coeffMatrix1);

	float2 c0, c1, c2, c3;
	if (input[0].IsXWall != 0) // X wall.
	{
		c0 = float2(0.0f, 1.0f);
		c1 = float2(1.0f, 1.0f);
		c2 = float2(1.0f, 0.0f);
		c3 = float2(0.0f, 0.0f);
	}
	else // Z wall.
	{
		c0 = float2(1.0f, 1.0f);
		c1 = float2(1.0f, 0.0f);
		c2 = float2(0.0f, 0.0f);
		c3 = float2(0.0f, 1.0f);
	}

	float3 p0, p1, p2, p3;
	if (fieldIndex0.x != c_InvalidIndex)
	{
		p0 = GetWorldPos(c0, fieldIndex0, coeffMatrix0);
		p1 = GetWorldPos(c1, fieldIndex0, coeffMatrix0);
	}
	if (fieldIndex1.x != c_InvalidIndex)
	{
		p2 = GetWorldPos(c2, fieldIndex1, coeffMatrix1);
		p3 = GetWorldPos(c3, fieldIndex1, coeffMatrix1);
	}

	if (fieldIndex0.x == c_InvalidIndex)
	{
		p0 = float3(p3.x, 0.0f, p3.z);
		p1 = float3(p2.x, 0.0f, p2.z);
	}
	else if (fieldIndex1.x == c_InvalidIndex)
	{
		p3 = float3(p0.x, 0.0f, p0.z);
		p2 = float3(p1.x, 0.0f, p1.z);
	}

	float edges[4];
	float inside[2];
	GetFieldTessFactors(edges, inside, p0, p1, p2, p3);

	output.Edges[0] = 1.0f;
	output.Edges[1] = edges[1];
	output.Edges[2] = 1.0f;
	output.Edges[3] = edges[3];

	// Inside tessellation factor cannot be just one because of the triangle-shaped convex starting slopes.
	float insideTessFactor = max(output.Edges[1], output.Edges[3]);

	output.Inside[0] = insideTessFactor;
	output.Inside[1] = 1.0f;

	output.IsXWall = input[0].IsXWall;
	output.FieldIndices.xy = fieldIndex0;
	output.FieldIndices.zw = fieldIndex1;
	output.CoeffMatrix0    = coeffMatrix0;
	output.CoeffMatrix1    = coeffMatrix1;

	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct HS_Output
{
};

[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("PatchConstantFunc")]
HS_Output HSMain()
{
	return (HS_Output)0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct DS_Output
{
	float4 Position	: SV_POSITION;
#if IS_SHOWING_GRID != 0
	float3 WorldPosition : TEXCOORD0;
	float IsXWall : TEXCOORD1;
#endif
};

[domain("quad")]
DS_Output DSMain(ConstantFunc_Output input, float2 uv : SV_DomainLocation, const OutputPatch<HS_Output, 4> dummyPatch)
{
	DS_Output output;

	uint2 fieldIndex0 = input.FieldIndices.xy;
	uint2 fieldIndex1 = input.FieldIndices.zw;

	float2 pos2;
	float2 c0, c1;
	if (input.IsXWall != 0) // X wall.
	{
		pos2 = float2(uv.x, 0.0f);
		if (fieldIndex1.x == c_InvalidIndex) pos2 += float2(fieldIndex0) + float2(0.0f, 1.0f);
		else pos2 += float2(fieldIndex1);
		c0 = float2(uv.x, 1.0f);
		c1 = float2(uv.x, 0.0f);
	}
	else // Z wall.
	{
		pos2 = float2(0.0f, 1.0f - uv.x);
		if (fieldIndex1.x == c_InvalidIndex) pos2 += float2(fieldIndex0) + float2(1.0f, 0.0f);
		else pos2 += float2(fieldIndex1);
		c0 = float2(1.0f, 1.0f - uv.x);
		c1 = float2(0.0f, 1.0f - uv.x);
	}

	float height0 = GetHeight(c0, input.CoeffMatrix0);
	float height1 = GetHeight(c1, input.CoeffMatrix1);
	float3 worldPos = float3(pos2.x, lerp(height0, height1, uv.y), pos2.y);

	output.Position = mul(ViewProjectionMatrix, float4(worldPos, 1.0f));

#if IS_SHOWING_GRID != 0
	output.WorldPosition = worldPos;
	output.IsXWall = float(input.IsXWall);
#endif

	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

float4 PSMain(DS_Output input) : SV_TARGET
{
	float3 color = float3(0.3f, 0.15f, 0.0f);
#if IS_SHOWING_GRID != 0
	float stepFactor1 = LineStep(input.WorldPosition.y);
	float stepFactor2 = LineStep((input.IsXWall > 0.5f) ? input.WorldPosition.x : input.WorldPosition.z);
	color = lerp(color, float3(0.0f, 0.0f, 0.0f), max(stepFactor1, stepFactor2));
#endif
	return float4(color, 1.0);
}