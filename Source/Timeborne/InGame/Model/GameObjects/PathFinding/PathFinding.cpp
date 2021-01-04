// Timeborne/InGame/Model/GameObjects/PathFinding/PathFinding.cpp

#include <Timeborne/InGame/Model/GameObjects/PathFinding/PathFinding.h>

#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>

#include <Core/SimpleBinarySerialization.hpp>

void GameObjectPathFieldData::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, TerrainTreeNodeIndex);
	Core::SerializeSB(bytes, Core::ToPlaceHolder(FieldIndex));
}

void GameObjectPathFieldData::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, TerrainTreeNodeIndex);
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(FieldIndex));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GameObjectPath::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, ObjectId);
	Core::SerializeSB(bytes, Core::ToPlaceHolder(SourceField));
	Core::SerializeSB(bytes, Core::ToPlaceHolder(TargetField));
	Core::SerializeSB(bytes, Fields);
	Core::SerializeSB(bytes, MaxDistance);
}

void GameObjectPath::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, ObjectId);
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(SourceField));
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(TargetField));
	Core::DeserializeSB(bytes, Fields);
	Core::DeserializeSB(bytes, MaxDistance);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PathDistance GetPathFindingHeuristicGuess(const PathFindingContext& context,
	unsigned startNodeIndex, unsigned endNodeIndex)
{
	auto& terrainTree = context.TerrainTree;
	auto& startNode = terrainTree.GetNode(startNodeIndex);
	auto& endNode = terrainTree.GetNode(endNodeIndex);

	// Multiply all indices by two to be able to represent middles.
	auto doubleDiff = glm::abs((endNode.Start + endNode.End) - (startNode.Start + startNode.End));
	int diagonal = std::min(doubleDiff.x, doubleDiff.y);
	int straight = (doubleDiff.x + doubleDiff.y) - 2 * diagonal;
	return PathDistance(straight, diagonal);
}

PathDistance GetPathFindingNodeDistance(const PathFindingContext& context,
	unsigned startNodeIndex, unsigned endNodeIndex)
{
	// @todo: implement different speeds.
	return GetPathFindingHeuristicGuess(context, startNodeIndex, endNodeIndex);
}