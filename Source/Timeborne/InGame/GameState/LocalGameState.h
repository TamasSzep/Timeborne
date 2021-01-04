// Timeborne/InGame/GameState/LocalGameState.h

#pragma once

#include <Timeborne/InGame/Controller/ControllerGameState.h>
#include <Timeborne/InGame/GameCamera/GameCameraState.h>

// Contains data that is:
//  - NOT synced with the server game state
//  - saved/loaded
struct LocalGameState
{
	GameCameraState GameCameraState;
	ControllerGameState ControllerGameState;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};