// Timeborne/InGame/Model/Terrain/FieldHeightQuadTree.cpp

#include <Timeborne/InGame/Model/Terrain/FieldHeightQuadTree.h>

#include <Core/Constants.h>
#include <Core/SimpleBinarySerialization.hpp>
#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>
#include <EngineBuildingBlocks/Math/Intersection.h>
#include <EngineBuildingBlocks/Math/IntervalArithmetic.h>

#include <Timeborne/InGame/Model/Terrain/Terrain.h>
#include <Timeborne/InGame/Model/Terrain/TerrainCommon.h>
#include <Timeborne/InputHandling.h>

using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Math;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline float GetConstantFunctionValue(const glm::mat4& coeffs)
{
	return coeffs[0][0];
}

inline float EvaluateBilinearFunctionValue(const glm::mat4& coeffs, float x, float z)
{
	return coeffs[0][0] + coeffs[0][1] * x + (coeffs[1][0] + coeffs[1][1] * x) * z;
}

inline bool IsFunctionConstant(const glm::mat4& coeffs)
{
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			if (std::fabsf(coeffs[i][j]) > 1e-5f && (i > 0 || j > 0)) return false;
	return true;
}

inline bool IsFunctionBilinear(const glm::mat4& coeffs)
{
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			if (std::fabsf(coeffs[i][j]) > 1e-5f && (i > 1 || j > 1)) return false;
	return true;
}

inline glm::vec2 GetConstantFunctionMinMax(const glm::mat4& coeffs)
{
	auto c = GetConstantFunctionValue(coeffs);
	return glm::vec2(c, c);
}

inline glm::vec2 GetBilinearFunctionMinMax(const glm::mat4& coeffs)
{
	float v0 = coeffs[1][0];
	float v1 = coeffs[0][1] + coeffs[1][0] + coeffs[1][1];
	float v2 = coeffs[0][1];
	const float v3 = 0.0f;
	return glm::vec2(
		std::min(std::min(std::min(v0, v1), v2), v3),
		std::max(std::max(std::max(v0, v1), v2), v3)) + coeffs[0][0];
}

inline glm::mat4x2 GetBicubicFunctionDXXMatrix(const glm::mat4& coeffs)
{
	glm::mat4x2 m(glm::uninitialize);
	for (int c = 0; c < 4; c++) m[c] = glm::vec2(coeffs[c][2] * 2.0f, coeffs[c][3] * 6.0f);
	return m;
}

inline float EvaluateBicubicFunctionDerivativeXX(const glm::mat4x2& dxxm, float x, float z)
{
	float z2 = z * z;
	glm::vec2 xs(1.0f, x);
	glm::vec4 zs(1.0f, z, z2, z * z2);
	return glm::dot(xs * dxxm, zs);
}

inline glm::mat2x4 GetBicubicFunctionDZZMatrix(const glm::mat4& coeffs)
{
	glm::mat2x4 m(glm::uninitialize);
	m[0] = coeffs[2] * 2.0f;
	m[1] = coeffs[3] * 6.0f;
	return m;
}

inline float EvaluateBicubicFunctionDerivativeZZ(const glm::mat2x4& dzzm, float x, float z)
{
	float x2 = x * x;
	glm::vec4 xs(1.0f, x, x2, x * x2);
	glm::vec2 zs(1.0f, z);
	return glm::dot(xs * dzzm, zs);
}

inline glm::mat3 GetBicubicFunctionDXZMatrix(const glm::mat4& coeffs)
{
	glm::mat3 m(glm::uninitialize);
	for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) m[i][j] = coeffs[i + 1][j + 1] * i * j;
	return m;
}

inline float EvaluateBicubicFunctionDerivativeXZ(const glm::mat3& dxzm, float x, float z)
{
	glm::vec3 xs(1.0f, x, x * x);
	glm::vec3 zs(1.0f, z, z * z);
	return glm::dot(xs * dxzm, zs);
}

inline glm::vec2 EvaluateBicubicFunctionGradient(const glm::mat4x3& dxm, const glm::mat3x4& dzm, float x, float z)
{
	return {
		EvaluateBicubicFunctionDerivativeX(dxm, x, z),
		EvaluateBicubicFunctionDerivativeZ(dzm, x, z) };
}

