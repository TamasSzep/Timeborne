// Timeborne/InGame/GameCamera/GameCameraState.h

#pragma once

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <EngineBuildingBlocks/Math/GLM.h>

struct GameCameraState
{
public: // Rotation.

	// For direction.xz:
	// 0: (+, -)
	// 1: (-, -)
	// 2: (-, +)
	// 3: (+, +)
	int RotationIndex = 1;

	// The rotation angle around the Y coordinate, for X = 1, Z = 0: angle = 0.
	float StartAngle = 0.0f;
	float TargetAngle = 0.0f;
	float RotationAngle = 0.0f;

	bool Rotating = false;
	float RotationAnimationTime = 0.0f;

public: // Position.

	glm::vec3 SpeedTime = glm::vec3(0.0f);
	glm::vec2 LookAt = glm::vec2(0.0f);

public: // Projection.

	bool Zooming = false;
	float ZoomValue = -1.0f; // Initialized to INVALID value.
	float StartZoom = -1.0f;
	float TargetZoom = -1.0f;
	float ZoomAnimationTime = 0.0f;

public:

	GameCameraState();

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};