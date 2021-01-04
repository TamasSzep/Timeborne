// Timeborne/InGame/Model/GameObjects/PathFinding/PathFinding.h

#pragma once

#define MEASURE_PATH_FINDING_EXECUTION_TIME 0
#define CREATE_PATH_FINDING_STATISTICS 0

#include <Timeborne/InGame/Model/GameObjects/GameObjectId.h>
#include <Timeborne/Math/SqrtExtendedIntegerRing.hpp>

#include <Core/DataStructures/ResourceUnorderedVector.hpp>
#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <EngineBuildingBlocks/Math/GLM.h>

struct GameObjectPathFieldData
{
	unsigned TerrainTreeNodeIndex;
	glm::ivec2 FieldIndex;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};

struct GameObjectPath
{
	GameObjectId ObjectId;
	glm::ivec2 SourceField;
	glm::ivec2 TargetField;
	Core::SimpleTypeVectorU<GameObjectPathFieldData> Fields;
	float MaxDistance;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};

struct GameObjectData;
class Level;
class TerrainTree;

struct PathFindingContext
{
	const GameObjectData& GameObjectData;
	const Level& Level;
	const TerrainTree& TerrainTree;
};

using PathDistance = SqrtExtendedIntegerRing<Sqrt2IntegerRingExtender>;

PathDistance GetPathFindingHeuristicGuess(const PathFindingContext& context,
	unsigned startNodeIndex, unsigned endNodeIndex);
PathDistance GetPathFindingNodeDistance(const PathFindingContext& context,
	unsigned startNodeIndex, unsigned endNodeIndex);