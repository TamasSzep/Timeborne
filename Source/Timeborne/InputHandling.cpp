// Timeborne/InputHandling.cpp

#include <Timeborne/InputHandling.h>

#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>

using namespace EngineBuildingBlocks::Graphics;

glm::vec2 GetCursorContentCSPosition(
	const glm::uvec2& windowsSize, const glm::uvec2& contentSize, const glm::vec2& cursorPositionInScreen)
{
	// The content is located in the top right corner of the screen.
	auto offsetX = windowsSize.x - contentSize.x;
	auto positionInContent = glm::clamp(glm::vec2(cursorPositionInScreen.x - offsetX, cursorPositionInScreen.y),
		glm::vec2(0), glm::vec2(contentSize) - 1.0f);

	auto cursorPosition = positionInContent / glm::vec2(contentSize) * 2.0f - 1.0f;
	cursorPosition.y *= -1.0f;

	return cursorPosition;
}

void GetContentCursorRayInWorldCs(
	const glm::uvec2& windowsSize, const glm::uvec2& contentSize, Camera& camera, bool clampingInCS,
	glm::vec3& origin, glm::vec3& direction, bool& valid, const glm::vec2& cursorPositionInScreen)
{
	auto cursorPosition = GetCursorContentCSPosition(windowsSize, contentSize, cursorPositionInScreen);

	if (clampingInCS)
	{
		cursorPosition = glm::clamp(cursorPosition, glm::vec2(-1.0f), glm::vec2(1.0f));
	}

	auto camInvViewProj = camera.GetViewProjectionMatrixInverse();
	auto cursorWorldPos4 = camInvViewProj * glm::vec4(cursorPosition, 0.0f, 1.0f); // Cursor on the near plane.
	auto cursorWorldPos = glm::vec3(cursorWorldPos4) / cursorWorldPos4.w;

	origin = cursorWorldPos;

	auto projectionType = camera.GetProjectionType();
	if (projectionType == ProjectionType::Perspective)
	{
		direction = glm::normalize(cursorWorldPos - camera.GetPosition());
	}
	else if (projectionType == ProjectionType::Orthographic)
	{
		direction = camera.GetDirection();
	}
	else
	{
		assert(false);
	}

	valid = cursorPosition.x >= -1.0f && cursorPosition.x <= 1.0f
		&& cursorPosition.y >= -1.0f && cursorPosition.y <= 1.0f;
}