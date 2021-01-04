// Timeborne/InGame/Model/GameObjects/GameObjectPose.h

#pragma once

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <EngineBuildingBlocks/Math/GLM.h>

class Terrain;

// Fixed point object position to allow computing the object pose using
// integer arithmetic, which is always expected to work the same way on
// all platforms.
class GameObjectPose
{
	// The world position, which is also the position in fields, since the field have the size 1x1.
	// Storing the position in double to avoid floating-point computation errors.
	glm::dvec3 m_Position;

	glm::vec3 m_Direction;
	glm::vec3 m_Up;

	// For objects moving on the ground the yaw angle and the terrain
	// determines the direction and up vectors. For objects oriented
	// independently of the terrain, animating the direction and up vectors
	// directly.

	// Yaw angle in radians. Range: [-pi, pi[. 0 is east, i.e. +X.
	float m_Yaw;

public:

	GameObjectPose();

	bool operator==(const GameObjectPose& other) const;
	bool operator!=(const GameObjectPose& other) const;

public: // Game positioning.

	// Getters.
	glm::ivec2 GetTerrainFieldIndex() const;
	static glm::ivec2 GetTerrainFieldIndex(const glm::dvec2& position);
	glm::dvec2 GetPosition2d() const;
	glm::dvec2 GetOffsetPosition2d(const glm::dvec2& target, double length) const;
	double GetDistance2d(const glm::dvec2& target) const;
	double GetDistance2d(const GameObjectPose& target) const;
	float GetYaw() const;
	float GetTargetYaw(const glm::dvec2& target) const;
	float GetTargetYaw(const GameObjectPose& target) const;

	struct GameOrientation2d
	{
		glm::vec2 Direction;
		glm::vec2 Right;
	};

	GameOrientation2d GetGameOrientation2d() const;

	// Setters.
	void SetPosition(const Terrain& terrain, const glm::dvec2& position2d, float flyHeight);
	void SetOrientationFromTerrain(const Terrain& terrain, float yaw);

	static glm::dvec2 GetMiddle2dFromTerrainFieldIndex(const glm::ivec2& fieldIndex);
	static float WrapWithRepeat(float angle);

private:

	static void ToTerrainIndices(const glm::dvec2& position2d, glm::ivec2& fieldIndex, float& xInField, float& zInField);

public: // World positioning.

	glm::vec3 GetWorldDirection() const;
	glm::vec3 GetWorldUp() const;
	glm::vec3 GetWorldPosition() const;

public: // Serialization.

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};