// Timeborne/InGame/Model/GameObjects/GameObjectPose.cpp

#include <Timeborne/InGame/Model/GameObjects/GameObjectPose.h>

#include <Timeborne/InGame/Model/Terrain/Terrain.h>
#include <Timeborne/Math/Math.h>

#include <Core/Comparison.h>
#include <Core/SimpleBinarySerialization.hpp>

#include <cmath>

GameObjectPose::GameObjectPose()
	: m_Position(0.0, 0.0, 0.0)
	, m_Direction(1.0f, 0.0f, 0.0f)
	, m_Up(0.0f, 1.0f, 0.0f)
	, m_Yaw(0.0f)
{
}

bool GameObjectPose::operator==(const GameObjectPose& other) const
{
	NumericalEqualCompareBlock(m_Position);
	NumericalEqualCompareBlock(m_Direction);
	NumericalEqualCompareBlock(m_Up);
	NumericalEqualCompareBlock(m_Yaw);
	return true;
}

bool GameObjectPose::operator!=(const GameObjectPose& other) const
{
	return !(*this == other);
}

glm::ivec2 GameObjectPose::GetTerrainFieldIndex() const
{
	return glm::ivec2(std::floor(m_Position.x), std::floor(m_Position.z));
}

glm::ivec2 GameObjectPose::GetTerrainFieldIndex(const glm::dvec2& position)
{
	return glm::ivec2(glm::floor(position));
}

glm::dvec2 GameObjectPose::GetPosition2d() const
{
	return glm::dvec2(m_Position.x, m_Position.z);
}

glm::dvec2 GameObjectPose::GetOffsetPosition2d(const glm::dvec2& target, double length) const
{
	auto position2d = GetPosition2d();
	return position2d + glm::normalize(target - position2d) * length;
}

double GameObjectPose::GetDistance2d(const glm::dvec2& target) const
{
	return glm::length(target - GetPosition2d());
}

double GameObjectPose::GetDistance2d(const GameObjectPose& target) const
{
	return glm::length(target.GetPosition2d() - GetPosition2d());
}

float GameObjectPose::GetYaw() const
{
	return m_Yaw;
}

float GameObjectPose::GetTargetYaw(const glm::dvec2& target) const
{
	auto d = target - GetPosition2d();
	return WrapWithRepeat((float)std::atan2(-d.y, d.x));
}

float GameObjectPose::GetTargetYaw(const GameObjectPose& target) const
{
	return GetTargetYaw(target.GetPosition2d());
}

GameObjectPose::GameOrientation2d GameObjectPose::GetGameOrientation2d() const
{
	glm::vec2 dir2 = glm::normalize(glm::vec2(m_Direction.x, m_Direction.z));
	return { dir2, glm::vec2(-dir2.y, dir2.x) };
}

void GameObjectPose::SetPosition(const Terrain& terrain, const glm::dvec2& position2d, float flyHeight)
{
	glm::ivec2 fieldIndex; float xInField, zInField;
	ToTerrainIndices(position2d, fieldIndex, xInField, zInField);
	float height = terrain.GetHeight(fieldIndex, xInField, zInField);
	m_Position = glm::dvec3(position2d.x, (double)height + (double)flyHeight, position2d.y);
}

void GameObjectPose::SetOrientationFromTerrain(const Terrain& terrain, float yaw)
{
	m_Yaw = WrapWithRepeat(yaw);

	glm::ivec2 fieldIndex; float xInField, zInField;
	ToTerrainIndices(GetPosition2d(), fieldIndex, xInField, zInField);
	m_Up = glm::vec3(terrain.GetNormal(fieldIndex, xInField, zInField));

	// Prefering correct direction over right vector. Note that this is identical to projecting the 2d direction vector
	// onto the surface (plane described by the normal vector).
	glm::vec3 right2d(std::sin(m_Yaw), 0.0f, std::cos(m_Yaw));
	m_Direction = glm::normalize(glm::cross(m_Up, right2d));
}

glm::dvec2 GameObjectPose::GetMiddle2dFromTerrainFieldIndex(const glm::ivec2& fieldIndex)
{
	return glm::dvec2(fieldIndex) + 0.5;
}

float GameObjectPose::WrapWithRepeat(float angle)
{
	assert(angle >= -3.0f * PI && angle < 3.0f * PI);
	if (angle < -PI) angle += TWO_PI;
	if (angle >= PI) angle -= TWO_PI;
	return angle;
}

void GameObjectPose::ToTerrainIndices(const glm::dvec2& position2d,
	glm::ivec2& fieldIndex, float& xInField, float& zInField)
{
	fieldIndex = glm::ivec2(glm::floor(position2d));
	xInField = (float)position2d.x - (float)fieldIndex.x;
	zInField = (float)position2d.y - (float)fieldIndex.y;
}

glm::vec3 GameObjectPose::GetWorldDirection() const
{
	return m_Direction;
}

glm::vec3 GameObjectPose::GetWorldUp() const
{
	return m_Up;
}

glm::vec3 GameObjectPose::GetWorldPosition() const
{
	return m_Position;
}

void GameObjectPose::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Core::ToPlaceHolder(m_Position));
	Core::SerializeSB(bytes, Core::ToPlaceHolder(m_Direction));
	Core::SerializeSB(bytes, Core::ToPlaceHolder(m_Up));
	Core::SerializeSB(bytes, m_Yaw);
}

void GameObjectPose::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(m_Position));
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(m_Direction));
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(m_Up));
	Core::DeserializeSB(bytes, m_Yaw);
}
