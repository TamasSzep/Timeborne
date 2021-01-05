// Timeborne/InGame/Model/GameObjects/PathFinding/PathFinder.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/PathFinding/PathFinding.h>
#include <Timeborne/InGame/Model/GameObjects/PathFinding/AStar.h>
#include <Timeborne/InGame/Model/GameObjects/PathFinding/SimpleHierarchicalPathFinder.h>

struct GameObjectData;
class GameObjectPose;
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

	void SolveWithAStar(const PathFindingContext& context, 
		const HeightDependentDistanceParameters* distanceParameters, 
		GameObjectPath& result);

	float GetPathFindingHeight(const glm::ivec2& fieldIndex) const;

public:

	PathFinder(const Level& level, const GameObjectData& gameObjectData);
	bool FindPath(GameObjectId objectId, const glm::ivec2& targetField,
		const HeightDependentDistanceParameters* distanceParameters,
		GameObjectPath& result);

	float GetPathFindingHeight(const GameObjectPose& pose) const;
};