// DX11/LevelEditor/BlockTool.hlsl

#include "../../HLSL/Structures.hlsl"
#include "../RenderPass.hlsl"
#include "../TerrainCommon.hlsl"

cbuffer MarkerCB : register(b2)
{
	uint2 Start;
	uint2 End;
	uint CornerIndex;
	float4 Color;
};

struct VS_DummyInput
{
	float Dummy : TEXCOORD0;
};

struct VS_Output
{
};

VS_Output VSMain()
{
	return (VS_Output)0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ConstantFunc_Output
{
	float Edges[4]			: SV_TessFactor;
	float Inside[2]			: SV_InsideTessFactor;
	uint2 FieldIndex		: TEXCOORD1;
	float4x4 CoeffMatrix	: TEXCOORD2;
};

ConstantFunc_Output PatchConstantFunc(uint patchId : SV_PrimitiveID)
{
	ConstantFunc_Output output;

	uint xSize = End.x - Start.x + 1;
	uint fiY = patchId / xSize;
	uint fiX = patchId - fiY * xSize;
	uint2 fieldIndex = uint2(fiX + Start.x, fiY + Start.y);

	float4x4 coeffMatrix = GetCoeffMatrix(fieldIndex);

	float3 p0 = GetWorldPos(float2(0.0f, 0.0f), fieldIndex, coeffMatrix);
	float3 p1 = GetWorldPos(float2(1.0f, 0.0f), fieldIndex, coeffMatrix);
	float3 p2 = GetWorldPos(float2(1.0f, 1.0f), fieldIndex, coeffMatrix);
	float3 p3 = GetWorldPos(float2(0.0f, 1.0f), fieldIndex, coeffMatrix);

	float diff0 = length(CameraPosition - p0);
	float diff1 = length(CameraPosition - p1);
	float diff2 = length(CameraPosition - p2);
	float diff3 = length(CameraPosition - p3);

	output.Edges[0] = GetEdgeTessFactor(min(diff3, diff0));
	output.Edges[1] = GetEdgeTessFactor(min(diff0, diff1));
	output.Edges[2] = GetEdgeTessFactor(min(diff1, diff2));
	output.Edges[3] = GetEdgeTessFactor(min(diff2, diff3));

	float insideTessFactor = max(max(max(output.Edges[0], output.Edges[1]), output.Edges[2]), output.Edges[3]);

	output.Inside[0] = insideTessFactor;
	output.Inside[1] = insideTessFactor;

	output.FieldIndex = fieldIndex;
	output.CoeffMatrix = coeffMatrix;

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
	float4 Position			: SV_POSITION;
	float2 TexCoord			: TEXCOORD0;
	float2 BlockTC			: TEXCOORD1;
};

float2 GetBlockTC(uint2 fieldIndex, float2 texCoord)
{
	return (float2(fieldIndex) + texCoord - Start) / (End - Start + 1.0f);
}

[domain("quad")]
DS_Output DSMain(ConstantFunc_Output input, float2 uv : SV_DomainLocation, const OutputPatch<HS_Output, 4> dummyPatch)
{
	DS_Output output;

	float3 worldPos = GetWorldPos(uv, input.FieldIndex, input.CoeffMatrix);

	output.Position = mul(ViewProjectionMatrix, float4(worldPos, 1.0f));
	output.TexCoord = uv;
	output.BlockTC = GetBlockTC(input.FieldIndex, uv);

	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const float LineThickness = 0.05f;
static const float StartHL = 0.5f;

float GetLineHighLight(float x, float lineThickness, float startHL)
{
	if (x <= 0.5f) return smoothstep(0.0f, lineThickness, lineThickness - x) * (1.0f - startHL);
	else return smoothstep(1.0f - lineThickness, 1.0f, x) * (1.0f - startHL);
}

float GetEdgeShadeFactor(float2 blockTC)
{
	uint2 countBlocks = End - Start + 1;	
	float edgeFactor = max(
		GetLineHighLight(blockTC.x, 2.0f * LineThickness / float(countBlocks.x), 0.0f),
		GetLineHighLight(blockTC.y, 2.0f * LineThickness / float(countBlocks.y), 0.0f));
	return edgeFactor;
}

float GetCornerFactor(float x, float lineThickness)
{
	return smoothstep(0.0f, lineThickness, x);
}

float4 PSMain(DS_Output input) : SV_TARGET
{
	float x = input.TexCoord.x;
	float z = input.TexCoord.y;

	float edgeHL = StartHL + max(GetLineHighLight(x, LineThickness, StartHL), GetLineHighLight(z, LineThickness, StartHL));
	float3 hColor = Color.rgb * edgeHL;
	float3 resColor = lerp(hColor, float3(1.0f, 1.0f, 1.0f), GetEdgeShadeFactor(input.BlockTC));

	float cornerFactor = 0.0f;
	if (CornerIndex < 4)
	{
		if (CornerIndex == 0) cornerFactor = GetCornerFactor(z - x - 0.5f, LineThickness);
		else if (CornerIndex == 1) cornerFactor = GetCornerFactor(x + z - 1.5f, LineThickness);
		else if (CornerIndex == 2) cornerFactor = GetCornerFactor(x - z - 0.5f, LineThickness);
		else if (CornerIndex == 3) cornerFactor = GetCornerFactor(0.5f - (x + z), LineThickness);
		resColor = lerp(resColor, float3(1.0f, 1.0f, 1.0f), cornerFactor);
	}

	return float4(resColor, lerp(Color.a, 1.0f, cornerFactor));
}