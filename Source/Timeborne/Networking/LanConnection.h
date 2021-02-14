// Timeborne/Networking/LanConnection.h

#pragma once

#include <Timeborne/Networking/LanServer.h>
#include <Timeborne/Networking/LanClient.h>

class LanConnection
{
	LanServer m_Server;
	LanClient m_Client;

public:

	LanConnection();
	~LanConnection();

	void Reset();

	LanServer& GetServer();
	LanClient& GetClient();
};
