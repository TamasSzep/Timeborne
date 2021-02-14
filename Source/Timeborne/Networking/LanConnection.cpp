// Timeborne/Networking/LanConnection.cpp

#include <Timeborne/Networking/LanConnection.h>

LanConnection::LanConnection()
{
}

LanConnection::~LanConnection()
{
	Reset();
}

void LanConnection::Reset()
{
	m_Server.Reset();
	m_Client.Reset();
}

LanServer& LanConnection::GetServer()
{
	return m_Server;
}

LanClient& LanConnection::GetClient()
{
	return m_Client;
}
