// Timeborne/InGame/Model/Terrain/TerrainCommon.h

#pragma once

#include <EngineBuildingBlocks/Math/GLM.h>

inline float EvaluateBicubicFunctionValue(const glm::mat4& coeffs, float x, float z)
{
	float x2 = x * x;
	float z2 = z * z;
	glm::vec4 xs(1.0f, x, x2, x * x2);
	glm::vec4 zs(1.0f, z, z2, z * z2);
	return glm::dot(xs * coeffs, zs);
}

inline glm::mat4x3 GetBicubicFunctionDXMatrix(const glm::mat4& coeffs)
{
	glm::mat4x3 m(glm::uninitialize);
	for (int c = 0; c < 4; c++) m[c] = glm::vec3(coeffs[c][1], coeffs[c][2] * 2.0f, coeffs[c][3] * 3.0f);
	return m;
}

inline float EvaluateBicubicFunctionDerivativeX(const glm::mat4x3& dxm, float x, float z)
{
	float z2 = z * z;
	glm::vec3 xs(1.0f, x, x * x);
	glm::vec4 zs(1.0f, z, z2, z * z2);
	return glm::dot(xs * dxm, zs);
}

inline glm::mat3x4 GetBicubicFunctionDZMatrix(const glm::mat4& coeffs)
{
	glm::mat3x4 m(glm::uninitialize);
	for (int c = 0; c < 3; c++) m[c] = coeffs[c + 1] * (c + 1.0f);
	return m;
}

inline float EvaluateBicubicFunctionDerivativeZ(const glm::mat3x4& dzm, float x, float z)
{
	float x2 = x * x;
	glm::vec4 xs(1.0f, x, x2, x * x2);
	glm::vec3 zs(1.0f, z, z * z);
	return glm::dot(xs * dzm, zs);
}