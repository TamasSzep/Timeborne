// Timeborne/InGame/GameState/ClientGameState.h

#pragma once

#include <Timeborne/GameCreation/GameCreationData.h>
#include <Timeborne/InGame/GameState/LocalGameState.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>

class ClientGameState
{
	GameCreationData m_GameCreationData;

	// The game state received/updated from the server, updated with the SERVER's model.
	ServerGameState m_ServerModelGameState;

	// The server's game state and some locally added changes from the CLIENT's model.
	//
	// This is the game state that will be saved in single player mode.
	//
	ServerGameState m_ClientModelGameState;

	// The synchronized game state for the view.
	//
	// If the game is single player, 'm_ClientModelGameState' is returned instead of this object
	// in order to avoid unnecessary copying/synchronization overhead.
	//
	ServerGameState m_SyncedGameState;

	// The LOCAL game state, which is only relevant for the view.
	LocalGameState m_LocalGameState;

public:

	const GameCreationData& GetGameCreationData() const;
	void SetGameCreationData(const GameCreationData& data);

	ServerGameState& GetClientModelGameState();
	ServerGameState& GetSyncedGameState();

	LocalGameState& GetLocalGameState();

	void Sync();

	void SerializeForSave(Core::ByteVector& bytes);
	void DeserializeForLoad(const Core::ByteVector& bytes);
};
