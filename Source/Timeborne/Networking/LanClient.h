// Timeborne/Networking/LanClient.h

#pragma once

#include <memory>

class LanClientImpl;

class LanClient
{
	std::unique_ptr<LanClientImpl> m_Implementor;

public:

	LanClient();
	~LanClient();

	void Reset();
	void ListServers();
	void Start();
};
