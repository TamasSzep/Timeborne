// Timeborne/GameCreation/GameCreationData.cpp

#include <Timeborne/GameCreation/GameCreationData.h>

#include <Core/SimpleBinarySerialization.hpp>

const GameCreationPlayerData& GameCreationData::GetOwnPlayerData() const
{
	return Players[LocalPlayerIndex];
}

void GameCreationData::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Players);
	Core::SerializeSB(bytes, LocalPlayerIndex);
	Core::SerializeSB(bytes, LevelName);
}

void GameCreationData::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Players);
	Core::DeserializeSB(bytes, LocalPlayerIndex);
	Core::DeserializeSB(bytes, LevelName);
}
