// Timeborne/InGame/Model/GameObjects/PathFinding/PathFinder.cpp

#include <Timeborne/InGame/Model/GameObjects/PathFinding/PathFinder.h>

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>
#include <Timeborne/InGame/Model/Level.h>

#include <Core/Constants.h>

PathFinder::PathFinder(const Level& level, const GameObjectData& gameObjectData)
	: m_Level(level)
	, m_GameObjectData(gameObjectData)
{
}

void PathFinder::FindPath(GameObjectId objectId, const glm::ivec2& targetField, float maxDistance, GameObjectPath& result)
{
	assert(m_Level.GetTerrainTree() != nullptr);

	assert(m_GameObjectData.ClientModelGameState != nullptr);
	const auto& gameObjectsMap = m_GameObjectData.ClientModelGameState->GetGameObjects().Get();

	auto gIt = gameObjectsMap.find(objectId);
	assert(gIt != gameObjectsMap.end());

	auto& sourceObject = gIt->second;
	auto& sourcePrototype = *GameObjectPrototype::GetPrototypes()[(uint32_t)sourceObject.Data.TypeIndex];

	auto sourceField = sourceObject.Data.Pose.GetTerrainFieldIndex();

	PathFindingContext context{ m_GameObjectData, m_Level, *m_Level.GetTerrainTree() };

	result.ObjectId = objectId;
	result.SourceField = sourceField;
	result.TargetField = targetField;
	result.Fields.Clear();
	result.MaxDistance = maxDistance;

	if (sourceField == targetField
		|| (maxDistance > 0.0f && glm::length(glm::vec2(targetField - sourceField)) <= maxDistance)) return;

	// Checking whether the source and target nodes are on the same island.
	{
		auto& terrainTree = context.TerrainTree;

		auto startNodeIndex = terrainTree.GetNodeIndexForField(result.SourceField);
		auto startIslandIndex = terrainTree.GetIslandIndex(startNodeIndex);
		assert(startIslandIndex != Core::c_InvalidIndexU);

		// @todo: implement a more efficient pattern.
		auto countFields = m_Level.GetCountFields();
		int lDistance = (maxDistance > 0.0f) ? (int)std::floor(maxDistance) : 0;
		float maxDistanceSqr = maxDistance * maxDistance;

		bool connected = false;
		for (int x = -lDistance; x <= lDistance; x++)
		{
			for (int z = -lDistance; z <= lDistance; z++)
			{
				if (x * x + z * z > maxDistanceSqr) continue;

				auto nodeIndex = terrainTree.GetNodeIndexForField(result.TargetField + glm::ivec2(x, z));
				auto islandIndex = terrainTree.GetIslandIndex(nodeIndex);
				assert(islandIndex != Core::c_InvalidIndexU);
				if (islandIndex == startIslandIndex)
				{
					connected = true;
					break;
				}
			}
		}

		if (!connected)
		{
#if MEASURE_PATH_FINDING_EXECUTION_TIME || CREATE_PATH_FINDING_STATISTICS
			printf("No route exists.\n");
#endif
			return;
		}
	}

	switch (m_Algorithm)
	{
		case Algorithm::AStarOnly: SolveWithAStar(context, result); break;
		case Algorithm::SimpleHierarchicalPathFinder: m_SimpleHierarchicalPathFinder.FindPath(context, result); break;
	}
}

void PathFinder::SolveWithAStar(const PathFindingContext& context, GameObjectPath& result)
{
	auto& terrainTree = context.TerrainTree;

	auto searchStartNode = terrainTree.GetNodeIndexForField(result.SourceField);
	auto searchEndNode = terrainTree.GetNodeIndexForField(result.TargetField);

	m_AStar.FindPath(context, searchStartNode, searchEndNode, result.MaxDistance, m_TempIndices);

	auto countFieldsInPath = m_TempIndices.GetSize();
	for (unsigned i = 0; i < countFieldsInPath; i++)
	{
		auto nodeIndex = m_TempIndices[i];
		auto& fieldData = result.Fields.PushBackPlaceHolder();
		fieldData.TerrainTreeNodeIndex = nodeIndex;
		fieldData.FieldIndex = terrainTree.GetNode(nodeIndex).Start;
	}
}
