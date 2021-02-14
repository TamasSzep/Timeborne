// Timeborne/Networking/LanCommon.h

#pragma once

#include <cstdint>

constexpr uint16_t c_DispatchPort = 0x4154;

inline uint16_t GetPortForClientIndex(uint32_t clientIndex, bool sendSocket)
{
	return (c_DispatchPort + 1) + (clientIndex * 2) + (sendSocket ? 0 : 1);
}
