// Timeborne/InGame/Model/Terrain/TerrainTree.h

#pragma once

#include <Timeborne/Declarations/CoreDeclarations.h>
#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <Core/Enum.h>
#include <Core/Platform.h>
#include <EngineBuildingBlocks/Math/AABoundingBox.h>

#include <cstdint>
#include <queue>
#include <unordered_map>

// Required for accessing the path handler to use cached computation data:
// the neighbor indices.
class MainApplication;

class Terrain;

// This class implements a complete quadtree for the terrain fields. It is intended to be used for
// multiple use cases, including culling, path finding, object information, time function representation, etc.
class TerrainTree
{
public:

	// We start to use this class in the level editor as well. The following
	// data members are NOT updated:
	//
	// - Height data: Node.MinHeight, Node.MaxHeight
	// - Connectivity data: Node.Flags
	// - Island index
	//
	// The first two could be updated, which would make the most sense
	// if the ray intersection would be reimplemented here and
	// 'FieldHeightQuadTree' would be deleted.
	//
	// @todo: reimplement the ray intersection with this class. This must be
	// done BEFORE implementing functionality in the level editor that
	// accesses the heights.
	//
	// The island indices are only used by the path finding, and it would
	// be inefficient to always recompute them, since a small local change
	// would lead to complete great changes in the island indices.

	enum class NodeFlags : unsigned
	{
		None = 0x00000000,

		// Directions.
		//
		//  +--------+--------+--------+-> X
		//  |        |        |        |
		//  |  0x80  |  0x01  |  0x02  |
		//  |        |        |        |
		//  +--------+--------+--------+
		//  |        |        |        |
		//  |  0x40  |   --   |  0x04  |
		//  |        |        |        |
		//  +--------+--------+--------+
		//  |        |        |        |
		//  |  0x20  |  0x10  |  0x08  |
		//  |        |        |        |
		//  +--------+--------+--------+
		//  |
		//  V Z
		//
		North = 0x00000001,
		NorthEast = 0x00000002,
		East = 0x00000004,
		SouthEast = 0x00000008,
		South = 0x00000010,
		SouthWest = 0x00000020,
		West = 0x00000040,
		NorthWest = 0x00000080,
		AllDirections = 0x000000ff
	};

	static CORE_FORCEINLINE bool HasDirection(NodeFlags flags, unsigned index)
	{
		return ((unsigned)flags) & (1 << index);
	}

	struct Node
	{
		unsigned Parent;
		unsigned Children[4];
		unsigned Neighbors[8]; // Indexing is consistent with the direction flags.
		glm::ivec2 Start;
		glm::ivec2 End;
		float MinHeight, MaxHeight;
		NodeFlags Flags;

		void SerializeSB(Core::ByteVector& bytes) const;
		void DeserializeSB(const unsigned char*& bytes);
	};

private: // Constant data.

	struct ConstantData
	{
		Core::IndexVectorU NeighborIndices;

		void SerializeSB(Core::ByteVector& bytes) const;
		void DeserializeSB(const unsigned char*& bytes);
	};

	static void LoadConstantData(ConstantData& constantData, const std::string& filePath);
	static void SaveConstantData(const ConstantData& constantData, const std::string& filePath);

private:
	const Terrain& m_Terrain;

	// Stores the nodes in level order and there:
	//
	//  +---------+---------+-> X
	//  |         |         |
	//  |    0    |    1    |
	//  |         |         |
	//  +---------+---------+
	//  |         |         |
	//  |    2    |    3    |
	//  |         |         |
	//  +---------+---------+
	//  |
	//  V Z
	//
	// The level order is created by first subdividing only along the longer side.
	// After the sizes along X and Z are the same 2-power, the usual quadtree subdivision
	// is executed.
	//
	Core::SimpleTypeVectorU<Node> m_Nodes;

	// Islands are connected components. SoA with 'm_Nodes'.
	// For inner nodes only set if all indices equal.
	Core::IndexVectorU m_IslandIndices;

	unsigned m_CountLeafs;

	Core::IndexVectorU m_TerrainFieldToNodeIndex;

	mutable std::deque<unsigned> m_NodeIndexQueue; // Temp for multiple functions.

	using LocationHashToIndexMap = std::unordered_map<uint64_t, unsigned>;

	void BuildTree(const MainApplication* application);
	void CreateNodes(bool constantDataExists, LocationHashToIndexMap& locationHashToIndexMap);
	void ComputeNeighbors(bool constantDataExists, const LocationHashToIndexMap& locationHashToIndexMap,
		ConstantData& constantData);
	void ComputeLeafConnectivity();
	void ComputeInnerNodeData();

	void InitializeIslandIndices();
	void ComputeLeafIslands();

	unsigned& GetNodeIndexForFieldRef(const glm::ivec2& fieldIndex);

private: // Neighbor indices.

	static uint64_t GetNodeLocationHash(const glm::ivec2& startIndex,
		const glm::ivec2& size);

	unsigned GetNodeIndex(const LocationHashToIndexMap& locationHashToIndexMap,
		const glm::ivec2& startIndex, const glm::ivec2& size) const;

public:

	explicit TerrainTree(const Terrain& terrain);
	TerrainTree(const Terrain& terrain, const MainApplication* application);

	const Terrain& GetTerrain() const;

	bool IsLeaf(unsigned index) const;
	EngineBuildingBlocks::Math::AABoundingBox GetBoundingBox(unsigned index) const;
	unsigned GetCountNodes() const;
	const Node& GetNode(unsigned index) const;
	glm::ivec2 GetNodeSize(unsigned index) const;

	unsigned GetIslandIndex(unsigned index) const;

	unsigned GetNodeIndexForField(const glm::ivec2& fieldIndex) const;
	unsigned GetNodeIndexForField(const glm::ivec2& fieldIndex, unsigned nodeSize) const;

	void GetNodeIndices(const glm::ivec2& startIndex,
		const glm::ivec2& endIndex, unsigned nodeSize,
		Core::IndexVectorU& nodeIndices) const;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);

public: // Hierarchical culling.

	enum class CullOutputType { Node, Field };

	struct CullParameters
	{
		CullOutputType outputType;
		float maxHeightOffset;

		// Settings for the node output type.
		unsigned minNodeSize;
		unsigned maxNodeSize;
	};

private:

	mutable std::vector<std::deque<unsigned>> m_NodeIndexQueuesForThreads;
	mutable std::vector<Core::IndexVectorU> m_OutputIndicesForThreads;

	mutable CullParameters m_CullParameters{};
	mutable const EngineBuildingBlocks::Math::Plane* m_FrustumPlanes = nullptr;
	mutable int m_CullSize = 0;

	void CullInThread(unsigned threadId,
		unsigned startTaskIndex, unsigned endTaskIndex) const;

public:

	void Cull(Core::ThreadPool& threadPool,
		EngineBuildingBlocks::Graphics::Camera& camera,
		Core::IndexVectorU& outputIndices,
		const CullParameters& parameters) const;
};

UseEnumAsFlagSet(TerrainTree::NodeFlags)