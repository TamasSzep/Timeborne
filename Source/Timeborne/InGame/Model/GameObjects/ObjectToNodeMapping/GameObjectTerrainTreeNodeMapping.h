// Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/GameObjectTerrainTreeNodeMapping.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/ObjectToNodeMapping.h>

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

class GameObjectTerrainTreeNodeMapping : public ObjectToNodeMapping<GameObjectTerrainTreeNodeMapping>
{
	unsigned m_NodeSize;

public:

	GameObjectTerrainTreeNodeMapping(const TerrainTree& terrainTree, unsigned nodeSize);

	void _UpdateCurrentNodeIndices(GameObjectId objectId,
		const EngineBuildingBlocks::Math::AABoundingBox& box);
};