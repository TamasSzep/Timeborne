// Line.hlsl

static const float LineStepFactor = 1.5f;
static const float LineThickness = 0.8f;
static const float MaxLineWidth = 0.05f;

float3 GetValueAndDerivatives(float value)
{
	return float3(value, ddx(value), ddy(value));
}

float SubpixelValue(float3 value3, float2 pixelOffset)
{
	return value3.x + value3.y * pixelOffset.x + value3.z * pixelOffset.y;
}

float LineStepAt(float3 x3, float2 pixelOffset, float lineWidth)
{
	float halfLineWidth = lineWidth * 0.5f;
	float halfStep = halfLineWidth * LineStepFactor;

	float x = SubpixelValue(x3, pixelOffset);
	return smoothstep(-halfStep, 0.0f, halfLineWidth - abs(round(x) - x));
}

float LineStep(float x)
{
	// Standard 16-sample multisample pattern.
	static const float2 Sample00 = float2(0.0625f, 0.0625f);   // (+1, +1)
	static const float2 Sample01 = float2(-0.0625f, -0.1875f); // (-1, -3)
	static const float2 Sample02 = float2(-0.1875f, 0.125f);   // (-3, +2)
	static const float2 Sample03 = float2(0.25f, -0.0625f);    // (+4, -1)
	static const float2 Sample04 = float2(-0.3125f, -0.125f);  // (-5, -2)
	static const float2 Sample05 = float2(0.125f, 0.3125f);    // (+2, +5)
	static const float2 Sample06 = float2(0.3125f, 0.1875f);   // (+5, +3)
	static const float2 Sample07 = float2(0.1875f, -0.3125f);  // (+3, -5)
	static const float2 Sample08 = float2(-0.125f, 0.375f);    // (-2, +6)
	static const float2 Sample09 = float2(0.0f, -0.4375f);     // (+0, -7)
	static const float2 Sample10 = float2(-0.25f, -0.375f);    // (-4, -6)
	static const float2 Sample11 = float2(-0.375f, 0.25f);     // (-6, +4)
	static const float2 Sample12 = float2(-0.5f, 0.0f);        // (-8, +0)
	static const float2 Sample13 = float2(0.4375f, -0.25f);    // (+7, -4)
	static const float2 Sample14 = float2(0.375f, 0.4375f);    // (+6, +7)
	static const float2 Sample15 = float2(-0.4375f, -0.5f);    // (-7, -8)

	float3 x3 = GetValueAndDerivatives(x);
	float lineWidth = min(length(x3.yz) * LineThickness, MaxLineWidth);

	return 0.0625f * (LineStepAt(x3, Sample00, lineWidth)
		+ LineStepAt(x3, Sample01, lineWidth)
		+ LineStepAt(x3, Sample02, lineWidth)
		+ LineStepAt(x3, Sample03, lineWidth)
		+ LineStepAt(x3, Sample04, lineWidth)
		+ LineStepAt(x3, Sample05, lineWidth)
		+ LineStepAt(x3, Sample06, lineWidth)
		+ LineStepAt(x3, Sample07, lineWidth)
		+ LineStepAt(x3, Sample08, lineWidth)
		+ LineStepAt(x3, Sample09, lineWidth)
		+ LineStepAt(x3, Sample10, lineWidth)
		+ LineStepAt(x3, Sample11, lineWidth)
		+ LineStepAt(x3, Sample12, lineWidth)
		+ LineStepAt(x3, Sample13, lineWidth)
		+ LineStepAt(x3, Sample14, lineWidth)
		+ LineStepAt(x3, Sample15, lineWidth));
}