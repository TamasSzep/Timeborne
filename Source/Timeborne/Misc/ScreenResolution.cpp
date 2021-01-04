// Timeborne/Misc/ScreenResolution.cpp

#include <Timeborne/Misc/ScreenResolution.h>

float RatioToPixels(float ratio, unsigned screenSize)
{
	return std::round(ratio * (float)screenSize);
}

float PixelsToRatio(float pixels, unsigned screenSize)
{
	return pixels / (float)screenSize;
}

glm::vec2 RatioToPixels(const glm::vec2& ratio, const glm::uvec2& screenSize)
{
	return glm::round(ratio * glm::vec2(screenSize));
}

glm::vec2 PixelsToRatio(const glm::vec2& pixels, const glm::uvec2& screenSize)
{
	return pixels / glm::vec2(screenSize);
}