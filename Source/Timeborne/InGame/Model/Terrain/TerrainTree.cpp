// Timeborne/InGame/Model/Terrain/TerrainTree.cpp

#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>

#include <Timeborne/InGame/Model/Terrain/Terrain.h>
#include <Timeborne/MainApplication.h>

#include <Core/System/Filesystem.h>
#include <Core/System/SimpleIO.h>
#include <Core/System/ThreadPool.h>
#include <Core/Constants.h>
#include <Core/SimpleBinarySerialization.hpp>
#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Math;

CORE_FORCEINLINE glm::ivec2 GetNodeSize(const TerrainTree::Node& node)
{
	return node.End - node.Start + 1;
}

CORE_FORCEINLINE void PushChildNodes(const TerrainTree::Node& node, std::deque<unsigned>& nodeIndexQueue)
{
	for (int c = 0; c < 4; c++)
	{
		auto childIndex = node.Children[c];
		if (childIndex != Core::c_InvalidIndexU)
		{
			nodeIndexQueue.push_back(childIndex);
		}
	}
}

CORE_FORCEINLINE bool isValidField(const glm::ivec2& fieldIndex, const glm::uvec2& countFields)
{
	return fieldIndex.x >= 0 && fieldIndex.x < (int)countFields.x
		&& fieldIndex.y >= 0 && fieldIndex.y < (int)countFields.y;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TerrainTree::TerrainTree(const Terrain& terrain)
	: m_Terrain(terrain)
{
}

TerrainTree::TerrainTree(const Terrain& terrain, const MainApplication* application)
	: m_Terrain(terrain)
{
	BuildTree(application);
}

inline void Subdivide(const glm::ivec2& start, const glm::ivec2& end, int& countChildren, glm::ivec2* starts, glm::ivec2* ends)
{
	auto size = end - start + 1;
	int xLimit = (size.x >= size.y && size.x > 1) ? 2 : 1;
	int zLimit = (size.y >= size.x && size.y > 1) ? 2 : 1;
	auto hSize = size / 2;
	countChildren = 0;
	for (int z = 0; z < zLimit; z++)
	{
		int zStart = start.y + z * hSize.y;
		int zEnd = end.y + (z + 1 - zLimit) * hSize.y;
		for (int x = 0; x < xLimit; x++)
		{
			int xStart = start.x + x * hSize.x;
			int xEnd = end.x + (x + 1 - xLimit) * hSize.x;

			starts[countChildren] = { xStart, zStart };
			ends[countChildren] = { xEnd, zEnd };
			countChildren++;
		}
	}
}

void TerrainTree::BuildTree(const MainApplication* application)
{
	auto countFields = m_Terrain.GetCountFields();
	char dataPath[256];
	std::snprintf(dataPath, 256, "TerrainTree_%dx%d.bin", countFields.x, countFields.y);
	auto constantDataPath = application->GetPathHandler()->GetPathFromBuiltResourcesDirectory(dataPath);
	bool constantDataExists = Core::FileExists(constantDataPath);

	ConstantData constantData;
	if (constantDataExists) LoadConstantData(constantData, constantDataPath);

	LocationHashToIndexMap locationHashToIndexMap;
	CreateNodes(constantDataExists, locationHashToIndexMap);
	ComputeNeighbors(constantDataExists, locationHashToIndexMap, constantData);
	ComputeLeafConnectivity();
	ComputeInnerNodeData();

	InitializeIslandIndices();
	ComputeLeafIslands();

	if (!constantDataExists) SaveConstantData(constantData, constantDataPath);
}

void TerrainTree::ConstantData::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, NeighborIndices);
}

void TerrainTree::ConstantData::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, NeighborIndices);
}

void TerrainTree::LoadConstantData(ConstantData& constantData, const std::string& filePath)
{
	Core::ByteVector bytes;
	Core::ReadAllBytes(filePath, bytes);
	Core::StartDeserializeSB(bytes, constantData);
}

void TerrainTree::SaveConstantData(const ConstantData& constantData, const std::string& filePath)
{
	Core::ByteVector bytes;
	Core::StartSerializeSB(bytes, constantData);
	Core::WriteAllBytes(filePath, bytes);
}

