// Timeborne/InGame/Model/TickContext.h

#pragma once

#include <cstdint>

struct TickContext
{
	uint32_t UpdateIntervalInMillis;
	uint32_t TickCount;
};