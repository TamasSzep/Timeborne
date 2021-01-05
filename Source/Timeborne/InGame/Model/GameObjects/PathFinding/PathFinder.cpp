// Timeborne/InGame/Model/GameObjects/PathFinding/PathFinder.cpp

#include <Timeborne/InGame/Model/GameObjects/PathFinding/PathFinder.h>

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectPose.h>
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

float PathFinder::GetPathFindingHeight(const glm::ivec2& fieldIndex) const
{	
	// Must be consistent with the formula in A-star.
	auto terrainTree = m_Level.GetTerrainTree();
	assert(terrainTree != nullptr);
	auto nodeIndex = terrainTree->GetNodeIndexForField(fieldIndex);
	const auto& node = terrainTree->GetNode(nodeIndex);
	return (node.MinHeight + node.MaxHeight) * 0.5f;
}

float PathFinder::GetPathFindingHeight(const GameObjectPose& pose) const
{
	return GetPathFindingHeight(pose.GetTerrainFieldIndex());
}

bool PathFinder::FindPath(GameObjectId objectId, const glm::ivec2& targetField,
	const HeightDependentDistanceParameters* distanceParameters, GameObjectPath& result)
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

	if (sourceField == targetField) return true;

	auto targetHeight = GetPathFindingHeight(targetField);

	if (distanceParameters != nullptr)
	{
		auto sourceHeight = GetPathFindingHeight(sourceField);
		auto maxDistance = distanceParameters->GetValue(sourceHeight, targetHeight);
		if (glm::length2(glm::vec2(targetField - sourceField)) <= maxDistance * maxDistance)
			return true;
	}

	// Checking whether the source/approaching and target nodes are on the same island.
	{
		auto& terrainTree = context.TerrainTree;

		auto startNodeIndex = terrainTree.GetNodeIndexForField(result.SourceField);
		auto startIslandIndex = terrainTree.GetIslandIndex(startNodeIndex);
		assert(startIslandIndex != Core::c_InvalidIndexU);

		auto countFields = m_Level.GetCountFields();
		float maxDistance = (distanceParameters != nullptr) ? distanceParameters->GetMax() : 0;
		int lDistance = (int)std::floor(maxDistance);
		float maxDistanceSqr = maxDistance * maxDistance;

		bool connected = false;
		for (int x = -lDistance; x <= lDistance && !connected; x++)
		{
			for (int z = -lDistance; z <= lDistance; z++)
			{
				auto distanceSqrToTarget = x * x + z * z;

				if (distanceSqrToTarget > maxDistanceSqr) continue;

				auto currentFieldIndex = result.TargetField + glm::ivec2(x, z);

				if (distanceParameters != nullptr)
				{
					auto currentHeight = GetPathFindingHeight(currentFieldIndex);
					auto currentMaxDistance = distanceParameters->GetValue(currentHeight, targetHeight);

					if (distanceSqrToTarget > currentMaxDistance * currentMaxDistance) continue;
				}

				auto nodeIndex = terrainTree.GetNodeIndexForField(currentFieldIndex);
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
			return false;
		}
	}

	switch (m_Algorithm)
	{
		case Algorithm::AStarOnly: SolveWithAStar(context, distanceParameters, result); break;
		case Algorithm::SimpleHierarchicalPathFinder:
			m_SimpleHierarchicalPathFinder.FindPath(context, distanceParameters, result); break;
	}

	return true;
}

void PathFinder::SolveWithAStar(const PathFindingContext& context,
	const HeightDependentDistanceParameters* distanceParameters, GameObjectPath& result)
{
	auto& terrainTree = context.TerrainTree;

	auto searchStartNode = terrainTree.GetNodeIndexForField(result.SourceField);
	auto searchEndNode = terrainTree.GetNodeIndexForField(result.TargetField);

	m_AStar.FindPath(context, searchStartNode, searchEndNode, distanceParameters, m_TempIndices);

	auto countFieldsInPath = m_TempIndices.GetSize();
	for (unsigned i = 0; i < countFieldsInPath; i++)
	{
		auto nodeIndex = m_TempIndices[i];
		auto& fieldData = result.Fields.PushBackPlaceHolder();
		fieldData.TerrainTreeNodeIndex = nodeIndex;
		fieldData.FieldIndex = terrainTree.GetNode(nodeIndex).Start;
	}
}
