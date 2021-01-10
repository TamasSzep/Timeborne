// Timeborne/InGame/GameState/LocalGameState.h

#pragma once

#include <Timeborne/InGame/Controller/ControllerGameState.h>
#include <Timeborne/InGame/GameCamera/GameCameraState.h>
#include <Timeborne/InGame/GameState/InGameStatistics.h>

struct GameCreationData;
class ServerGameState;

// Contains data that is:
//  - NOT synced with the server game state
//  - saved/loaded
class LocalGameState
{
	GameCameraState m_GameCameraState;
	ControllerGameState m_ControllerGameState;
	InGameStatistics m_Statistics;

public:

	explicit LocalGameState(ServerGameState& modelGameState);
	~LocalGameState();

	void SetGameCreationData(const GameCreationData& data);

	const GameCameraState& GetGameCameraState() const;
	GameCameraState& GetGameCameraState();
	const ControllerGameState& GetControllerGameState() const;
	ControllerGameState& GetControllerGameState();

	const InGameStatistics& GetStatistics() const;
	InGameStatistics& GetStatistics();

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};