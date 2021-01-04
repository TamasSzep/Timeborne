// Timeborne/InGame/GameState/LocalGameState.cpp

#include <Timeborne/InGame/GameState/LocalGameState.h>

#include <Core/SimpleBinarySerialization.hpp>

void LocalGameState::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, GameCameraState);
	Core::SerializeSB(bytes, ControllerGameState);
}

void LocalGameState::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, GameCameraState);
	Core::DeserializeSB(bytes, ControllerGameState);
}
