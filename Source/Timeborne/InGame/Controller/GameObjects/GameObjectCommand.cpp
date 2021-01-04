// Timeborne/InGame/Controller/GameObjects/GameObjectCommand.cpp

#include <Timeborne/InGame/Controller/GameObjects/GameObjectCommand.h>

#include <Core/SimpleBinarySerialization.hpp>

void GameObjectCommand::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Type);
	Core::SerializeSB(bytes, SourceIds);
	Core::SerializeSB(bytes, Core::ToPlaceHolder(TargetField));
	Core::SerializeSB(bytes, TargetId);
}

void GameObjectCommand::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Type);
	Core::DeserializeSB(bytes, SourceIds);
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(TargetField));
	Core::DeserializeSB(bytes, TargetId);
}
