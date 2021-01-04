// Timeborne/InGame/Model/Terrain/Terrain.h

#pragma once

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/DataStructures/SimpleTypeUnorderedVector.hpp>
#include <EngineBuildingBlocks/Math/GLM.h>

struct FieldData
{
	// Field heights according to the following convention:
	//
	// +-------------> X
	// |  3   2
	// |  +---+
	// |  |   |
	// |  |   |
	// |  |   |
	// |  +---+
	// |  0   1
	// V
	// Z
	glm::vec4 Heights;
};

class Terrain
{
	// Fields have the world size: 1x1

	// Number of fields.
	glm::uvec2 m_CountFields;

	// Edit view of the terrain: 4 corner editable height values per field.
	Core::SimpleTypeVectorU<FieldData> m_Fields;

	// Render and compute view of the terrain: per-field derivatives and 4x4 matrix coefficients.
	// They represent a per-field bicubic spline, where derivatives are synchronized between same-position
	// corners.
	Core::SimpleTypeVectorU<glm::vec4> m_DX, m_DY, m_DXY;
	Core::SimpleTypeVectorU<glm::mat4> m_SurfaceCoefficients;

	void CreateSurfaceData();

public:

	Terrain();
	Terrain(const glm::uvec2& countFields);

	const glm::uvec2 GetCountFields() const;
	const glm::uvec2 GetCountCornerHeights() const;

	glm::vec2 GetWorldSizeXZ() const;

	FieldData* GetFields();
	const FieldData* GetFields() const;

	float GetHeight(const glm::ivec2& fieldIndex, float xInField, float zInField) const;
	float GetMiddleHeight(const glm::ivec2& fieldIndex) const;
	glm::vec3 GetPosition(const glm::ivec2& fieldIndex, float xInField, float zInField) const;
	glm::vec3 GetMiddlePosition(const glm::ivec2& fieldIndex) const;
	glm::vec3 GetNormal(const glm::ivec2& fieldIndex, float xInField, float zInField) const;

	const Core::SimpleTypeVectorU<glm::mat4>& GetSurfaceCoefficients() const;

	void UpdateSurfaceData(const glm::ivec2& changeStart, const glm::ivec2& changeEnd);

	static const float* GetFieldHeights(const FieldData* fields, const glm::uvec2& countFields, int x, int z);
	static glm::vec2 GetFieldHeightMinMax(const FieldData* fields, const glm::uvec2& countFields, int x, int z);
	static float GetFieldHeight(const FieldData* fields, const glm::uvec2& countFields, int x, int z, int fId);

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes, bool forceRecomputations);

	static void GetFlippedLimits(const glm::ivec2& start, const glm::ivec2& end,
		glm::ivec2* pStart, glm::ivec2* pEnd);
};
