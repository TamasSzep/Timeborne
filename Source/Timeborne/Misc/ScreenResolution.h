// Timeborne/Misc/ScreenResolution.h

#pragma once

#include <EngineBuildingBlocks/Math/GLM.h>

float RatioToPixels(float ratio, unsigned screenSize);
float PixelsToRatio(float pixels, unsigned screenSize);

glm::vec2 RatioToPixels(const glm::vec2& ratio, const glm::uvec2& screenSize);
glm::vec2 PixelsToRatio(const glm::vec2& pixels, const glm::uvec2& screenSize);
