// Timeborne/Networking/NetworkingCommon.cpp

#include <Timeborne/Networking/NetworkingCommon.h>

#include <Timeborne/Logger.h>

#include <cstdint>

// Checks whether the target platform has little-endian byte order. Note that the VALUE of endianness does NOT matter,
// but to keep the serialization simple, no byte swaps were implemented, so the endianness must MATCH between
// server and client.
void Networking_CheckEndianness()
{
	// When compiling with C++20 supported, check simply: std::endian::native == std::endian::little
	const uint64_t x = 0x0706050403020100;
	const auto bytes = (const uint8_t*)&x;
	bool isLittleEndian = true;
	for (int i = 0; i < 8; i++)
	{
		if (bytes[i] != i) { isLittleEndian = false; break; }
	}
	if (!isLittleEndian)
	{
		Logger::Log("Only CPUs with little-endian byte order are supported.", LogSeverity::Error,
			LogFlags::AddMessageBox);
	}
}