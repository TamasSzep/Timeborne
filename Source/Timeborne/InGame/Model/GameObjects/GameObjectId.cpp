// Timeborne/InGame/Model/GameObjects/GameObjectId.cpp

#include <Timeborne/InGame/Model/GameObjects/GameObjectId.h>

#include <Core/SimpleBinarySerialization.hpp>

GameObjectId GameObjectId::MakeId(uint32_t id)
{
	return GameObjectId(id);
}

GameObjectId::operator uint32_t() const
{
	return Id;
}

bool GameObjectId::operator==(const GameObjectId& other) const
{
	return Id == other.Id;
}

bool GameObjectId::operator!=(const GameObjectId& other) const
{
	return Id != other.Id;
}

bool GameObjectId::operator<(const GameObjectId& other) const
{
	return Id < other.Id;
}

void GameObjectId::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Id);
}

void GameObjectId::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Id);
}