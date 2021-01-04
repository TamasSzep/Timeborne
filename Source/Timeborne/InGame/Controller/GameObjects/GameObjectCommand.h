// Timeborne/InGame/Controller/GameObjects/GameObjectCommand.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>

#include <EngineBuildingBlocks/Math/GLM.h>
#include <Core/DataStructures/SimpleTypeVector.hpp>

struct GameObjectCommand
{
	enum class Type { ObjectToObject, ObjectToTerrain } Type;
	Core::SimpleTypeVectorU<GameObjectId> SourceIds;
	glm::ivec2 TargetField;
	GameObjectId TargetId;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};