void TerrainTree::CreateNodes(bool constantDataExists, LocationHashToIndexMap& locationHashToIndexMap)
{
	auto fields = m_Terrain.GetFields();
	auto countFields = m_Terrain.GetCountFields();

	m_CountLeafs = countFields.x * countFields.y;

	m_TerrainFieldToNodeIndex.Resize(m_CountLeafs);

	m_Nodes.Clear();
	Node rootNode;
	rootNode.Parent = Core::c_InvalidIndexU;
	std::fill(rootNode.Children, rootNode.Children + 4, Core::c_InvalidIndexU);
	rootNode.Start = { 0, 0 };
	rootNode.End = { glm::ivec2(countFields) - 1 };
	rootNode.MinHeight = 0.0f;
	rootNode.MaxHeight = 0.0f;
	rootNode.Flags = NodeFlags::None;
	std::fill(rootNode.Neighbors, rootNode.Neighbors + 8, Core::c_InvalidIndexU);
	m_Nodes.PushBack(rootNode);
	if (!constantDataExists)
	{
		locationHashToIndexMap.insert(std::make_pair(GetNodeLocationHash(rootNode.Start, countFields), 0U));
	}

	std::queue<unsigned> nodesToProcess;
	if (countFields.x > 1 || countFields.y > 1)
	{
		nodesToProcess.push(0);
	}

	while (!nodesToProcess.empty())
	{
		auto nodeIndex = nodesToProcess.front();
		nodesToProcess.pop();

		// Creating children.
		int countChildren;
		glm::ivec2 starts[4];
		glm::ivec2 ends[4];
		Subdivide(m_Nodes[nodeIndex].Start, m_Nodes[nodeIndex].End, countChildren, starts, ends);

		for (int i = 0; i < countChildren; i++)
		{
			// The storage guarentees indices, therefore reaccess is necessary.
			auto& node = m_Nodes[nodeIndex];

			unsigned childNodeIndex = m_Nodes.GetSize();

			// Creating child node.
			Node childNode;
			childNode.Parent = nodeIndex;
			std::fill(childNode.Children, childNode.Children + 4, Core::c_InvalidIndexU);
			childNode.Start = starts[i];
			childNode.End = ends[i];
			childNode.Flags = NodeFlags::None;
			std::fill(childNode.Neighbors, childNode.Neighbors + 8, Core::c_InvalidIndexU);

			bool isLeaf = (childNode.Start == childNode.End);

			glm::vec2 heightMinMax;
			if (isLeaf)
			{
				heightMinMax = Terrain::GetFieldHeightMinMax(fields, countFields, childNode.Start.x, childNode.Start.y);

				GetNodeIndexForFieldRef(childNode.Start) = childNodeIndex;
			}

			childNode.MinHeight = heightMinMax.x;
			childNode.MaxHeight = heightMinMax.y;

			// Setting the child for the parent.
			node.Children[i] = childNodeIndex;

			if (!constantDataExists)
			{
				locationHashToIndexMap.insert(std::make_pair(
					GetNodeLocationHash(childNode.Start, ::GetNodeSize(childNode)),
					m_Nodes.GetSize()));
			}
			m_Nodes.PushBack(childNode);

			if (!isLeaf)
			{
				nodesToProcess.push(childNodeIndex);
			}
		}
	}

	assert(constantDataExists || (unsigned)locationHashToIndexMap.size() == m_Nodes.GetSize());
}

