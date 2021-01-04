// Timeborne/InputHandling.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <EngineBuildingBlocks/Math/GLM.h>

glm::vec2 GetCursorContentCSPosition(
	const glm::uvec2& windowsSize, const glm::uvec2& contentSize,
	const glm::vec2& cursorPositionInScreen);

void GetContentCursorRayInWorldCs(
	const glm::uvec2& windowsSize, const glm::uvec2& contentSize,
	EngineBuildingBlocks::Graphics::Camera& camera,
	bool clampingInCS,
	glm::vec3& origin, glm::vec3& direction, bool& valid,
	const glm::vec2& cursorPositionInScreen);