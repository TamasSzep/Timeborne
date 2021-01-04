// Timeborne/InGame/Model/GameObjects/PathFinding/AStar.cpp

#include <Timeborne/InGame/Model/GameObjects/PathFinding/AStar.h>

#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>

#include <Core/Constants.h>

bool HasSingleNodeSize(const TerrainTree& terrainTree, unsigned nodeIndex1, unsigned nodeIndex2)
{
	auto s1 = terrainTree.GetNodeSize(nodeIndex1);
	auto s2 = terrainTree.GetNodeSize(nodeIndex2);
	return s1 == s2 && s1.x == s1.y;
}

void AStar::ReconstructPath(unsigned endLocalIndex, Core::IndexVectorU& nodeIndices)
{
	for (unsigned i = endLocalIndex; i != 0; i = m_CameFrom[i]) nodeIndices.PushBack(m_Nodes[i].NodeIndex);
	nodeIndices.PushBack(m_Nodes[0].NodeIndex);
	unsigned countNodes = nodeIndices.GetSize();
	unsigned limit = countNodes >> 1;
	unsigned lastIndex = countNodes - 1;
	for (unsigned i = 0; i < limit; i++) std::swap(nodeIndices[i], nodeIndices[lastIndex - i]);
}

void AStar::FindPath(const PathFindingContext& context, unsigned startNodeIndex, unsigned endNodeIndex,
	float maxDistance, Core::IndexVectorU& nodeIndices)
{
#if MEASURE_PATH_FINDING_EXECUTION_TIME
	auto startTime = std::chrono::steady_clock::now();
#endif
#if CREATE_PATH_FINDING_STATISTICS
	int visitCount = 0;
	Core::FastStdSet<unsigned> visitedLocalNodes;
	auto Statistics_Visit = [&](unsigned localIndex) {
		visitCount++;
		visitedLocalNodes.insert(localIndex);
	};
#endif

	auto MakeNodeSortData = [](const PathDistance& totalCost, const PathDistance& heuriticCostToEnd) {
		return NodeSortData{ (float)totalCost, (float)heuriticCostToEnd };
	};

	nodeIndices.Clear();
	if (startNodeIndex == endNodeIndex) return;

	auto& terrainTree = context.TerrainTree;

	bool isApproachingPath = (maxDistance > 0.0f);
	float maxDistanceSqr = maxDistance * maxDistance;
	auto endField = terrainTree.GetNode(endNodeIndex).Start;
	auto isCloseEnoughToEndField = [&terrainTree, endField, maxDistanceSqr](uint32_t currentNodeIndex) {
		auto currentField = terrainTree.GetNode(currentNodeIndex).Start;
		auto offset = endField - currentField;
		return offset.x * offset.x + offset.y * offset.y <= maxDistanceSqr;
	};

	assert(HasSingleNodeSize(terrainTree, startNodeIndex, endNodeIndex));

	auto startHeuristicDistance = GetPathFindingHeuristicGuess(context, startNodeIndex, endNodeIndex);

	m_Nodes.Clear();
	auto& startLocalNode = m_Nodes.PushBackPlaceHolder();
	startLocalNode.NodeIndex = startNodeIndex;
	startLocalNode.CostFromStart = PathDistance::Zero();
	startLocalNode.HeuriticCostToEnd = startHeuristicDistance;

	if (m_NodeToDataMap.GetSize() != terrainTree.GetCountNodes())
	{
		m_NodeToDataMap.Clear();
		m_NodeToDataMap.PushBack(Core::c_InvalidIndexU, terrainTree.GetCountNodes());
	}
	m_NodeToDataMap[startNodeIndex] = 0U;

	m_OpenSet.clear();
	m_OpenSet.insert(std::make_pair(MakeNodeSortData(startHeuristicDistance, startHeuristicDistance), 0U));

	m_CameFrom.Clear();
	m_CameFrom.PushBack(Core::c_InvalidIndexU);

	while (!m_OpenSet.empty())
	{
		auto currentIt = m_OpenSet.begin();
		auto current = *currentIt;
		m_OpenSet.erase(currentIt);
		auto currentLocalIndex = current.second;
		auto& currentData = m_Nodes[currentLocalIndex];
		auto currentNodeIndex = currentData.NodeIndex;
		auto currentCostFromStart = currentData.CostFromStart;

#if CREATE_PATH_FINDING_STATISTICS
		Statistics_Visit(currentLocalIndex);
#endif

		if (currentNodeIndex == endNodeIndex || (isApproachingPath && isCloseEnoughToEndField(currentNodeIndex)))
		{
			ReconstructPath(currentLocalIndex, nodeIndices);
			break;
		}

		auto& currentNodeData = terrainTree.GetNode(currentNodeIndex);
		auto nodeFlags = currentNodeData.Flags;

		for (unsigned d = 0; d < 8; d++)
		{
			if (TerrainTree::HasDirection(nodeFlags, d))
			{
				unsigned neighborNodeIndex = currentNodeData.Neighbors[d];
				
				auto currentToNeighborDistance = GetPathFindingNodeDistance(context, currentNodeIndex, neighborNodeIndex);
				auto newCostFromStateForNeighbor = currentCostFromStart + currentToNeighborDistance;
				
				auto localIndex = m_NodeToDataMap[neighborNodeIndex];
				if (localIndex == Core::c_InvalidIndexU)
				{
					auto heuristicCost = GetPathFindingHeuristicGuess(context, neighborNodeIndex, endNodeIndex);
					auto totalCost = newCostFromStateForNeighbor + heuristicCost;

					localIndex = m_Nodes.GetSize();
					auto& neighborData = m_Nodes.PushBackPlaceHolder();
					neighborData.NodeIndex = neighborNodeIndex;
					neighborData.CostFromStart = newCostFromStateForNeighbor;
					neighborData.HeuriticCostToEnd = heuristicCost;

					m_NodeToDataMap[neighborNodeIndex] = localIndex;

					m_OpenSet.insert(std::make_pair(MakeNodeSortData(totalCost, heuristicCost), localIndex));

					m_CameFrom.PushBack(currentLocalIndex);
				}
				else
				{
					auto& neighborData = m_Nodes[localIndex];
					if (newCostFromStateForNeighbor < neighborData.CostFromStart)
					{
						const auto& heuristicCost = neighborData.HeuriticCostToEnd;
						auto oldTotalCost = neighborData.CostFromStart + heuristicCost;
						auto newTotalCost = newCostFromStateForNeighbor + heuristicCost;

						neighborData.CostFromStart = newCostFromStateForNeighbor;

						m_CameFrom[localIndex] = currentLocalIndex;

						// Updating the cost in the open set.
						{
							auto searchedElement = MakeNodeSortData(oldTotalCost, heuristicCost);
							auto oIt = m_OpenSet.lower_bound(searchedElement);
							auto oEnd = m_OpenSet.end();
							bool found = false;
							for (; oIt != oEnd && oIt->first == searchedElement; ++oIt)
							{
								if (oIt->second == localIndex) { found = true; break; }
							}
							if(found) m_OpenSet.erase(oIt);
						}
						m_OpenSet.insert(std::make_pair(MakeNodeSortData(newTotalCost, heuristicCost), localIndex));
					}
				}
			}
		}
	}

	// Reverting all changes in 'm_NodeToDataMap'.
	auto countNodes = m_Nodes.GetSize();
	for (unsigned i = 0; i < countNodes; i++)
	{
		m_NodeToDataMap[m_Nodes[i].NodeIndex] = Core::c_InvalidIndexU;
	}

#if MEASURE_PATH_FINDING_EXECUTION_TIME
	auto endTime = std::chrono::steady_clock::now();
	int procTime = (int)std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
	static int countMeasurements = 0;
	static int sumProcTime = 0;
	countMeasurements++;
	sumProcTime += procTime;
	int avgProcTime = sumProcTime / countMeasurements;
#endif

#if MEASURE_PATH_FINDING_EXECUTION_TIME || CREATE_PATH_FINDING_STATISTICS

#if !MEASURE_PATH_FINDING_EXECUTION_TIME
	int procTime = 0;
	int avgProcTime = 0;
#endif

#if CREATE_PATH_FINDING_STATISTICS
	int countVisitedNodes = (int)visitedLocalNodes.size();
#else
	int visitCount = 0;
	int countVisitedNodes = 0;
#endif
	char startNodeBuffer[32], endNodeBuffer[32];
	auto printNodeInfo = [&terrainTree](unsigned nodeIndex, char* buffer) {
		auto& node = terrainTree.GetNode(nodeIndex);
		if (node.Start == node.End) snprintf(buffer, 32, "(%d, %d)", node.Start.x, node.Start.y);
		else snprintf(buffer, 32, "[%d, %d]x[%d, %d]", node.Start.x, node.End.x, node.Start.y, node.End.y);
	};
	printNodeInfo(startNodeIndex, startNodeBuffer);
	printNodeInfo(endNodeIndex, endNodeBuffer);
	printf("AStar stat: route: %s -> %s, visits: %d, nodes: %d, time: %d us, avg time: %d us\n", startNodeBuffer, endNodeBuffer,
		visitCount, countVisitedNodes, procTime, avgProcTime);
#endif
}
