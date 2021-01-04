// Timeborne/GameCreation/GameCreationData.h

#pragma once

#include <Timeborne/GameCreation/PlayerData.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/Constants.h>

#include <cstdint>
#include <string>

struct GameCreationData
{
	PlayerDataList Players;
	uint32_t LocalPlayerIndex = Core::c_InvalidIndexU;
	std::string LevelName;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};
