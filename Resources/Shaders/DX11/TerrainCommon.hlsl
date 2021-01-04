// TerrainCommon.hlsl

cbuffer CBType : register(b1)
{
	uint FieldIndexShift;
	uint FieldIndexMask;
	float4 FieldIndexToTexCoord;
	float ExplicitTessellationFactor;
};

Texture2D<float4> CoeffTex0 : register(t0);
Texture2D<float4> CoeffTex1 : register(t1);
Texture2D<float4> CoeffTex2 : register(t2);
Texture2D<float4> CoeffTex3 : register(t3);

SamplerState CoeffTexSampler : register(s0);

static const uint c_InvalidIndex = 4294967295;

uint2 GetFieldIndex(uint fieldId)
{
	return uint2(fieldId & FieldIndexMask, fieldId >> FieldIndexShift);
}

float4x4 GetCoeffMatrix(uint2 fieldIndex)
{
	float2 fieldIndexF = float2(fieldIndex);
	float2 uv = float2(
		FieldIndexToTexCoord.x * fieldIndexF.x + FieldIndexToTexCoord.y,
		FieldIndexToTexCoord.z * fieldIndexF.y + FieldIndexToTexCoord.w);
	float4x4 m =
	{
		CoeffTex0.SampleLevel(CoeffTexSampler, uv, 0.0f),
		CoeffTex1.SampleLevel(CoeffTexSampler, uv, 0.0f),
		CoeffTex2.SampleLevel(CoeffTexSampler, uv, 0.0f),
		CoeffTex3.SampleLevel(CoeffTexSampler, uv, 0.0f)
	};
	return m;
}

float GetHeight(float2 texCoord, float4x4 coeffMatrix)
{
	float x = texCoord.x;
	float y = texCoord.y;
	float x2 = x * x;
	float y2 = y * y;
	float4 xs = float4(1.0f, x, x2, x * x2);
	float4 ys = float4(1.0f, y, y2, y * y2);
	return dot(mul(xs, coeffMatrix), ys);
}

float3 GetWorldPos(float2 texCoord, uint2 fieldIndex, float4x4 coeffMatrix)
{
	float2 pos2 = (float2(fieldIndex) + texCoord);
	return float3(pos2.x, GetHeight(texCoord, coeffMatrix), pos2.y);
}

float GetEdgeTessFactor(float diff)
{
	const float minTessDistance = 1.0f;
	const float maxTessDistance = 50.0f;

	const float minTessExp = 0.0f;
	const float maxTessExp = 6.0f;

	float x = clamp((diff - minTessDistance) / (maxTessDistance - minTessDistance), 0.0f, 1.0f);
	return pow(2.0f, maxTessExp - (maxTessExp - minTessExp) * x);
}

// For 0x height ratio (flat): 1x, for 5x height ratio: 2x, for 10x height ratio: 4x
static const float c_HeightRatioMultiplier = 0.2f;
static const float c_MaxTessellationFactor = 64.0f;

float GetHeightFactor(float3 p0, float3 p1)
{
	return pow(2.0f, abs(p1.y - p0.y) * c_HeightRatioMultiplier);
}

void GetFieldTessFactors(out float edges[4], out float inside[2], float3 p0, float3 p1, float3 p2, float3 p3)
{
#if IS_INGAME != 0

	edges[0] = min(ExplicitTessellationFactor * GetHeightFactor(p3, p0), c_MaxTessellationFactor);
	edges[1] = min(ExplicitTessellationFactor * GetHeightFactor(p0, p1), c_MaxTessellationFactor);
	edges[2] = min(ExplicitTessellationFactor * GetHeightFactor(p1, p2), c_MaxTessellationFactor);
	edges[3] = min(ExplicitTessellationFactor * GetHeightFactor(p2, p3), c_MaxTessellationFactor);

#else

	float diff0 = length(CameraPosition - p0);
	float diff1 = length(CameraPosition - p1);
	float diff2 = length(CameraPosition - p2);
	float diff3 = length(CameraPosition - p3);

	edges[0] = GetEdgeTessFactor(min(diff3, diff0));
	edges[1] = GetEdgeTessFactor(min(diff0, diff1));
	edges[2] = GetEdgeTessFactor(min(diff1, diff2));
	edges[3] = GetEdgeTessFactor(min(diff2, diff3));

#endif

	float insideTessFactor = max(max(max(edges[0], edges[1]), edges[2]), edges[3]);

	inside[0] = insideTessFactor;
	inside[1] = insideTessFactor;
}