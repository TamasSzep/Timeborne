// Timeborne/LevelEditor/Terrain/TerrainEditing.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <EngineBuildingBlocks/Math/GLM.h>

#include <mutex>

struct DragDifference
{
	std::mutex Mutex;
	glm::vec2 DragDiff;

	void Update(const EngineBuildingBlocks::Event* _event);
	void Clear();
	float RemoveIntegerY(float divisor);
};