void TerrainTree::ComputeNeighbors(bool constantDataExists, const LocationHashToIndexMap& locationHashToIndexMap,
	ConstantData& constantData)
{
	unsigned countNodes = m_Nodes.GetSize();

	auto copySize = sizeof(unsigned) * 8;
	if (!constantDataExists) constantData.NeighborIndices.Resize(countNodes * 8);
	auto constDataPtr = (uint8_t*)constantData.NeighborIndices.GetArray();

	if (constantDataExists)
	{	
		for (unsigned i = 0; i < countNodes; i++, constDataPtr += copySize)
		{
			std::memcpy(m_Nodes[i].Neighbors, constDataPtr, copySize);
		}
	}
	else
	{
		auto GetNodeIndexL = [this, &locationHashToIndexMap](const glm::ivec2& startIndex, const glm::ivec2& nodeSize) {
			return GetNodeIndex(locationHashToIndexMap, startIndex, nodeSize);
		};

		for (unsigned i = 0; i < countNodes; i++, constDataPtr += copySize)
		{
			auto& node = m_Nodes[i];
			auto start = node.Start;
			auto nodeSize = ::GetNodeSize(node);
			node.Neighbors[0] = GetNodeIndexL(start + glm::ivec2(0, -nodeSize.y), nodeSize);
			node.Neighbors[1] = GetNodeIndexL(start + glm::ivec2(nodeSize.x, -nodeSize.y), nodeSize);
			node.Neighbors[2] = GetNodeIndexL(start + glm::ivec2(nodeSize.x, 0), nodeSize);
			node.Neighbors[3] = GetNodeIndexL(start + glm::ivec2(nodeSize.x, nodeSize.y), nodeSize);
			node.Neighbors[4] = GetNodeIndexL(start + glm::ivec2(0, nodeSize.y), nodeSize);
			node.Neighbors[5] = GetNodeIndexL(start + glm::ivec2(-nodeSize.x, nodeSize.y), nodeSize);
			node.Neighbors[6] = GetNodeIndexL(start + glm::ivec2(-nodeSize.x, 0), nodeSize);
			node.Neighbors[7] = GetNodeIndexL(start + glm::ivec2(-nodeSize.x, -nodeSize.y), nodeSize);

			std::memcpy(constDataPtr, m_Nodes[i].Neighbors, copySize);
		}
	}
}

void TerrainTree::ComputeLeafConnectivity()
{
	auto fields = m_Terrain.GetFields();
	auto countFields = m_Terrain.GetCountFields();
	auto getHeights = [fields, &countFields](const glm::ivec2& fieldIndex) {
		return Terrain::GetFieldHeights(fields, countFields, fieldIndex.x, fieldIndex.y);
	};
	glm::ivec2 lastFieldIndex = glm::ivec2(countFields) - 1;

	auto addFlagToNeighbor = [this](const Node& node, unsigned nIndex, NodeFlags flag) {
		auto nNodeIndex = node.Neighbors[nIndex];
		if (nNodeIndex != Core::c_InvalidIndexU) m_Nodes[nNodeIndex].Flags |= flag;
	};

	unsigned countNodes = m_Nodes.GetSize();
	unsigned countInnerNodes = countNodes - m_CountLeafs;
	for (unsigned i = countInnerNodes; i < countNodes; i++)
	{
		auto& node = m_Nodes[i];
		auto fieldIndex = node.Start;
		auto heights = getHeights(fieldIndex);

		if (fieldIndex.y > 0)
		{
			auto neighborHeights = getHeights(fieldIndex + glm::ivec2(0, -1));
			if (heights[3] == neighborHeights[0] && heights[2] == neighborHeights[1])
			{
				node.Flags |= NodeFlags::North;
				addFlagToNeighbor(node, 0, NodeFlags::South);
			}
		}
		if (fieldIndex.x < lastFieldIndex.x && fieldIndex.y > 0)
		{
			float height = heights[2];
			auto diagHeights = getHeights(fieldIndex + glm::ivec2(1, -1));
			auto northHeights = getHeights(fieldIndex + glm::ivec2(0, -1));
			auto eastHeights = getHeights(fieldIndex + glm::ivec2(1, 0));
			if (diagHeights[0] == height && northHeights[1] == height && eastHeights[3] == height)
			{
				node.Flags |= NodeFlags::NorthEast;
				addFlagToNeighbor(node, 1, NodeFlags::SouthWest);
			}
		}
		if (fieldIndex.x < lastFieldIndex.x)
		{
			auto neighborHeights = getHeights(fieldIndex + glm::ivec2(1, 0));
			if (heights[1] == neighborHeights[0] && heights[2] == neighborHeights[3])
			{
				node.Flags |= NodeFlags::East;
				addFlagToNeighbor(node, 2, NodeFlags::West);
			}
		}
		if (fieldIndex.x < lastFieldIndex.x && fieldIndex.y < lastFieldIndex.y)
		{
			auto height = heights[1];
			auto diagHeights = getHeights(fieldIndex + glm::ivec2(1, 1));
			auto southHeights = getHeights(fieldIndex + glm::ivec2(0, 1));
			auto eastHeights = getHeights(fieldIndex + glm::ivec2(1, 0));
			if (diagHeights[3] == height && southHeights[2] == height && eastHeights[0] == height)
			{
				node.Flags |= NodeFlags::SouthEast;
				addFlagToNeighbor(node, 3, NodeFlags::NorthWest);
			}
		}
	}
}

