// Timeborne/InGame/Model/GameObjects/PathFinding/SimpleHierarchicalPathFinder.cpp

#include <Timeborne/InGame/Model/GameObjects/PathFinding/SimpleHierarchicalPathFinder.h>

#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>
#include <Timeborne/InGame/Model/Level.h>

constexpr unsigned c_StartNodeSize = 16;

void SimpleHierarchicalPathFinder::FindPath(PathFindingContext& context,
	const HeightDependentDistanceParameters* distanceParameters, GameObjectPath& result)
{
	auto& level = context.Level;
	auto& terrain = level.GetTerrain();
	auto& terrainTree = context.TerrainTree;

	auto searchStartNode = terrainTree.GetNodeIndexForField(result.SourceField, c_StartNodeSize);
	auto searchEndNode = terrainTree.GetNodeIndexForField(result.TargetField, c_StartNodeSize);

	// Execute A* from 'searchStartNode' to 'searchEndNode' using neighbors and connectivity information.
	// ...
}
