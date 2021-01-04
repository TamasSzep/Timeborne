// Timeborne/InGame/Model/GameObjects/Prototype/GameObjectMovementPrototype.h

#pragma once

#include <Core/Enum.h>
#include <EngineBuildingBlocks/Math/GLM.h>

struct GameObjectMovementPrototype
{
public: // Flags.

	enum class MovementFlags
	{
		None = 0,
		Dynamic = 1
	};

	MovementFlags Flags = MovementFlags::None;

	bool IsDynamic() const;
	void SetDynamic();

public: // Positions, speeds.

	// Fly height in units.
	float FlyHeight = 0.0f;

	// Speed in unit per second.
	float Speed = 0.0f;

	// Rotation speed in radians per second.
	float RotationSpeed = 0.0f;

public: // For mappings.

	glm::vec3 GameLogicSize = glm::vec3(0.0f);

	// Position terrain node mapping.
	float PositionTerrainNodeMappingCircleRadius = 0.0f;
};

UseEnumAsFlagSet(GameObjectMovementPrototype::MovementFlags);
