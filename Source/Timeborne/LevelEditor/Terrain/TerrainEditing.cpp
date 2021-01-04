// Timeborne/LevelEditor/Terrain/TerrainEditing.cpp

#include <Timeborne/LevelEditor/Terrain/TerrainEditing.h>

#include <Timeborne/InputHandling.h>

#include <EngineBuildingBlocks/Input/MouseHandler.h>
#include <EngineBuildingBlocks/Math/Intersection.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Input;
using namespace EngineBuildingBlocks::Math;

void DragDifference::Update(const Event* _event)
{
	auto& eDD = ToMouseDragEvent(_event).DragDifference;
	std::lock_guard<std::mutex> lock(Mutex);
	DragDiff += glm::vec2(eDD.x, -eDD.y);
}

void DragDifference::Clear()
{
	std::lock_guard<std::mutex> lock(Mutex);
	DragDiff = glm::vec2(0.0f);
}

float DragDifference::RemoveIntegerY(float divisor)
{
	float dragDiffY;
	std::lock_guard<std::mutex> lock(Mutex);
	if (DragDiff.y > 0.0f) dragDiffY = std::floor(DragDiff.y / divisor);
	else dragDiffY = std::ceil(DragDiff.y / divisor);
	DragDiff.y -= dragDiffY * divisor;
	return dragDiffY;
}
