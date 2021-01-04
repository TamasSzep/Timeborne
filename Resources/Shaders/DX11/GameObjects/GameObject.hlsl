// DX11/GameObjects/GameObject.hlsl

#include "../../HLSL/Structures.hlsl"
#include "../RenderPass.hlsl"
#include "../Lighting.hlsl"

////////////////////////////////////// DATA /////////////////////////////////////

///// Visible to vertex shader.

cbuffer ObjectCBType : register(b1)
{
	float4x4 ModelMatrix;
	float4x4 InverseMatrix;
	float4x4 MVPMatrix;
	uint ColorIndex;
	float3 _Padding0_ObjectCBType;
	float Alpha;
	float3 _Padding1_ObjectCBType;
};

///// Visible to pixel shader.

// Render pass CB.

cbuffer MaterialCBType : register(b2)
{
	float3 DiffuseColor;
	float _Padding0_MaterialCBType;
	float3 SpecularColor;
	float _Padding1_MaterialCBType;
};

Texture2D DiffuseTexture : register(t0);
SamplerState MySampler : register(s0);

static const float3 c_SpecularColor = float3(1.0f, 1.0f, 1.0f);

static const uint c_MaxColorCount = 8;
static const float3 c_PlayerColors[c_MaxColorCount] = {
	float3(0.0f, 162.0f, 232.0f) / 255.0f,
	float3(237.0f, 28.0f, 36.0f) / 255.0f,
	float3(34.0f, 177.0f, 76.0f) / 255.0f,
	float3(255.0f, 255.0f, 0.0f) / 255.0f,
	float3(255.0f, 0.0f, 255.0f) / 255.0f,
	float3(0.0f, 255.0f, 255.0f) / 255.0f,
	float3(255.0f, 128.0f, 0.0f) / 255.0f,
	float3(0.0f, 64.0f, 0.0f) / 255.0f
};
static const float3 c_PlayerVertexIndicatorColor = float3(205.0f / 255.0f, 0.0f, 205.0f / 255.0f);
static const float c_PlayerColorEpsilon = 0.5f / 255.0f;

///////////////////////////////////// SHADERS /////////////////////////////////////


#if (IS_USING_VERTEX_COLOR == 1)
VertexPos4TextCoordNormalWPosColor VSMain(VertexPos3TextCoordNormalColor input)
#else
VertexPos4TextCoordNormalWPos VSMain(VertexPos3TextCoordNormal input)
#endif
{
#if (IS_USING_VERTEX_COLOR == 1)
	VertexPos4TextCoordNormalWPosColor result;
#else
	VertexPos4TextCoordNormalWPos result;
#endif

	float4 position1 = float4(input.Position, 1.0f);
	float4 normal0 = float4(input.Normal, 0.0f);

	result.Position = mul(MVPMatrix, position1);
	result.TexCoord = input.TexCoord;
	result.Normal = normalize(mul(normal0, InverseMatrix).xyz);
	result.WorldPosition = mul(ModelMatrix, position1).xyz;

#if (IS_USING_VERTEX_COLOR == 1)
	result.VertexColor = input.VertexColor;

	// Replacing player colors.
	if (length(input.VertexColor.xyz - c_PlayerVertexIndicatorColor) < c_PlayerColorEpsilon
		&& ColorIndex < c_MaxColorCount)
	{
		result.VertexColor.xyz = c_PlayerColors[ColorIndex];
	}

#endif

	return result;
}

#if (IS_USING_VERTEX_COLOR == 1)
float4 PSMain(VertexPos4TextCoordNormalWPosColor input) : SV_TARGET
#else
float4 PSMain(VertexPos4TextCoordNormalWPos input) : SV_TARGET
#endif
{
	float3 normal = normalize(input.Normal);

	float4 diffuseColor4 = DiffuseTexture.Sample(MySampler, input.TexCoord);
	float3 diffuseColor = diffuseColor4.xyz;

	float3 specularColor;
#if (IS_USING_MATERIAL_COLORS == 1)
	diffuseColor *= DiffuseColor;
	specularColor = SpecularColor;
#else
	specularColor = c_SpecularColor;
#endif

#if (IS_USING_VERTEX_COLOR == 1)
	diffuseColor *= input.VertexColor;
	specularColor *= input.VertexColor;
#endif

	float opacity = diffuseColor4.w * Alpha;

	float3 color = EvaluatePhong(normal, input.WorldPosition, CameraPosition, diffuseColor, specularColor);

	return float4(color, opacity);
}