// Timeborne/Render/PlayerColors.h

#pragma once

#include <EngineBuildingBlocks/Math/GLM.h>

#include <cstdint>

static glm::vec3 c_PlayerColors[] =
{
	glm::vec3(0.0f, 162.0f, 232.0f) / 255.0f,
	glm::vec3(237.0f, 28.0f, 36.0f) / 255.0f,
	glm::vec3(34.0f, 177.0f, 76.0f) / 255.0f,
	glm::vec3(255.0f, 255.0f, 0.0f) / 255.0f,
	glm::vec3(255.0f, 0.0f, 255.0f) / 255.0f,
	glm::vec3(0.0f, 255.0f, 255.0f) / 255.0f,
	glm::vec3(255.0f, 128.0f, 0.0f) / 255.0f,
	glm::vec3(0.0f, 64.0f, 0.0f) / 255.0f
};

constexpr uint32_t c_CountPlayerColors = sizeof(c_PlayerColors) / sizeof(c_PlayerColors[0]);
