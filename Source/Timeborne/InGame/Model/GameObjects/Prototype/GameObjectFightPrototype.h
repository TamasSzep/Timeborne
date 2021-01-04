// Timeborne/InGame/Model/GameObjects/Prototype/GameObjectFightPrototype.h

#pragma once

#include <cstdint>

enum class AttackType
{
	None, Immediate, Projectile
};

struct AttackApproachPrototypeData
{
	float MaxDistance = 0.0f;
	float HeightDistanceFactor = 0.0f;
	float HeightDistanceMin = 1.0f;
	float HeightDistanceMax = 1.0f;
};

struct AttackPrototypeData
{
	AttackType Type = AttackType::None;
	uint32_t AnimationDurationMs = 0;
	uint32_t ReattackDurationMs = 0;
	uint32_t HitPoints = 0;
	bool EnRouteAttacker = false;
	AttackApproachPrototypeData Approach = {};
};

struct GameObjectFightPrototype
{
	uint32_t MaxHealthPoints = 0;

	AttackPrototypeData GroundAttack;
	AttackPrototypeData AirAttack;
};
