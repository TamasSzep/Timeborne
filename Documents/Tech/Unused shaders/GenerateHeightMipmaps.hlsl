// DX11/GenerateHeightMipmaps.hlsl

Texture2D<float> TexIn : register(t0);
RWTexture2D<float> TexOut : register(u0);

cbuffer CB_Type : register(b0)
{
	uint2 SourceLastElement;
};

float GetValue(uint2 index)
{
	return TexIn.Load(int3(min(index, SourceLastElement), 0));
}

[numthreads(8, 4, 1)]
void Main(uint3 globalID : SV_DispatchThreadID)
{
	int2 targetID = int2(globalID.xy);
	int2 sourceID = targetID << 1;

	TexOut[targetID]
		= (TexIn.Load(int3(sourceID, 0)) // This element cannot be out of bounds.
		+ GetValue(sourceID + int2(0, 1))
		+ GetValue(sourceID + int2(1, 0))
		+ GetValue(sourceID + int2(1, 1))) * 0.25f;
}