inline glm::mat2 EvaluateBicubicFunctionHessianMatrix(const glm::mat4x2& dxxm, const glm::mat2x4& dzzm, const glm::mat3& dxzm,
	float x, float z)
{
	auto dxx = EvaluateBicubicFunctionDerivativeXX(dxxm, x, z);
	auto dzz = EvaluateBicubicFunctionDerivativeZZ(dzzm, x, z);
	auto dxz = EvaluateBicubicFunctionDerivativeXZ(dxzm, x, z);
	return { dxx, dxz, dxz, dzz };
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline float GetCubicFunctionValue3(float a, float b, float c, float x)
{
	return ((a * x + b) * x + c) * x;
}

// Returns the minimum and the maximum of the function: a * x^3 + b * x^2 + c * x + const on the [0, 1] interval.
inline glm::vec2 GetCubicFunctionMinimumAndMaximumArg(float a, float b, float c)
{
	struct SortS { float X, Y; bool operator<(const SortS& other) const { NumericalLessCompareBlock(Y); return X < other.X; } };
	SortS points[4];
	points[0] = { 0.0f, 0.0f };
	points[1] = { 1.0f, a + b + c };
	int countPoints = 2;

	float qb = 2.0f * b;

	if (std::fabsf(a) < 1e-5f)
	{
		if (std::fabsf(b) >= 1e-5f)
		{
			float x = -c / qb;
			if (x >= 0.0f && x <= 1.0f) points[countPoints++] = { x, GetCubicFunctionValue3(a, b, c, x) };
		}
	}
	else
	{
		float qa = 3.0f * a;
		float d = qb * qb - 4.0f * qa * c;
		if (d >= 0.0f)
		{
			float dr = std::sqrtf(d);
			float div = 0.5f / qa;
			float x0 = (-qb + dr) * div;
			float x1 = (-qb - dr) * div;
			if (x0 >= 0.0f && x0 <= 1.0f) points[countPoints++] = { x0, GetCubicFunctionValue3(a, b, c, x0) };
			if (x1 >= 0.0f && x1 <= 1.0f) points[countPoints++] = { x1, GetCubicFunctionValue3(a, b, c, x1) };
		}
	}

	std::sort(points, points + countPoints);

	return { points[0].X, points[countPoints - 1].X };
}

inline void LimitStep(float x, float step, float* pT)
{
	if (x + step < 0.0f)* pT = std::min(*pT, -x / step);
	else if (x + step > 1.0f)* pT = std::min(*pT, (1.0f - x) / step);
	assert(*pT >= 0.0f);
}

inline glm::vec2 GetLocalExtremaArg(bool isMaximum, const glm::vec2& start, const glm::mat4& coeffs, const glm::mat4x3& dxm, const glm::mat3x4& dzm,
	const glm::mat4x2& dxxm, const glm::mat2x4& dzzm, const glm::mat3& dxzm)
{
	const glm::vec2 limitMin = glm::vec2(0.0f);
	const glm::vec2 limitMax = glm::vec2(1.0f);
	const float gamma = 1.0f;

	auto x = start;

	// We use Newton's method with a modification: we aim to step toward the smaller or greater values (depending on whether we
	// minimize or maximize) and not towards f'(x) = 0. If the nearest stationary point happens to change the function in the desired
	// direction the modification makes no difference, otherwise we step in the opposite direction.
	const int maxIterations = 16;
	for (int i = 0; i < maxIterations; i++)
	{
		auto gradient = EvaluateBicubicFunctionGradient(dxm, dzm, x.x, x.y);
		auto hessian = EvaluateBicubicFunctionHessianMatrix(dxxm, dzzm, dxzm, x.x, x.y);
		auto invHessian = (std::fabsf(glm::determinant(hessian)) < 1e-5f ? glm::mat2() : glm::inverse(hessian));

		auto step = gamma * invHessian * gradient;

		if (glm::length(step) < 1e-6f) break;

		// Computing step sign.
		auto epsStepped = x + glm::normalize(step) * 1e-4f;
		bool newSmaller = (EvaluateBicubicFunctionValue(coeffs, epsStepped.x, epsStepped.y) < EvaluateBicubicFunctionValue(coeffs, x.x, x.y));
		if (newSmaller == isMaximum)
		{
			step *= -1.0f;
		}

		float t = 1.0f;
		LimitStep(x.x, step.x, &t);
		LimitStep(x.y, step.y, &t);
		step *= t;

		if (glm::length(step) < 1e-6f) break;

		x = glm::clamp(x + step, limitMin, limitMax);
	}

	return x;
}

inline glm::vec2 FindFieldMinimumAndMaximum(const glm::mat4& coeffs)
{
	if (IsFunctionConstant(coeffs)) return GetConstantFunctionMinMax(coeffs);
	if (IsFunctionBilinear(coeffs)) return GetBilinearFunctionMinMax(coeffs);

	auto dxm = GetBicubicFunctionDXMatrix(coeffs);
	auto dzm = GetBicubicFunctionDZMatrix(coeffs);
	auto dxxm = GetBicubicFunctionDXXMatrix(coeffs);
	auto dzzm = GetBicubicFunctionDZZMatrix(coeffs);
	auto dxzm = GetBicubicFunctionDXZMatrix(coeffs);

	glm::vec2 mm0 = GetCubicFunctionMinimumAndMaximumArg(coeffs[0][3], coeffs[0][2], coeffs[0][1]); // z = 0
	glm::vec2 mm1 = GetCubicFunctionMinimumAndMaximumArg(coeffs[3][0], coeffs[2][0], coeffs[1][0]); // x = 0
	glm::vec2 mm2 = GetCubicFunctionMinimumAndMaximumArg(
		coeffs[0][3] + coeffs[1][3] + coeffs[2][3] + coeffs[3][3],
		coeffs[0][2] + coeffs[1][2] + coeffs[2][2] + coeffs[3][2],
		coeffs[0][1] + coeffs[1][1] + coeffs[2][1] + coeffs[3][1]); // z = 1
	glm::vec2 mm3 = GetCubicFunctionMinimumAndMaximumArg(
		coeffs[3][0] + coeffs[3][1] + coeffs[3][2] + coeffs[3][3],
		coeffs[2][0] + coeffs[2][1] + coeffs[2][2] + coeffs[2][3],
		coeffs[1][0] + coeffs[1][1] + coeffs[1][2] + coeffs[1][3]); // x = 1

	glm::vec2 mins[5], maxs[5];
	mins[0] = GetLocalExtremaArg(false, glm::vec2(mm0.x, 0.0f), coeffs, dxm, dzm, dxxm, dzzm, dxzm);
	maxs[0] = GetLocalExtremaArg(true, glm::vec2(mm0.y, 0.0f), coeffs, dxm, dzm, dxxm, dzzm, dxzm);
	mins[1] = GetLocalExtremaArg(false, glm::vec2(0.0f, mm1.x), coeffs, dxm, dzm, dxxm, dzzm, dxzm);
	maxs[1] = GetLocalExtremaArg(true, glm::vec2(0.0f, mm1.y), coeffs, dxm, dzm, dxxm, dzzm, dxzm);
	mins[2] = GetLocalExtremaArg(false, glm::vec2(mm2.x, 1.0f), coeffs, dxm, dzm, dxxm, dzzm, dxzm);
	maxs[2] = GetLocalExtremaArg(true, glm::vec2(mm2.y, 1.0f), coeffs, dxm, dzm, dxxm, dzzm, dxzm);
	mins[3] = GetLocalExtremaArg(false, glm::vec2(1.0f, mm3.x), coeffs, dxm, dzm, dxxm, dzzm, dxzm);
	maxs[3] = GetLocalExtremaArg(true, glm::vec2(1.0f, mm3.y), coeffs, dxm, dzm, dxxm, dzzm, dxzm);
	mins[4] = GetLocalExtremaArg(false, glm::vec2(0.5f, 0.5f), coeffs, dxm, dzm, dxxm, dzzm, dxzm);
	maxs[4] = GetLocalExtremaArg(true, glm::vec2(0.5f, 0.5f), coeffs, dxm, dzm, dxxm, dzzm, dxzm);

	struct SortS {
		float X, Y, Z;
		bool operator<(const SortS& other) const { NumericalLessCompareBlock(Y); NumericalLessCompareBlock(X); return Z < other.Z; }
	};
	SortS points[5];
	glm::vec2 result;

	for (int c = 0; c < 5; c++) points[c] = { mins[c].x, EvaluateBicubicFunctionValue(coeffs, mins[c].x, mins[c].y), mins[c].y };
	std::sort(points, points + 5);
	result.x = points[0].Y;

	for (int c = 0; c < 5; c++) points[c] = { maxs[c].x, EvaluateBicubicFunctionValue(coeffs, maxs[c].x, maxs[c].y), maxs[c].y };
	std::sort(points, points + 5);
	result.y = points[4].Y;

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FieldHeightQuadTree::FieldHeightQuadTree(const Terrain* terrain)
	: m_Terrain(terrain)
{
	Node root;
	root.Parent = Core::c_InvalidIndexU;
	for (unsigned i = 0; i < 4; i++) root.Children[i] = Core::c_InvalidIndexU;
	root.MinHeight = root.MaxHeight = 0.0f;
	root.IsDirty = false;
	m_Nodes.Add(root);

	auto countFields = terrain->GetCountFields();
	if (countFields.x > 1 && countFields.y > 1)
	{
		Update(glm::ivec2(0), glm::ivec2(countFields) - glm::ivec2(1));
	}
}

FieldHeightQuadTree::Node& FieldHeightQuadTree::GetRootNode()
{
	return m_Nodes[0];
}

void FieldHeightQuadTree::CreateChildNodes(unsigned nodeIndex, glm::ivec2& start, glm::ivec2& end)
{
	assert(start != end);
	unsigned innerIndices[] = { 0, 1, 2, 3 };
	unsigned countIndices = 4;
	if (start.y == end.y)
	{
		countIndices = 2;
	}
	else if (start.x == end.x)
	{
		innerIndices[1] = 2;
		countIndices = 2;
	}
	for (unsigned c = 0; c < countIndices; c++)
	{
		unsigned childNodeIndex = m_Nodes.Add();

		auto& node = m_Nodes[nodeIndex];
		node.Children[innerIndices[c]] = childNodeIndex;

		auto& childNode = m_Nodes[childNodeIndex];
		childNode.Parent = nodeIndex;
		for (unsigned i = 0; i < 4; i++) childNode.Children[i] = Core::c_InvalidIndexU;
		childNode.MinHeight = node.MinHeight;
		childNode.MaxHeight = node.MaxHeight;
		childNode.IsDirty = false;
	}
}

unsigned FieldHeightQuadTree::PrepareLeaf(int x, int z)
{
	unsigned nodeIndex = 0;
	glm::ivec2 start(0), end(glm::ivec2(m_Terrain->GetCountFields()) - glm::ivec2(1));
	while (start != end)
	{
		auto middle = (start + end) / glm::ivec2(2);

		int childIndex = 0;
		if (x > middle.x) childIndex++;
		if (z > middle.y) childIndex += 2;

		auto pNode = &m_Nodes[nodeIndex];
		if (pNode->Children[childIndex] == Core::c_InvalidIndexU)
		{
			CreateChildNodes(nodeIndex, start, end);
			pNode = &m_Nodes[nodeIndex];
		}

		if (x <= middle.x) end.x = middle.x;
		else start.x = middle.x + 1;
		if (z <= middle.y) end.y = middle.y;
		else start.y = middle.y + 1;

		nodeIndex = pNode->Children[childIndex];
	}

	return nodeIndex;
}

unsigned FieldHeightQuadTree::GetNode(int x, int z, glm::ivec2* pStart, glm::ivec2* pEnd)
{
	unsigned nodeIndex = 0;
	glm::ivec2 start(0), end(glm::ivec2(m_Terrain->GetCountFields()) - glm::ivec2(1));
	while (start != end)
	{
		auto middle = (start + end) / glm::ivec2(2);

		int childIndex = 0;
		if (x > middle.x) childIndex++;
		if (z > middle.y) childIndex += 2;

		auto& node = m_Nodes[nodeIndex];
		if (node.Children[childIndex] == Core::c_InvalidIndexU)
		{
			break;
		}

		if (x <= middle.x) end.x = middle.x;
		else start.x = middle.x + 1;
		if (z <= middle.y) end.y = middle.y;
		else start.y = middle.y + 1;

		nodeIndex = node.Children[childIndex];
	}

	*pStart = start;
	*pEnd = end;

	return nodeIndex;
}

inline void SetChildBounds(int childIndex, glm::ivec2* pStart, glm::ivec2* pEnd)
{
	auto middle = (*pStart + *pEnd) / glm::ivec2(2);
	if (childIndex & 1) pStart->x = middle.x + 1;
	else pEnd->x = middle.x;
	if (childIndex & 2) pStart->y = middle.y + 1;
	else pEnd->y = middle.y;
}

inline void GetChildBounds(const glm::ivec2& start, const glm::ivec2& end, int childIndex,
	glm::ivec2* cStart, glm::ivec2* cEnd)
{
	auto middle = (start + end) / glm::ivec2(2);
	if (childIndex & 1) { cStart->x = middle.x + 1; cEnd->x = end.x; }
	else { cStart->x = start.x; cEnd->x = middle.x; }
	if (childIndex & 2) { cStart->y = middle.y + 1; cEnd->y = end.y; }
	else { cStart->y = start.y; cEnd->y = middle.y; }
}

void FieldHeightQuadTree::Update(const glm::ivec2& changeStart, const glm::ivec2& changeEnd)
{
	auto fields = m_Terrain->GetFields();
	auto countFields = m_Terrain->GetCountFields();
	auto& surfaceCoeffs = m_Terrain->GetSurfaceCoefficients();

	Core::IndexVectorU dirtyIndices;

	int sX2 = std::max(changeStart.x - 2, 0);
	int sY2 = std::max(changeStart.y - 2, 0);
	int eX2 = std::min(changeEnd.x + 2, (int)countFields.x - 1);
	int eY2 = std::min(changeEnd.y + 2, (int)countFields.y - 1);

	// Updating leafs.
	for (int z = sY2; z <= eY2; z++)
	{
		for (int x = sX2; x <= eX2; x++)
		{
			auto& coeffs = surfaceCoeffs[z * countFields.x + x];
			auto& node = m_Nodes[PrepareLeaf(x, z)];

			auto minMaxHeights = FindFieldMinimumAndMaximum(coeffs);
			node.MinHeight = minMaxHeights.x;
			node.MaxHeight = minMaxHeights.y;
			node.IsDirty = false;

			auto& parentNode = m_Nodes[node.Parent];
			if (!parentNode.IsDirty)
			{
				parentNode.IsDirty = true;
				dirtyIndices.PushBack(node.Parent);
			}
		}
	}

	// Updating hierarchy.
	for (unsigned i = 0; i < dirtyIndices.GetSize(); i++)
	{
		auto& node = m_Nodes[dirtyIndices[i]];
		assert(node.IsDirty);

		node.MinHeight = std::numeric_limits<float>::max();
		node.MaxHeight = std::numeric_limits<float>::lowest();
		for (unsigned c = 0; c < 4; c++)
		{
			if (node.Children[c] != Core::c_InvalidIndexU)
			{
				auto& childNode = m_Nodes[node.Children[c]];
				node.MinHeight = std::min(node.MinHeight, childNode.MinHeight);
				node.MaxHeight = std::max(node.MaxHeight, childNode.MaxHeight);
			}
		}
		node.IsDirty = false;

		if (node.Parent != Core::c_InvalidIndexU)
		{
			auto& parentNode = m_Nodes[node.Parent];
			if (!parentNode.IsDirty)
			{
				parentNode.IsDirty = true;
				dirtyIndices.PushBack(node.Parent);
			}
		}

		// Removing unused nodes.
		if (node.MinHeight == node.MaxHeight)
		{
			for (unsigned c = 0; c < 4; c++)
			{
				if (node.Children[c] != Core::c_InvalidIndexU)
				{
					m_Nodes.Remove(node.Children[c]);
					node.Children[c] = Core::c_InvalidIndexU;
				}
			}
		}
	}
}

struct RayIntersectionConstants
{
	glm::vec3 RayOrigin;
	glm::vec3 RayDir;
	glm::uvec2 CountFields;
	const FieldHeightQuadTree::Node* Nodes;
	const glm::mat4* SurfaceCoeffs;
};

inline float GetNodeMaxPlaneIntersection(const FieldHeightQuadTree::Node& node, const glm::ivec2& start, const glm::ivec2& end,
	const RayIntersectionConstants& rc)
{
	return GetRayAABBIntersection_PositiveT(
		glm::vec3(start.x, std::numeric_limits<float>::lowest(), start.y),
		glm::vec3((end.x + 1), node.MaxHeight, (end.y + 1)),
		rc.RayOrigin, rc.RayDir);
}

inline void CheckRayIntersectionBounds(const RayIntersectionConstants& rc, float* pT)
{
	auto fMin = std::numeric_limits<float>::lowest();
	auto fMax = std::numeric_limits<float>::max();

	auto xEnd = rc.CountFields.x;
	auto zEnd = rc.CountFields.y;

	auto t = *pT;
	t = std::min(t, GetRayAABBIntersection_PositiveT(glm::vec3(fMin, fMin, fMin), glm::vec3(fMax, 0.0f, 0.0f), rc.RayOrigin, rc.RayDir));
	t = std::min(t, GetRayAABBIntersection_PositiveT(glm::vec3(fMin, fMin, zEnd), glm::vec3(fMax, 0.0f, fMax), rc.RayOrigin, rc.RayDir));
	t = std::min(t, GetRayAABBIntersection_PositiveT(glm::vec3(fMin, fMin, fMin), glm::vec3(0.0f, 0.0f, fMax), rc.RayOrigin, rc.RayDir));
	t = std::min(t, GetRayAABBIntersection_PositiveT(glm::vec3(xEnd, fMin, fMin), glm::vec3(fMax, 0.0f, fMax), rc.RayOrigin, rc.RayDir));
	*pT = t;
}

inline bool IsIntersectionConsistenWithField(const glm::ivec2& start, const glm::ivec2& end, const RayIntersectionConstants& rc, float t)
{
	auto x = rc.RayOrigin + rc.RayDir * t;
	return (x.x >= start.x && x.x < (end.x + 1)
		&& x.z >= start.y && x.z < (end.y + 1));
}

inline bool IsIntersectionConsistenWithFieldEdgeAllowed(const glm::ivec2& start, const glm::ivec2& end, const RayIntersectionConstants& rc, float t)
{
	auto x = rc.RayOrigin + rc.RayDir * t;
	return (x.x >= start.x && x.x <= (end.x + 1)
		&& x.z >= start.y && x.z <= (end.y + 1));
}

inline void AdjustIntersectionToField(const glm::ivec2& start, const glm::ivec2& end, const RayIntersectionConstants& rc, float* pT)
{
	// This simple algorithm ensures, that the computed world intersection is consistent with the field.
	// Therefore we have to compute it using the ray origin, direction and parameter t.
	float t = *pT;
	if (IsIntersectionConsistenWithField(start, end, rc, t)) return;
	float xzSize = glm::length(glm::vec2(rc.RayDir.x, rc.RayDir.z));
	if (xzSize < 1e-5f) return;
	float stepBase = 1.0f / xzSize;
	float step = stepBase * 1e-6f;
	float stepLimit = stepBase * 1e-3f;
	for (; step <= stepLimit; step *= 1.5f)
	{
		float t0 = t - step;
		float t1 = t + step;
		if (IsIntersectionConsistenWithField(start, end, rc, t0)) { *pT = t0; return; }
		if (IsIntersectionConsistenWithField(start, end, rc, t1)) { *pT = t1; return; }
	}

	// We must have intersected exactly the edge. This is fine, because the intersection search only returns the
	// parameter t.
	assert(IsIntersectionConsistenWithFieldEdgeAllowed(start, end, rc, t));
}

inline bool IsRayGoingUnderSurface(const glm::mat4& coeffs, const glm::vec3& rayOriginL, const glm::vec3& rayDirL, const IntervalF& t)
{
	auto x = rayOriginL.x + rayDirL.x * t;
	auto y = rayOriginL.y + rayDirL.y * t;
	auto z = rayOriginL.z + rayDirL.z * t;
	auto x2 = x * x;
	auto x3 = x * x2;
	auto z2 = z * z;
	auto z3 = z * z2;
	auto value = y
		- (coeffs[0][0] + coeffs[0][1] * x + coeffs[0][2] * x2 + coeffs[0][3] * x3)
		- (coeffs[1][0] + coeffs[1][1] * x + coeffs[1][2] * x2 + coeffs[1][3] * x3) * z
		- (coeffs[2][0] + coeffs[2][1] * x + coeffs[2][2] * x2 + coeffs[2][3] * x3) * z2
		- (coeffs[3][0] + coeffs[3][1] * x + coeffs[3][2] * x2 + coeffs[3][3] * x3) * z3;
	return (value.Min <= 0.0f);
}

inline float GetFieldIntersectionY(const glm::mat4& coeffs, const glm::vec3& rayOriginL, const glm::vec3& rayDirL, const IntervalF& t)
{
	assert(t.Max >= t.Min);
	if (!IsRayGoingUnderSurface(coeffs, rayOriginL, rayDirL, t)) return c_InvalidIntersectionT;
	if (t.Max - t.Min <= 1e-5f) return t.GetMiddle();

	IntervalF first, second;
	t.Split(&first, &second);
	if(first.IsLengthZero() || second.IsLengthZero()) return t.GetMiddle();

	auto res0 = GetFieldIntersectionY(coeffs, rayOriginL, rayDirL, first);
	if (res0 != c_InvalidIntersectionT) return res0;
	return GetFieldIntersectionY(coeffs, rayOriginL, rayDirL, second);
}

inline float GetFieldIntersection(const glm::ivec2& start, const glm::mat4& coeffs, const RayIntersectionConstants& rc)
{
	auto rayOriginL = rc.RayOrigin - glm::vec3(start.x, 0.0f, start.y);
	auto rayDirL = glm::normalize(rc.RayDir);

	// Intersecting with field X-Z interval.
	IntervalF t;
	GetRayAABBIntersection(
		glm::vec3(0.0f, c_FloatMin, 0.0f),
		glm::vec3(1.0f, c_FloatMax, 1.0f),
		rayOriginL, rayDirL, &t.Min, &t.Max);
	if (t.Min == c_InvalidIntersectionT || t.Max < 0.0f) return c_InvalidIntersectionT;
	t.Min = std::max(t.Min, 0.0f);

	// Calling recursive Y-intersection method.
	auto tLocal = GetFieldIntersectionY(coeffs, rayOriginL, rayDirL, t);
	if (tLocal == c_InvalidIntersectionT) return c_InvalidIntersectionT;
	assert(tLocal >= 0.0f);
	return tLocal / glm::length(rc.RayDir);
}

struct TraverseData
{
	glm::ivec2 Start, End;
	unsigned ChildInnerIndices[4];
	unsigned CurrentBranch;
};

inline void CreateTraverseChildData(const FieldHeightQuadTree::Node* nodes,
	const FieldHeightQuadTree::Node& node, TraverseData& traverseData, const RayIntersectionConstants& rc)
{
	constexpr float c_Lowest = std::numeric_limits<float>::lowest();
	struct TSort {
		float T; unsigned Index; bool operator<(const TSort& o) const {
			if (T != o.T) { return T < o.T; } return (Index < o.Index);
		}
	};
	TSort ts[4];
	for (int c = 0; c < 4; c++)
	{
		auto childNodeIndex = node.Children[c];
		if (childNodeIndex == Core::c_InvalidIndexU)
		{
			ts[c].T = c_Lowest;
		}
		else
		{
			glm::ivec2 cStart(glm::uninitialize), cEnd(glm::uninitialize);
			GetChildBounds(traverseData.Start, traverseData.End, c, &cStart, &cEnd);
			auto t = GetNodeMaxPlaneIntersection(nodes[childNodeIndex], cStart, cEnd, rc);
			ts[c].T = (t == c_InvalidIntersectionT ? c_Lowest : t);
		}
		ts[c].Index = c;
	}
	std::sort(ts, ts + 4);
	traverseData.CurrentBranch = 4;
	for (int c = 0; c < 4; c++)
	{
		traverseData.ChildInnerIndices[c] = ts[c].Index;
		if (traverseData.CurrentBranch == 4 && ts[c].T != c_Lowest)
			traverseData.CurrentBranch = c;
	}
}

inline bool BacktrackInRayIntersectionTraversal(Core::SimpleTypeVectorU<TraverseData>& traverseStack, const FieldHeightQuadTree::Node& node,
	unsigned* pNodeIndex, glm::ivec2* pStart, glm::ivec2* pEnd)
{
	// Backtrack.
	traverseStack.PopBack();
	if (traverseStack.IsEmpty()) return false;
	auto& traverseData = traverseStack.GetLastElement();
	traverseData.CurrentBranch++;
	*pNodeIndex = node.Parent;
	*pStart = traverseData.Start;
	*pEnd = traverseData.End;
	return true;
}

inline void GetTerminalInnerNodeIntersection(const FieldHeightQuadTree::Node& node, const glm::ivec2& start, const glm::ivec2& end,
	const RayIntersectionConstants& rc, float* pT)
{
	assert(node.MinHeight == node.MaxHeight);
	assert(node.Children[0] == Core::c_InvalidIndexU && node.Children[1] == Core::c_InvalidIndexU
		&& node.Children[2] == Core::c_InvalidIndexU && node.Children[3] == Core::c_InvalidIndexU);

	*pT = GetNodeMaxPlaneIntersection(node, start, end, rc);
	AdjustIntersectionToField(start, end, rc, pT);
}

inline float IntersectRayInnerTerrain(const RayIntersectionConstants& rc)
{
	unsigned nodeIndex = 0;
	glm::ivec2 start(0), end(glm::ivec2(rc.CountFields) - glm::ivec2(1));

	auto t = c_InvalidIntersectionT;
	if (GetNodeMaxPlaneIntersection(rc.Nodes[0], start, end, rc) == c_InvalidIntersectionT) return t;

	if (rc.Nodes[0].MinHeight == rc.Nodes[0].MaxHeight)
	{
		GetTerminalInnerNodeIntersection(rc.Nodes[0], start, end, rc, &t);
		return t;
	}

	Core::SimpleTypeVectorU<TraverseData> traverseStack;
	auto& rTraverseData = traverseStack.PushBackPlaceHolder();
	rTraverseData.Start = start;
	rTraverseData.End = end;
	CreateTraverseChildData(rc.Nodes, rc.Nodes[0], rTraverseData, rc);

	assert(start != end);

	// In the loop the current has always an intersection with the ray.
	while (true)
	{
		auto& node = rc.Nodes[nodeIndex];
		auto& traverseData = traverseStack.GetLastElement();

		if (traverseData.CurrentBranch == 4)
		{
			if (!BacktrackInRayIntersectionTraversal(traverseStack, node, &nodeIndex, &start, &end)) break;
		}
		else
		{
			// Forward track.
			auto childInnerIndex = traverseData.ChildInnerIndices[traverseData.CurrentBranch];
			nodeIndex = node.Children[childInnerIndex];
			SetChildBounds(childInnerIndex, &start, &end);

			auto& traverseData = traverseStack.PushBackPlaceHolder();
			traverseData.Start = start;
			traverseData.End = end;

			auto& childNode = rc.Nodes[nodeIndex];
			if (start == end)
			{
				auto& coeffs = rc.SurfaceCoeffs[start.y * rc.CountFields.x + start.x];
				t = GetFieldIntersection(start, coeffs, rc);
				if (t != c_InvalidIntersectionT)
				{
					AdjustIntersectionToField(start, end, rc, &t);
					break;
				}

				if (!BacktrackInRayIntersectionTraversal(traverseStack, childNode, &nodeIndex, &start, &end)) break;
			}
			else
			{
				if (childNode.MinHeight == childNode.MaxHeight)
				{
					GetTerminalInnerNodeIntersection(childNode, start, end, rc, &t);
					assert(t != c_InvalidIntersectionT);
					break;
				}
				else
				{
					CreateTraverseChildData(rc.Nodes, childNode, traverseData, rc);
				}
			}
		}
	}
	return t;
}

inline float IntersectRay(const RayIntersectionConstants& rc)
{
	auto t = IntersectRayInnerTerrain(rc);
	CheckRayIntersectionBounds(rc, &t);
	return t;
}

float FieldHeightQuadTree::IntersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir) const
{
	return ::IntersectRay(RayIntersectionConstants
		{
			rayOrigin, rayDir,
			m_Terrain->GetCountFields(), m_Nodes.GetArray(), m_Terrain->GetSurfaceCoefficients().GetArray()
		});
}

void FieldHeightQuadTree::ShrinkNodes()
{
	auto countNodes = m_Nodes.GetSize();
	auto endIt = m_Nodes.GetEndIterator();

	std::map<unsigned, unsigned> indexMapping;
	{
		int i = 0;
		for (auto it = m_Nodes.GetBeginIterator(); it != endIt; ++it, ++i)
		{
			indexMapping.insert(std::make_pair(m_Nodes.ToIndex(it), i));
		}
	}
	auto mapNodeIndex = [&indexMapping](unsigned index) { return index == Core::c_InvalidIndexU ? Core::c_InvalidIndexU : indexMapping[index]; };

	Core::SimpleTypeUnorderedVectorU<Node> resultNodes;
	{
		for (auto it = m_Nodes.GetBeginIterator(); it != endIt; ++it)
		{
			auto& sourceNode = *it;
			Node targetNode;
			targetNode.Parent = mapNodeIndex(sourceNode.Parent);
			for (unsigned i = 0; i < 4; i++)
			{
				targetNode.Children[i] = mapNodeIndex(sourceNode.Children[i]);
			}
			targetNode.MinHeight = sourceNode.MinHeight;
			targetNode.MaxHeight = sourceNode.MaxHeight;
			targetNode.IsDirty = sourceNode.IsDirty;
			resultNodes.Add(targetNode);
		}
	}

	m_Nodes = resultNodes;
}

void FieldHeightQuadTree::SerializeSB(Core::ByteVector& bytes) const
{
	// The terrain pointer is not serialized.

	auto serializedObject = *this;
	serializedObject.Update(glm::ivec2(0), glm::ivec2(m_Terrain->GetCountFields()) - 1);
	serializedObject.ShrinkNodes();
	auto& nodes = serializedObject.m_Nodes;

	auto countNodes = nodes.GetSize();
	Core::SerializeSB(bytes, countNodes);
	for (unsigned i = 0; i < countNodes; i++)
	{
		Core::SerializeSB(bytes, nodes[i]);
	}
}

void FieldHeightQuadTree::DeserializeSB(const unsigned char*& bytes)
{
	// The terrain pointer has to be set by the user code.

	unsigned countNodes;
	Core::DeserializeSB(bytes, countNodes);
	m_Nodes.Clear();
	for (unsigned i = 0; i < countNodes; i++)
	{
		Node node;
		Core::DeserializeSB(bytes, node);
		m_Nodes.Add(node);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void SetIntersectionToGrid(int blockSize, int size, float& worldIntersection, int& start, int& end)
{
	int hBlockSize = blockSize >> 1;
	if ((blockSize & 1) == 0)
	{
		int indexIntersection = (int)glm::round(worldIntersection);
		worldIntersection = (float)indexIntersection;
		start = std::max((int)indexIntersection - (int)hBlockSize, 0);
		end = std::min((int)indexIntersection + (int)hBlockSize - 1, size - 1);
	}
	else
	{
		int indexIntersection = (int)glm::floor(worldIntersection);
		worldIntersection = (float)indexIntersection;
		start = std::max((int)indexIntersection - (int)hBlockSize, 0);
		end = std::min((int)indexIntersection + (int)hBlockSize, size - 1);
	}
}

TerrainBlockIntersection GetTerrainBlockIntersection(Camera* camera,
	const glm::uvec2& windowsSize, const glm::uvec2& contentSize,
	const Terrain& terrain, const FieldHeightQuadTree* quadTree, const glm::ivec2& blockSize,
	const glm::vec2& cursorPositionInScreen)
{
	assert(camera != nullptr);

	glm::vec3 rayOrigin, rayDirection;
	bool rayValid;
	GetContentCursorRayInWorldCs(windowsSize, contentSize, *camera, false, rayOrigin, rayDirection, rayValid,
		cursorPositionInScreen);

	TerrainBlockIntersection result;

	//  Intersecting height quad tree.
	auto t = quadTree->IntersectRay(rayOrigin, rayDirection);
	if (t == c_InvalidIntersectionT) result.HasIntersection = false;
	else
	{
		auto countFields = terrain.GetCountFields();

		auto worldIntersection = rayOrigin + rayDirection * t;
		auto originalWorldIntersection = worldIntersection;
		SetIntersectionToGrid(blockSize.x, countFields.x, worldIntersection.x, result.Start.x, result.End.x);
		SetIntersectionToGrid(blockSize.y, countFields.y, worldIntersection.z, result.Start.y, result.End.y);
		result.HasIntersection = (result.Start.x < (int)countFields.x && result.Start.y < (int)countFields.y
			&& result.End.x >= 0 && result.End.y >= 0);

		result.CornerIndex = Core::c_InvalidIndexU;
		if (blockSize.x == 1 && blockSize.y == 1)
		{
			auto x = std::fmod(originalWorldIntersection.x, 1.0f);
			auto z = std::fmod(originalWorldIntersection.z, 1.0f);
			if (x + z < 0.5f) result.CornerIndex = 3;
			else if (x + z > 1.5f) result.CornerIndex = 1;
			else if (z - x > 0.5f) result.CornerIndex = 0;
			else if (x - z > 0.5f) result.CornerIndex = 2;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FieldHeightQuadTree::Node::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Parent);
	Core::SerializeSB(bytes, Children);
	Core::SerializeSB(bytes, MinHeight);
	Core::SerializeSB(bytes, MaxHeight);
	assert(!IsDirty);
}

void FieldHeightQuadTree::Node::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Parent);
	Core::DeserializeSB(bytes, Children);
	Core::DeserializeSB(bytes, MinHeight);
	Core::DeserializeSB(bytes, MaxHeight);
	IsDirty = false;
}
