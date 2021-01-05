// Timeborne/InGame/Model/GameObjects/PathFinding/AStar.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/PathFinding/PathFinding.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/Comparison.h>
#include <Core/SingleElementPoolAllocator.hpp>

struct HeightDependentDistanceParameters;

class AStar
{
	// Storing the node with the lowest heuristic value in the lowest total value class.
	// We won't compute anything from this data so it ca be stored as floats.
	struct NodeSortData
	{
		float TotalCost;
		float HeuriticCostToEnd;

		inline constexpr bool operator==(const NodeSortData& other) const
		{
			NumericalEqualCompareBlock(TotalCost);
			return HeuriticCostToEnd == other.HeuriticCostToEnd;
		}

		inline constexpr bool operator<(const NodeSortData& other) const
		{
			NumericalLessCompareBlock(TotalCost);
			return HeuriticCostToEnd < other.HeuriticCostToEnd;
		}
	};

	struct NodeData
	{
		unsigned NodeIndex;
		PathDistance CostFromStart;     // g function: getting lower
		PathDistance HeuriticCostToEnd; // h function: constant

		// Note that the total cost - f function - is not stored explicitly,
		// since f = g + h always holds.
	};

	Core::SimpleTypeVectorU<NodeData> m_Nodes;

	// Node index -> Index in 'm_Nodes'. SoA with the terrain tree's nodes.
	Core::IndexVectorU m_NodeToDataMap;

	// TotalCost -> Index in 'm_Nodes'.
	Core::FastStdMultiMap<NodeSortData, unsigned> m_OpenSet;

	// SoA with m_Nodes. Indexed by and stores local indices.
	Core::SimpleTypeVectorU<unsigned> m_CameFrom;

	void ReconstructPath(unsigned endNodeIndex, Core::IndexVectorU& nodeIndices);

public:
	void FindPath(const PathFindingContext& context, 
		unsigned startNodeIndex, unsigned endNodeIndex,
		const HeightDependentDistanceParameters* distanceParameters,
		Core::IndexVectorU& nodeIndices);
};
