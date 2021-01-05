// Timeborne/InGame/Model/GameObjects/HeightDependentDistanceParameters.h

#pragma once

struct HeightDependentDistanceParameters
{
	float BaseDistance = 0.0f;
	float HeightDistanceFactor = 0.0f;
	float HeightDistanceMin = 0.0f;
	float HeightDistanceMax = 0.0f;

	inline float GetValue(float sourceHeight, float targetHeight) const
	{
		// If the source is higher, the threshold also gets higher.
		return BaseDistance +
			glm::clamp((sourceHeight - targetHeight) * HeightDistanceFactor,
				HeightDistanceMin, HeightDistanceMax);
	}

	inline float GetMin() const { return BaseDistance + HeightDistanceMin; }
	inline float GetMax() const { return BaseDistance + HeightDistanceMax; }
};
