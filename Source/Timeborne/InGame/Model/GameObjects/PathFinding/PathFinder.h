// Timeborne/InGame/Model/GameObjects/PathFinding/PathFinder.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/PathFinding/PathFinding.h>
#include <Timeborne/InGame/Model/GameObjects/PathFinding/AStar.h>
#include <Timeborne/InGame/Model/GameObjects/PathFinding/SimpleHierarchicalPathFinder.h>

struct GameObjectData;
class Level;

class PathFinder
{
	const Level& m_Level;
	const GameObjectData& m_GameObjectData;

	AStar m_AStar;
	SimpleHierarchicalPathFinder m_SimpleHierarchicalPathFinder;

	Core::IndexVectorU m_TempIndices;

	enum class Algorithm
	{
		AStarOnly,
		SimpleHierarchicalPathFinder
	} m_Algorithm = Algorithm::AStarOnly;

	void SolveWithAStar(const PathFindingContext& context, GameObjectPath& result);

public:

	PathFinder(const Level& level, const GameObjectData& gameObjectData);
	void FindPath(GameObjectId objectId, const glm::ivec2& targetField,
		float maxDistance, GameObjectPath& result);
};