void TerrainTree::InitializeIslandIndices()
{
	m_IslandIndices.Clear();
	m_IslandIndices.PushBack(Core::c_InvalidIndexU, m_Nodes.GetSize());
}

void TerrainTree::ComputeLeafIslands()
{
	unsigned islandIndex = 0;

	unsigned countNodes = m_Nodes.GetSize();
	unsigned countInnerNodes = countNodes - m_CountLeafs;
	for (unsigned startNodeIndex = countInnerNodes; startNodeIndex < countNodes; startNodeIndex++)
	{
		if (m_IslandIndices[startNodeIndex] == Core::c_InvalidIndexU)
		{
			m_IslandIndices[startNodeIndex] = islandIndex;

			assert(m_NodeIndexQueue.empty());
			m_NodeIndexQueue.push_back(startNodeIndex);

			while (!m_NodeIndexQueue.empty())
			{
				unsigned nodeIndex = m_NodeIndexQueue.front();
				m_NodeIndexQueue.pop_front();

				auto& nodeData = m_Nodes[nodeIndex];
				auto nodeFlags = nodeData.Flags;
				const unsigned* neighbours = nodeData.Neighbors;

				for (unsigned d = 0; d < 8; d++)
				{
					if (HasDirection(nodeFlags, d))
					{
						unsigned neighborNodeIndex = neighbours[d];
						if (m_IslandIndices[neighborNodeIndex] == Core::c_InvalidIndexU)
						{
							m_IslandIndices[neighborNodeIndex] = islandIndex;
							m_NodeIndexQueue.push_back(neighborNodeIndex);
						}
					}
				}
			}

			islandIndex++;
		}
	}
}

