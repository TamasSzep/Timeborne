// Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/GroundObjectTerrainTreeNodeMapping.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/ObjectToNodeMapping.h>

#include <Timeborne/InGame/Model/GameObjects/GameObjectTypeIndex.h>

#include <EngineBuildingBlocks/Math/GLM.h>

class GameObjectPrototype;
class GameObjectPose;

class GroundObjectTerrainTreeNodeMapping : public ObjectToNodeMapping<GroundObjectTerrainTreeNodeMapping>
{
	void PushForCirle(double circleRadius, const GameObjectPose& pose);
	void PushForRectangle(const glm::vec3& sizeF, const GameObjectPose& pose);

public:
	explicit GroundObjectTerrainTreeNodeMapping(const TerrainTree& terrainTree);

	void _UpdateCurrentNodeIndices(GameObjectId objectId,
		GameObjectTypeIndex typeIndex, const GameObjectPose& pose);

	static bool MatchesSamplingCriteria(const GameObjectPrototype& prototype);
};