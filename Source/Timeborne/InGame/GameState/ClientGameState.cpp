// Timeborne/InGame/GameState/ClientGameState.cpp

#include <Timeborne/InGame/GameState/ClientGameState.h>

#include <Core/SimpleBinarySerialization.hpp>

ClientGameState::ClientGameState()
	: m_LocalGameState(m_ClientModelGameState)
{
}

ClientGameState::~ClientGameState()
{
}

const GameCreationData& ClientGameState::GetGameCreationData() const
{
	return m_GameCreationData;
}

void ClientGameState::SetGameCreationData(const GameCreationData& data)
{
	m_GameCreationData = data;
	m_LocalGameState.SetGameCreationData(data);
}

LocalGameState& ClientGameState::GetLocalGameState()
{
	return m_LocalGameState;
}

ServerGameState& ClientGameState::GetClientModelGameState()
{
	return m_ClientModelGameState;
}

ServerGameState& ClientGameState::GetSyncedGameState()
{
	// Optimization for single player mode: no need to synchronize the game state.
	return m_GameCreationData.Players.IsMultiplayerGame() ? m_SyncedGameState : m_ClientModelGameState;
}

void ClientGameState::Sync()
{
	if (m_GameCreationData.Players.IsMultiplayerGame())
	{
		// @todo: implement synchronization. Elements must be added one by one to notify listeners.
	}
}

void ClientGameState::SerializeForSave(Core::ByteVector& bytes)
{
	Core::SerializeSB(bytes, m_GameCreationData);
	Core::SerializeSB(bytes, m_ClientModelGameState);
	Core::SerializeSB(bytes, m_LocalGameState);
}

void ClientGameState::DeserializeForLoad(const Core::ByteVector& bytes)
{
	auto byteArray = bytes.GetArray();
	Core::DeserializeSB(byteArray, m_GameCreationData);
	Core::DeserializeSB(byteArray, m_ClientModelGameState);
	Core::DeserializeSB(byteArray, m_LocalGameState);
}
