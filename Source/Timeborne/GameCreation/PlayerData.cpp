// Timeborne/InGame/Model/PlayerData.cpp

#include <Timeborne/GameCreation/PlayerData.h>

#include <Core/SimpleBinarySerialization.hpp>

void PlayerData::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, PlayerType);
	Core::SerializeSB(bytes, AllianceIndex);
}

void PlayerData::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, PlayerType);
	Core::DeserializeSB(bytes, AllianceIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PlayerDataList::PlayerDataList()
{
}

PlayerDataList::~PlayerDataList()
{
}

const PlayerData& PlayerDataList::operator[](uint32_t index) const
{
	return m_Players[index];
}

uint32_t PlayerDataList::GetCountPlayers() const
{
	return m_Players.GetSize();
}

bool PlayerDataList::IsMultiplayerGame() const
{
	uint32_t countUsers = 0;
	uint32_t countPlayers = m_Players.GetSize();
	for (uint32_t i = 0; i < countPlayers; i++)
	{
		countUsers += (uint32_t)(m_Players[i].PlayerType == PlayerType::User);
	}
	return (countUsers >= 2);
}

bool PlayerDataList::AreAllied(uint32_t playerIndex1, uint32_t playerIndex2) const
{
	return m_Players[playerIndex1].AllianceIndex == m_Players[playerIndex2].AllianceIndex;
}

uint32_t PlayerDataList::AddPlayer()
{
	uint32_t playerIndex = m_Players.GetSize();
	m_Players.PushBack(PlayerData{});
	return playerIndex;
}

void PlayerDataList::RemovePlayer(uint32_t playerIndex)
{
	m_Players.Remove(playerIndex);
}

void PlayerDataList::SetPlayerType(uint32_t playerIndex, PlayerType playerType)
{
	m_Players[playerIndex].PlayerType = playerType;
}

void PlayerDataList::SetAllianceIndex(uint32_t playerIndex, uint32_t allianceIndex)
{
	m_Players[playerIndex].AllianceIndex = allianceIndex;
}

void PlayerDataList::SetLevelEditorIndex(uint32_t playerIndex, uint32_t levelEditorIndex)
{
	m_Players[playerIndex].LevelEditorIndex = levelEditorIndex;
}

void PlayerDataList::SetFreeForAll(uint32_t maxCountAlliances)
{
	uint32_t countPlayers = m_Players.GetSize();
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

void PlayerDataList::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, m_Players);
}

void PlayerDataList::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, m_Players);
}
