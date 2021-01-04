// Timeborne/InGame/Model/PlayerData.h

#pragma once

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <Core/Constants.h>

#include <cstdint>

enum class PlayerType
{
	Computer, User
};

struct PlayerData
{
	PlayerType PlayerType = PlayerType::Computer;
	uint32_t AllianceIndex = 0;
	uint32_t LevelEditorIndex = Core::c_InvalidIndexU;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};

class PlayerDataList
{
	Core::SimpleTypeVectorU<PlayerData> m_Players;

public:

	PlayerDataList();
	~PlayerDataList();

	const PlayerData& operator[](uint32_t index) const;
	
	uint32_t GetCountPlayers() const;
	bool IsMultiplayerGame() const;
	bool AreAllied(uint32_t playerIndex1, uint32_t playerIndex2) const;

	uint32_t AddPlayer();
	void RemovePlayer(uint32_t playerIndex);
	
	void SetPlayerType(uint32_t playerIndex, PlayerType playerType);
	void SetAllianceIndex(uint32_t playerIndex, uint32_t allianceIndex);
	void SetLevelEditorIndex(uint32_t playerIndex, uint32_t levelEditorIndex);

	void SetFreeForAll(uint32_t maxCountAlliances);

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};