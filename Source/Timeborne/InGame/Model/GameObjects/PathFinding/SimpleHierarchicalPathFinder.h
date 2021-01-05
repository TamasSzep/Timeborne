// Timeborne/InGame/Model/GameObjects/PathFinding/SimpleHierarchicalPathFinder.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/PathFinding/PathFinding.h>

struct HeightDependentDistanceParameters;

class SimpleHierarchicalPathFinder
{
public:
	void FindPath(PathFindingContext& context,
		const HeightDependentDistanceParameters* distanceParameters,
		GameObjectPath& result);
};