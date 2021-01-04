// TerrainField.hlsl

#include "../HLSL/Structures.hlsl"
#include "RenderPass.hlsl"
#include "TerrainCommon.hlsl"
#include "Line.hlsl"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct VS_Input
{
	uint FieldIndex : TEXCOORD0;
};

VS_Input VSMain(VS_Input input)
{
	return input;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ConstantFunc_Output
{
	float Edges[4]			: SV_TessFactor;
	float Inside[2]			: SV_InsideTessFactor;
	uint2 FieldIndex		: TEXCOORD0;
	float4x4 CoeffMatrix	: TEXCOORD1;
};

ConstantFunc_Output PatchConstantFunc(InputPatch<VS_Input, 1> patches, uint patchId : SV_PrimitiveID)
{
	ConstantFunc_Output output;

#if IS_INGAME != 0
	uint linearFieldIndex = patches[0].FieldIndex;
#else
	uint linearFieldIndex = patchId;
#endif

	uint2 fieldIndex = GetFieldIndex(linearFieldIndex);
	float4x4 coeffMatrix = GetCoeffMatrix(fieldIndex);

	float3 p0 = GetWorldPos(float2(0.0f, 0.0f), fieldIndex, coeffMatrix);
	float3 p1 = GetWorldPos(float2(1.0f, 0.0f), fieldIndex, coeffMatrix);
	float3 p2 = GetWorldPos(float2(1.0f, 1.0f), fieldIndex, coeffMatrix);
	float3 p3 = GetWorldPos(float2(0.0f, 1.0f), fieldIndex, coeffMatrix);

	float edges[4];
	float inside[2];
	GetFieldTessFactors(edges, inside, p0, p1, p2, p3);

	output.Edges = edges;
	output.Inside = inside;
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
	uint2 FieldIndex		: TEXCOORD1;
	float4x4 CoeffMatrix	: TEXCOORD2;
};

[domain("quad")]
DS_Output DSMain(ConstantFunc_Output input, float2 uv : SV_DomainLocation, const OutputPatch<HS_Output, 4> dummyPatch)
{
	DS_Output output;

	float3 worldPos = GetWorldPos(uv, input.FieldIndex, input.CoeffMatrix);

	output.Position = mul(ViewProjectionMatrix, float4(worldPos, 1.0f));
	output.TexCoord = uv;
	output.FieldIndex = input.FieldIndex;
	output.CoeffMatrix = input.CoeffMatrix;

	return output;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

float4 PSMain(DS_Output input) : SV_TARGET
{
	float4x4 coeffMatrix = input.CoeffMatrix;

	const float posEpsilon = 1e-3f;
	float3 p0 = GetWorldPos(input.TexCoord, input.FieldIndex, input.CoeffMatrix);
	float3 p1 = GetWorldPos(input.TexCoord + float2(posEpsilon, 0.0f), input.FieldIndex, input.CoeffMatrix);
	float3 p2 = GetWorldPos(input.TexCoord + float2(0.0f, posEpsilon), input.FieldIndex, input.CoeffMatrix);
	float3 normal = normalize(cross(p2 - p0, p1 - p0));

	float3 lightDir = float3(0.0f, -1.0f, 0.0f);
	float3 baseColor = float3(0.5f, 0.5f, 0.5f);

	float3 toLight = -lightDir;
	float shadeFactor = 0.5f + 0.5f * dot(normal, toLight);
	float3 color = baseColor * shadeFactor;

	// Interpolation of normal between the surface and the geometry normal?
	// For smaller distances the surface normal is smoother, but when the surface is strongly simplified to a few triangles
	// the geometry normal can remove some artifacts. Note that the best solution would be to sample the filtered surface.
	// float3 geometryNormal = normalize(cross(ddy(input.WorldPos), ddx(input.WorldPos)));

#if IS_SHOWING_GRID != 0
	color = lerp(color, float3(0.0f, 0.0f, 0.0f), max(LineStep(p0.x), LineStep(p0.z)));
#endif

	return float4(color, 1.0f);
}