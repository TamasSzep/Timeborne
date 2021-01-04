// Timeborne/InGame/Controller/ControllerGameState.h

#pragma once

#include <Timeborne/InGame/Controller/GameObjects/GameObjectCommand.h>
#include <Timeborne/InGame/Model/GameObjects/GameObject.h>

#include <Core/Constants.h>

struct ControllerGameState
{
	Core::SimpleTypeVectorU<GameObjectId> SourceGameObjectIds;
	GameObjectCommand LastGameObjectCommand{};
	unsigned LastCommandId = Core::c_InvalidIndexU;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};
