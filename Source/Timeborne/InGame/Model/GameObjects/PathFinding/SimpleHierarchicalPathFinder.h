// Timeborne/InGame/Model/GameObjects/PathFinding/SimpleHierarchicalPathFinder.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/PathFinding/PathFinding.h>

class SimpleHierarchicalPathFinder
{
public:
	void FindPath(PathFindingContext& context, GameObjectPath& result);
};