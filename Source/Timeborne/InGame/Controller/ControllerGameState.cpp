// Timeborne/InGame/Controller/ControllerGameState.cpp

#include <Timeborne/InGame/Controller/ControllerGameState.h>

#include <Core/SimpleBinarySerialization.hpp>

void ControllerGameState::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, SourceGameObjectIds);
	Core::SerializeSB(bytes, LastGameObjectCommand);
	Core::SerializeSB(bytes, LastCommandId);
}

void ControllerGameState::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, SourceGameObjectIds);
	Core::DeserializeSB(bytes, LastGameObjectCommand);
	Core::DeserializeSB(bytes, LastCommandId);
}