void TerrainTree::ComputeInnerNodeData()
{
	constexpr NodeFlags West0 = NodeFlags::West | NodeFlags::SouthWest;
	constexpr NodeFlags North0 = NodeFlags::North | NodeFlags::NorthEast;
	constexpr NodeFlags North1 = NodeFlags::North | NodeFlags::NorthWest;
	constexpr NodeFlags East1 = NodeFlags::East | NodeFlags::SouthEast;
	constexpr NodeFlags East3 = NodeFlags::East | NodeFlags::NorthEast;
	constexpr NodeFlags South3 = NodeFlags::South | NodeFlags::SouthWest;
	constexpr NodeFlags South2 = NodeFlags::South | NodeFlags::SouthEast;
	constexpr NodeFlags West2 = NodeFlags::West | NodeFlags::NorthWest;

	auto diagonalFlag = [](NodeFlags flags, NodeFlags target) {
		return flags & target;
	};
	auto sideFlag = [](NodeFlags flags, NodeFlags mask, NodeFlags target) { 
		return ((bool)(flags & mask) ? target : NodeFlags::None);
	};

	unsigned countInnerNodes = m_Nodes.GetSize() - m_CountLeafs;
	for (int i = countInnerNodes - 1; i >= 0; i--)
	{
		auto& node = m_Nodes[i];
		auto children = (const unsigned*)node.Children;

		// Height data.
		float minHeight = std::numeric_limits<float>::max();
		float maxHeight = std::numeric_limits<float>::lowest();
		for (int c = 0; c < 4; c++)
		{
			auto childNodeIndex = children[c];
			if (childNodeIndex != Core::c_InvalidIndexU)
			{
				auto& childNode = m_Nodes[childNodeIndex];
				if (childNode.MinHeight < minHeight) minHeight = childNode.MinHeight;
				if (childNode.MaxHeight > maxHeight) maxHeight = childNode.MaxHeight;
			}
		}
		node.MinHeight = minHeight;
		node.MaxHeight = maxHeight;

		// Connectivity.
		unsigned c0, c1, c2, c3;
		auto nodeSize = ::GetNodeSize(node);
		if (nodeSize.x != nodeSize.y)
		{
			if (nodeSize.x > nodeSize.y)
			{
				c0 = 0; c1 = 1; c2 = 0; c3 = 1;
			}
			else
			{
				c0 = 0; c1 = 0; c2 = 1; c3 = 1;
			}
		}
		else
		{
			c0 = 0; c1 = 1; c2 = 2; c3 = 3;
		}

		auto flags0 = m_Nodes[children[c0]].Flags;
		auto flags1 = m_Nodes[children[c1]].Flags;
		auto flags2 = m_Nodes[children[c2]].Flags;
		auto flags3 = m_Nodes[children[c3]].Flags;

		node.Flags |= sideFlag(flags0, West0, NodeFlags::West)
			| diagonalFlag(flags0, NodeFlags::NorthWest)
			| sideFlag(flags0, North0, NodeFlags::North)
			| sideFlag(flags1, North1, NodeFlags::North)
			| diagonalFlag(flags1, NodeFlags::NorthEast)
			| sideFlag(flags1, East1, NodeFlags::East)
			| sideFlag(flags3, East3, NodeFlags::East)
			| diagonalFlag(flags3, NodeFlags::SouthEast)
			| sideFlag(flags3, South3, NodeFlags::South)
			| sideFlag(flags2, South2, NodeFlags::South)
			| diagonalFlag(flags2, NodeFlags::SouthWest)
			| sideFlag(flags2, West2, NodeFlags::West);
	}
}

const Terrain& TerrainTree::GetTerrain() const
{
	return m_Terrain;
}

unsigned TerrainTree::GetCountNodes() const
{
	return m_Nodes.GetSize();
}

const TerrainTree::Node& TerrainTree::GetNode(unsigned index) const
{
	return m_Nodes[index];
}

glm::ivec2 TerrainTree::GetNodeSize(unsigned index) const
{
	return ::GetNodeSize(m_Nodes[index]);
}

unsigned TerrainTree::GetIslandIndex(unsigned index) const
{
	return m_IslandIndices[index];
}

bool TerrainTree::IsLeaf(unsigned index) const
{
	return index + m_CountLeafs < m_Nodes.GetSize();
}

