// Timeborne/Math/Math.h

#pragma once

constexpr float PI = 3.1415926535897932384626433832795f;
constexpr float TWO_PI = 6.283185307179586476925286766559f;

inline constexpr float DegreesToRadiaans(float degrees)
{
	constexpr float DEGREES_TO_RADIANS = 0.01745329251994329576923690768489f;

	return degrees * DEGREES_TO_RADIANS;
}
