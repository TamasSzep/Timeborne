// Timeborne/InGame/GameState/InGameStatistics.cpp

#include <Timeborne/InGame/GameState/InGameStatistics.h>

#include <Timeborne/GameCreation/GameCreationData.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>

#include <numeric>

void InGameStatistics::PlayerData::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, ProducedUnits);
	Core::SerializeSB(bytes, ProducedBuildings);
	Core::SerializeSB(bytes, DestroyedUnits);
	Core::SerializeSB(bytes, DestroyedBuildings);
}

void InGameStatistics::PlayerData::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, ProducedUnits);
	Core::DeserializeSB(bytes, ProducedBuildings);
	Core::DeserializeSB(bytes, DestroyedUnits);
	Core::DeserializeSB(bytes, DestroyedBuildings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

InGameStatistics::InGameStatistics(ServerGameState& modelGameState)
{
	modelGameState.GetGameObjects().AddFightListenerOnce(*this);
}

InGameStatistics::~InGameStatistics()
{
}

void InGameStatistics::SetGameCreationData(const GameCreationData& data)
{
	uint32_t countPlayers = data.Players.GetCountPlayers();
	m_PlayerData.resize(countPlayers);
	for (uint32_t i = 0; i < countPlayers; i++)
	{
		m_PlayerData[i].DestroyedUnits.PushBack(0U, countPlayers);
		m_PlayerData[i].DestroyedBuildings.PushBack(0U, countPlayers);
	}
}

const std::vector<InGameStatistics::PlayerData>& InGameStatistics::GetPlayerData() const
{
	return m_PlayerData;
}

void InGameStatistics::OnGameObjectDestroyed(const GameObject& sourceObject, const GameObject& targetObject)
{
	auto sourcePlayer = sourceObject.Data.PlayerIndex;
	auto targetPlayer = targetObject.Data.PlayerIndex;
	if (sourcePlayer == Core::c_InvalidIndexU || targetPlayer == Core::c_InvalidIndexU) return;

	auto type = GameObjectPrototype::GetPrototypes()[(uint32_t)targetObject.Data.TypeIndex]->GetType();

	if (type == GameObjectPrototype::Type::Unit)
	{
		m_PlayerData[sourcePlayer].DestroyedUnits[targetPlayer]++;
	}
	else if (type == GameObjectPrototype::Type::Building)
	{
		m_PlayerData[sourcePlayer].DestroyedBuildings[targetPlayer]++;
	}
}

const Core::IndexVectorU& InGameStatistics::GetLostUnits(uint32_t playerIndex) const
{
	auto countPlayers = (uint32_t)m_PlayerData.size();
	m_TempIndexVector.Resize(countPlayers);
	for (uint32_t i = 0; i < countPlayers; i++)
	{
		m_TempIndexVector[i] = m_PlayerData[i].DestroyedUnits[playerIndex];
	}
	return m_TempIndexVector;
}

const Core::IndexVectorU& InGameStatistics::GetLostBuildings(uint32_t playerIndex) const
{
	auto countPlayers = (uint32_t)m_PlayerData.size();
	m_TempIndexVector.Resize(countPlayers);
	for (uint32_t i = 0; i < countPlayers; i++)
	{
		m_TempIndexVector[i] = m_PlayerData[i].DestroyedBuildings[playerIndex];
	}
	return m_TempIndexVector;
}

uint32_t InGameStatistics::GetLostUnitCount(uint32_t playerIndex) const
{
	const auto& values = GetLostUnits(playerIndex);
	return std::accumulate(values.GetArray(), values.GetEndPointer(), 0);
}

uint32_t InGameStatistics::GetLostBuildingCount(uint32_t playerIndex) const
{
	const auto& values = GetLostBuildings(playerIndex);
	return std::accumulate(values.GetArray(), values.GetEndPointer(), 0);
}

uint32_t InGameStatistics::GetWinnerAlliance() const
{
	return m_WinnerAlliance;
}

void InGameStatistics::SetWinnerAlliance(uint32_t winnerAlliance)
{
	m_WinnerAlliance = winnerAlliance;
}

void InGameStatistics::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, m_PlayerData);
	Core::SerializeSB(bytes, m_WinnerAlliance);
}

void InGameStatistics::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, m_PlayerData);
	Core::DeserializeSB(bytes, m_WinnerAlliance);
}