EngineBuildingBlocks::Math::AABoundingBox TerrainTree::GetBoundingBox(unsigned index) const
{
	auto& node = m_Nodes[index];
	EngineBuildingBlocks::Math::AABoundingBox box;
	box.Minimum = { node.Start.x, node.MinHeight, node.Start.y };
	box.Maximum = { (node.End.x + 1), node.MaxHeight, (node.End.y + 1) };
	return box;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class CullResult
{
	Outside, Intersects, Contains
};

CullResult CullAABBWithSidePlanes(const Math::Plane* frustumPlanes, const AABoundingBox& box)
{
	auto result = CullResult::Contains;

	auto size = box.Maximum - box.Minimum;
	auto sizePtr = glm::value_ptr(size);

	// Culling only the 4 side planes.
	for (unsigned i = 0; i < 4U; i++)
	{
		auto& plane = frustumPlanes[i];
		auto& planeNormal = plane.Normal;
		auto planeNormalPtr = glm::value_ptr(planeNormal);

		float min = glm::dot(planeNormal, box.Minimum) + plane.D;
		float max = min;

		auto extend = [planeNormalPtr, sizePtr, &min, &max](unsigned j) {
			float dotComp = planeNormalPtr[j] * sizePtr[j];
			if (dotComp >= 0.0) max += dotComp;
			else min += dotComp;
		};

		extend(0);
		extend(1);
		extend(2);

		if (min > 0.0f) return CullResult::Outside;
		if (max > 0.0f) result = CullResult::Intersects;
	}

	return result;
}

CORE_FORCEINLINE bool NodeSizeMatchesCullingSize(const TerrainTree::Node& node, int cullSize)
{
	auto size = GetNodeSize(node);
	return (size.x <= cullSize || size.y <= cullSize);
}

inline void PushNodesWithoutCulling(unsigned startNodeIndex, const TerrainTree& terrainTree, int cullSize,
	std::deque<unsigned>& nodeIndexQueue, Core::IndexVectorU& outputIndices)
{
	// Reusing the same deque for the iterations.

	auto dequeStartSize = nodeIndexQueue.size();
	nodeIndexQueue.push_back(startNodeIndex);

	auto currentIndexInQueue = (unsigned)dequeStartSize;

	while (currentIndexInQueue < nodeIndexQueue.size())
	{
		auto nodeIndex = nodeIndexQueue[currentIndexInQueue++];
		auto& node = terrainTree.GetNode(nodeIndex);
		
		if (NodeSizeMatchesCullingSize(node, cullSize))
		{
			outputIndices.PushBack(nodeIndex);
		}
		else
		{
			PushChildNodes(node, nodeIndexQueue);
		}
	}

	nodeIndexQueue.resize(dequeStartSize);
}

inline void CullNodes(const TerrainTree& terrainTree, const TerrainTree::CullParameters& parameters,
	const Plane* frustumPlanes, int cullSize, const glm::uvec2& countFields,
	std::deque<unsigned>& nodeIndexQueue, Core::IndexVectorU& outputIndices)
{
	auto nodeIndex = nodeIndexQueue.front();
	nodeIndexQueue.pop_front();

	auto& node = terrainTree.GetNode(nodeIndex);

	auto box = terrainTree.GetBoundingBox(nodeIndex);
	box.Maximum.y += parameters.maxHeightOffset;

	auto cullResult = CullAABBWithSidePlanes(frustumPlanes, box);

	if (cullResult == CullResult::Outside) return;

	bool sizeMatches = NodeSizeMatchesCullingSize(node, cullSize);
	if (cullResult == CullResult::Contains || sizeMatches)
	{
		if (parameters.outputType == TerrainTree::CullOutputType::Node)
		{
			if (sizeMatches) // This is just an optimization to use the deque less.
			{
				outputIndices.PushBack(nodeIndex);
			}
			else
			{
				PushNodesWithoutCulling(nodeIndex, terrainTree, cullSize, nodeIndexQueue, outputIndices);
			}
		}
		else
		{
			for (int z = node.Start.y; z <= node.End.y; z++)
			{
				unsigned startIndex = z * countFields.x;
				for (int x = node.Start.x; x <= node.End.x; x++)
				{
					outputIndices.PushBack(startIndex + x);
				}
			}
		}
	}
	else
	{
		PushChildNodes(node, nodeIndexQueue);
	}
}

void TerrainTree::Cull(Core::ThreadPool& threadPool, EngineBuildingBlocks::Graphics::Camera& camera,
	Core::IndexVectorU& outputIndices, const CullParameters& parameters) const
{
	// Initializing containers with the thread pool.
	if (m_NodeIndexQueuesForThreads.empty())
	{
		auto countThreads = threadPool.GetCountThreads();
		for (unsigned i = 0; i < countThreads; i++)
		{
			m_NodeIndexQueuesForThreads.emplace_back();
			m_OutputIndicesForThreads.emplace_back();
		}
	}

	// Tests suggest that the zoom factor can be ignored here according to cull size exponent.

	constexpr int cTaskPackageSize = 1;
	constexpr int cThreadStartNodeLimit = 16;

#ifdef _DEBUG
	constexpr int cCullSizeExponent = 4;
#else
	constexpr int cCullSizeExponent = 0;
#endif

	int cullSize = 1 << cCullSizeExponent;
	if (parameters.outputType == CullOutputType::Node)
	{
		cullSize = std::min(std::max(cullSize, (int)parameters.minNodeSize), (int)parameters.maxNodeSize);
	}

	auto& countFields = m_Terrain.GetCountFields();

	if (countFields.x == 0 || countFields.y == 0) return;

	assert(m_NodeIndexQueue.empty());
	m_NodeIndexQueue.push_back(0);

	auto frustumPlanes = camera.GetViewFrustum().GetPlanes().Planes;

	outputIndices.Clear();

	while (!m_NodeIndexQueue.empty())
	{
		CullNodes(*this, parameters, frustumPlanes, cullSize, countFields, m_NodeIndexQueue, outputIndices);

		// Stopping if there are too many nodes. Stopping at a point where all nodes that has to be processed
		// are on a single level, so that sorting is only conditionally necessary in the result merging step.
		if (m_NodeIndexQueue.size() >= cThreadStartNodeLimit
			&& GetNodeSize(m_NodeIndexQueue.front()) == GetNodeSize(m_NodeIndexQueue.back())) break;
	}

	if (!m_NodeIndexQueue.empty())
	{
		auto countThreads = threadPool.GetCountThreads();
		auto countTasks = (unsigned)m_NodeIndexQueue.size();

		m_CullParameters = parameters;
		m_FrustumPlanes = frustumPlanes;
		m_CullSize = cullSize;

		for (auto& cOutputIndices : m_OutputIndicesForThreads)
		{
			cOutputIndices.Clear();
		}

		threadPool.ExecuteWithDynamicScheduling(countTasks, &TerrainTree::CullInThread, this, cTaskPackageSize);

		m_NodeIndexQueue.clear();

		// Concatenating results.
		for (unsigned i = 0; i < countThreads; i++)
		{
			outputIndices.PushBack(m_OutputIndicesForThreads[i]);
		}
		if (parameters.outputType == CullOutputType::Node && parameters.minNodeSize != parameters.maxNodeSize)
		{
			std::sort(outputIndices.GetArray(), outputIndices.GetEndPointer());
		}
	}
}

void TerrainTree::CullInThread(unsigned threadId, unsigned startTaskIndex, unsigned endTaskIndex) const
{
	auto& nodeIndexQueue = m_NodeIndexQueuesForThreads[threadId];
	auto& outputIndices = m_OutputIndicesForThreads[threadId];
	for (unsigned taskIndex = startTaskIndex; taskIndex < endTaskIndex; taskIndex++)
	{
		auto nodeIndex = m_NodeIndexQueue[taskIndex];

		assert(nodeIndexQueue.empty());
		nodeIndexQueue.push_back(nodeIndex);

		while (!nodeIndexQueue.empty())
		{
			CullNodes(*this, m_CullParameters, m_FrustumPlanes, m_CullSize, m_Terrain.GetCountFields(),
				nodeIndexQueue, outputIndices);
		}
	}
}

unsigned& TerrainTree::GetNodeIndexForFieldRef(const glm::ivec2& fieldIndex)
{
	auto& countFields = m_Terrain.GetCountFields();
	return m_TerrainFieldToNodeIndex[fieldIndex.y * countFields.x + fieldIndex.x];
}

unsigned TerrainTree::GetNodeIndexForField(const glm::ivec2& fieldIndex) const
{
	auto& countFields = m_Terrain.GetCountFields();
	if (!isValidField(fieldIndex, countFields)) return Core::c_InvalidIndexU;
	return m_TerrainFieldToNodeIndex[fieldIndex.y * countFields.x + fieldIndex.x];
}

unsigned TerrainTree::GetNodeIndexForField(const glm::ivec2& fieldIndex, unsigned nodeSize) const
{
	unsigned nodeIndex = GetNodeIndexForField(fieldIndex);
	if (nodeIndex == Core::c_InvalidIndexU) return nodeIndex;

	while (true)
	{
		auto& node = m_Nodes[nodeIndex];
		auto size = ::GetNodeSize(node);
		if (size.x == nodeSize && size.y == nodeSize)
		{
			assert(fieldIndex.x >= node.Start.x && fieldIndex.x <= node.End.x
				&& fieldIndex.y >= node.Start.y && fieldIndex.y <= node.End.y);
			return nodeIndex;
		}
		nodeIndex = node.Parent;
		assert(nodeIndex != Core::c_InvalidIndexU);
	}
}

void TerrainTree::GetNodeIndices(const glm::ivec2& startIndex, const glm::ivec2& endIndex, unsigned nodeSize,
	Core::IndexVectorU& nodeIndices) const
{
	nodeIndices.Clear();

	assert(m_NodeIndexQueue.empty());
	m_NodeIndexQueue.push_back(0);

	while (!m_NodeIndexQueue.empty())
	{
		auto nodeIndex = m_NodeIndexQueue.front();
		m_NodeIndexQueue.pop_front();

		auto& node = m_Nodes[nodeIndex];
		auto& nStart = node.Start;
		auto& nEnd = node.End;

		if (endIndex.x < nStart.x || endIndex.y < nStart.y || startIndex.x > nEnd.x || startIndex.y > nEnd.y) continue;

		auto size = ::GetNodeSize(node);
		if (size.x == nodeSize && size.y == nodeSize)
		{
			nodeIndices.PushBack(nodeIndex);
		}
		else
		{
			assert(size.x > (int)nodeSize && size.y > (int)nodeSize);

			PushChildNodes(node, m_NodeIndexQueue);
		}
	}
}

uint64_t TerrainTree::GetNodeLocationHash(const glm::ivec2& startIndex, const glm::ivec2& size)
{
	auto pack = [](int value, unsigned _2_byteIndex) {
		return (uint64_t)((unsigned)value & 0xffff) << (uint64_t)(_2_byteIndex << 4);
	};
	return pack(startIndex.x, 0) | pack(startIndex.y, 1) | pack(size.x, 2) | pack(size.y, 3);
}

unsigned TerrainTree::GetNodeIndex(const LocationHashToIndexMap& locationHashToIndexMap,
	const glm::ivec2& startIndex, const glm::ivec2& size) const
{
	auto countFields = m_Terrain.GetCountFields();
	auto lastIndex = startIndex + size - 1;
	if (!isValidField(startIndex, countFields) || !isValidField(lastIndex, countFields)) return Core::c_InvalidIndexU;

	uint64_t hash = GetNodeLocationHash(startIndex, size);
	auto it = locationHashToIndexMap.find(hash);
	return it == locationHashToIndexMap.end() ? Core::c_InvalidIndexU : it->second;
}

void TerrainTree::Node::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Parent);
	for (size_t i = 0; i < 4; i++) Core::SerializeSB(bytes, Children[i]);
	for (size_t i = 0; i < 8; i++) Core::SerializeSB(bytes, Neighbors[i]);
	Core::SerializeSB(bytes, Core::ToPlaceHolder(Start));
	Core::SerializeSB(bytes, Core::ToPlaceHolder(End));
	Core::SerializeSB(bytes, MinHeight);
	Core::SerializeSB(bytes, MaxHeight);
	Core::SerializeSB(bytes, Flags);
}

void TerrainTree::Node::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Parent);
	for (size_t i = 0; i < 4; i++) Core::DeserializeSB(bytes, Children[i]);
	for (size_t i = 0; i < 8; i++) Core::DeserializeSB(bytes, Neighbors[i]);
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(Start));
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(End));
	Core::DeserializeSB(bytes, MinHeight);
	Core::DeserializeSB(bytes, MaxHeight);
	Core::DeserializeSB(bytes, Flags);
}

void TerrainTree::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, m_Nodes);
	Core::SerializeSB(bytes, m_IslandIndices);
	Core::SerializeSB(bytes, m_CountLeafs);
	Core::SerializeSB(bytes, m_TerrainFieldToNodeIndex);
}

void TerrainTree::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, m_Nodes);
	Core::DeserializeSB(bytes, m_IslandIndices);
	Core::DeserializeSB(bytes, m_CountLeafs);
	Core::DeserializeSB(bytes, m_TerrainFieldToNodeIndex);
}