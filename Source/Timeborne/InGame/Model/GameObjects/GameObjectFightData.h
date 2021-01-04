// Timeborne/InGame/Model/GameObjects/GameObjectFightData.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/GameObjectId.h>

#include <Core/DataStructures/SimpleTypeUnorderedVector.hpp>
#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/Constants.h>

#include <cstdint>

enum class AttackState
{
	// Not attacking anything.
	None,

	// Approaching the target.
	//
	// - EnRouteAttackers: positioning
	// - NonEnRouteAttackers: positioning and orienting
	Approach,

	// Attacking without movement and orienting.
	Attack,

	// EnRouteAttacker-only state: orienting while not being on the route.
	Turning

	// Use these states for secondary attack targets:

	//// EnRouteAttacker-only state: orienting on the route.
	//EnRouteTurning,

	//// EnRouteAttacker-only state: attacking on the route.
	//EnRouteAttack
};

struct GameObjectFightData
{
	uint32_t HealthPoints = Core::c_InvalidIndexU;

	GameObjectId AttackTarget = c_InvalidGameObjectId;
	AttackState AttackState = AttackState::None;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};

using GameObjectFightList = Core::SimpleTypeUnorderedVectorU<GameObjectFightData>;
