// Timeborne/InGame/Model/GameObjects/Prototype/GameObjectFightPrototype.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/HeightDependentDistanceParameters.h>

#include <cstdint>

enum class AttackType
{
	None, Immediate, Projectile
};

struct AttackPrototypeData
{
	AttackType Type = AttackType::None;
	uint32_t AnimationDurationMs = 0;
	uint32_t ReattackDurationMs = 0;
	uint32_t HitPoints = 0;
	bool EnRouteAttacker = false;
	HeightDependentDistanceParameters ApproachDistance = {};
};

struct GameObjectFightPrototype
{
	uint32_t MaxHealthPoints = 0;

	AttackPrototypeData GroundAttack;
	AttackPrototypeData AirAttack;
};
