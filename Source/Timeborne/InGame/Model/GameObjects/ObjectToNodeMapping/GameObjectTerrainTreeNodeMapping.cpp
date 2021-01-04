// Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/GameObjectTerrainTreeNodeMapping.cpp

#include <Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/GameObjectTerrainTreeNodeMapping.h>

#include <Timeborne/InGame/Model/GameObjects/GameObjectConstants.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectPose.h>
#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>

#include <EngineBuildingBlocks/Math/AABoundingBox.h>

GameObjectTerrainTreeNodeMapping::GameObjectTerrainTreeNodeMapping(const TerrainTree& terrainTree,
	unsigned nodeSize)
	: ObjectToNodeMapping(terrainTree)
	, m_NodeSize(nodeSize)
{
	assert(m_NodeSize > 0);
}

void GameObjectTerrainTreeNodeMapping::_UpdateCurrentNodeIndices(GameObjectId objectId,
	const EngineBuildingBlocks::Math::AABoundingBox& box)
{
	auto& boxMin = box.Minimum;
	auto& boxMax = box.Maximum;

	assert(boxMin.x < boxMax.x && boxMin.y < boxMax.y && boxMin.z < boxMax.z);
	assert(boxMax.y <= c_MaxGameObjectBoundingBoxYFromSurface);

	// Slightly eroding the bounding box's to avoid false positives in the integer node indices.
	glm::dvec2 boxMinD = glm::dvec2(boxMin.x, boxMin.z) + 1E-5;
	glm::dvec2 boxMaxD = glm::dvec2(boxMax.x, boxMax.z) - 1E-5;
	boxMinD = glm::min(boxMinD, boxMaxD);

	glm::ivec2 startIndex = GameObjectPose::GetTerrainFieldIndex(boxMinD);
	glm::ivec2 endIndex = GameObjectPose::GetTerrainFieldIndex(boxMaxD);
	glm::ivec2 size = endIndex - startIndex + 1;
	m_TerrainTree->GetNodeIndices(startIndex, endIndex, m_NodeSize, m_CurrentNodeIndices);
}
