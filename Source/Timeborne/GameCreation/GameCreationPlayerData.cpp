// Timeborne/InGame/Model/GameCreationPlayerData.cpp

#include <Timeborne/GameCreation/GameCreationPlayerData.h>

#include <Core/SimpleBinarySerialization.hpp>

void GameCreationPlayerData::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Name);
	Core::SerializeSB(bytes, PlayerType);
	Core::SerializeSB(bytes, AllianceIndex);
	Core::SerializeSB(bytes, LevelEditorIndex);
}

void GameCreationPlayerData::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Name);
	Core::DeserializeSB(bytes, PlayerType);
	Core::DeserializeSB(bytes, AllianceIndex);
	Core::DeserializeSB(bytes, LevelEditorIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GameCreationPlayerDataList::GameCreationPlayerDataList()
{
}

GameCreationPlayerDataList::~GameCreationPlayerDataList()
{
}

const GameCreationPlayerData& GameCreationPlayerDataList::operator[](uint32_t index) const
{
	return m_Players[index];
}

uint32_t GameCreationPlayerDataList::GetCountPlayers() const
{
	return (uint32_t)m_Players.size();
}

bool GameCreationPlayerDataList::IsMultiplayerGame() const
{
	uint32_t countUsers = 0;
	uint32_t countPlayers = (uint32_t)m_Players.size();
	for (uint32_t i = 0; i < countPlayers; i++)
	{
		countUsers += (uint32_t)(m_Players[i].PlayerType == PlayerType::User);
	}
	return (countUsers >= 2);
}

bool GameCreationPlayerDataList::AreAllied(uint32_t playerIndex1, uint32_t playerIndex2) const
{
	return m_Players[playerIndex1].AllianceIndex == m_Players[playerIndex2].AllianceIndex;
}

uint32_t GameCreationPlayerDataList::AddPlayer()
{
	uint32_t playerIndex = (uint32_t)m_Players.size();
	m_Players.emplace_back();
	return playerIndex;
}

void GameCreationPlayerDataList::SetPlayerName(uint32_t playerIndex, const char* name)
{
	m_Players[playerIndex].Name = name;
}

void GameCreationPlayerDataList::SetPlayerType(uint32_t playerIndex, PlayerType playerType)
{
	m_Players[playerIndex].PlayerType = playerType;
}

void GameCreationPlayerDataList::SetAllianceIndex(uint32_t playerIndex, uint32_t allianceIndex)
{
	m_Players[playerIndex].AllianceIndex = allianceIndex;
}

void GameCreationPlayerDataList::SetLevelEditorIndex(uint32_t playerIndex, uint32_t levelEditorIndex)
{
	m_Players[playerIndex].LevelEditorIndex = levelEditorIndex;
}

void GameCreationPlayerDataList::SetFreeForAll(uint32_t maxCountAlliances)
{
	uint32_t countPlayers = (uint32_t)m_Players.size();
	uint32_t smallTeamSize = countPlayers / maxCountAlliances;
	uint32_t bigTeamSize = smallTeamSize + 1;
	uint32_t countBigTeams = countPlayers - (smallTeamSize * maxCountAlliances);
	uint32_t playerIndex = 0;
	for (uint32_t i = 0; i < countBigTeams; i++)
	{
		for (uint32_t j = 0; j < bigTeamSize; j++, playerIndex++)
		{
			m_Players[playerIndex].AllianceIndex = i;
		}
	}
	for (uint32_t i = countBigTeams; i < maxCountAlliances; i++)
	{
		for (uint32_t j = 0; j < smallTeamSize; j++, playerIndex++)
		{
			m_Players[playerIndex].AllianceIndex = i;
		}
	}
	assert(playerIndex == countPlayers);
}

void GameCreationPlayerDataList::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, m_Players);
}

void GameCreationPlayerDataList::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, m_Players);
}
