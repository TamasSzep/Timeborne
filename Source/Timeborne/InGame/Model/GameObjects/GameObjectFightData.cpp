// Timeborne/InGame/Model/GameObjects/GameObjectFightData.cpp

#include <Timeborne/InGame/Model/GameObjects/GameObjectFightData.h>

#include <Core/SimpleBinarySerialization.hpp>

void GameObjectFightData::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, HealthPoints);
	Core::SerializeSB(bytes, AttackTarget);
	Core::SerializeSB(bytes, AttackState);
}

void GameObjectFightData::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, HealthPoints);
	Core::DeserializeSB(bytes, AttackTarget);
	Core::DeserializeSB(bytes, AttackState);
}
