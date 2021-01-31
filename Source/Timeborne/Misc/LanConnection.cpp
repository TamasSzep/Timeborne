// Timeborne/Misc/LanConnection.cpp

#include <Timeborne/Misc/LanConnection.h>

#include <Timeborne/Logger.h>

#include <Core/System/Socket.h>
#include <Core/System/ThreadPool.h>

#include <cstdint>

constexpr uint32_t c_CountMaxConnections = 8U;
constexpr uint16_t c_DispatchPort = 0x4154;

uint16_t GetPortForClientIndex(uint32_t clientIndex, bool sendSocket)
{
	return (c_DispatchPort + 1) + (clientIndex * 2) + (sendSocket ? 0 : 1);
}

// Checks whether the target platform has little-endian byte order. Note that the VALUE of endianness does NOT matter,
// but to keep the serialization simple, no byte swaps were implemented, so the endianness must MATCH between
// server and client.
void LanConnection::CheckEndianness()
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

uint32_t GetNext2Power(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Threads: dispatch (1), send (1), receive (#Clients)

constexpr uint32_t c_DispatchThreadIndex = 0;
constexpr uint32_t c_SendThreadIndex = 1;
constexpr uint32_t c_ReceiveThreadOffset = 2;
constexpr uint32_t c_ServerReceiveBufferSize = 1024 * 1024;

LanServer::LanServer()
	: m_ThreadPool(std::make_unique<Core::ThreadPool>(c_CountMaxConnections + c_ReceiveThreadOffset))
{
}

LanServer::~LanServer()
{
}

void LanServer::Reset()
{
	m_Running = false;
	m_ThreadPool->Join();

	// No other threads can run, so we can access all data members without mutex blocks.

	m_Clients.Clear();
	m_ClientIdToIndexMap.clear();
}

void LanServer::Start()
{
	m_Running = true;
	m_ThreadPool->GetThread(c_DispatchThreadIndex).Execute(&LanServer::_RunDispatcher, this);
}

void LanServer::_RunDispatcher()
{
	Core::ServerSocket dispatchSocket;
	dispatchSocket.SetTimeout(100);

	while (m_Running)
	{
		bool cannotAddNewClient;
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			assert(m_Clients.GetSize() <= c_CountMaxConnections);
			cannotAddNewClient = (m_Clients.GetSize() == c_CountMaxConnections);
		}

		// 'cannotAddNewClient' might be invalid at this point, but it's okay, because we only decide here
		// whether a sleep is necessary.
		if (cannotAddNewClient)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		dispatchSocket.ClearError();
		dispatchSocket.Start(c_DispatchPort);
		if (!dispatchSocket.HasError())
		{
			uint32_t clientIndex;
			{
				std::lock_guard<std::mutex> lock(m_Mutex);

				assert(m_Clients.GetSize() < c_CountMaxConnections);

				clientIndex = m_Clients.Add();
				assert(clientIndex < c_CountMaxConnections);

				dispatchSocket.Send(&clientIndex, sizeof(uint32_t));
				if (dispatchSocket.HasError())
				{
					m_Clients.Remove(clientIndex);
					continue;
				}

				auto& clientData = m_Clients[clientIndex];

				bool result = ConnectSocket(clientIndex, GetPortForClientIndex(clientIndex, true), clientData.SendSocket);
				if (!result)
				{
					m_Clients.Remove(clientIndex);
					continue;
				}

				result = ConnectSocket(clientIndex, GetPortForClientIndex(clientIndex, false), clientData.ReceiveSocket);
				if (!result)
				{
					m_Clients.Remove(clientIndex);
					continue;
				}

				uint32_t clientId = m_NextClientId++;
				clientData.ClientId = clientId;
				m_ClientIdToIndexMap[clientId] = clientIndex;
			}

			m_ThreadPool->GetThread(clientIndex + c_ReceiveThreadOffset).Execute(
				&LanServer::_Receive, this, clientIndex);
		}
	}
}

bool LanServer::ConnectSocket(uint32_t clientIndex, uint16_t port, std::unique_ptr<Core::ServerSocket>& socket)
{
	socket = std::make_unique<Core::ServerSocket>();
	socket->SetTimeout(100);

	while (m_Running)
	{
		socket->ClearError();
		socket->Start(port);
		if (socket->HasError())
		{
			return true;
		}
	}

	return false;
}

void LanServer::Send(uint32_t clientId, const void* buffer, size_t size)
{
	m_Mutex.lock();

	auto cIt = m_ClientIdToIndexMap.find(clientId);
	if (cIt != m_ClientIdToIndexMap.end())
	{
		uint32_t clientIndex = cIt->second;

		// Allocating send buffer.
		Core::ByteVectorU* sendBuffer;
		{
			auto& clientData = m_Clients[clientIndex];
			auto& sendBuffers = clientData.SendBuffers;
			auto& unusedSendBufferIndices = clientData.UnusedSendBufferIndices;

			uint32_t sendBufferIndex;
			if (unusedSendBufferIndices.IsEmpty())
			{
				sendBufferIndex = (uint32_t)sendBuffers.size();
				sendBuffers.push_back(std::make_unique<Core::ByteVectorU>());
			}
			else
			{
				sendBufferIndex = unusedSendBufferIndices.PopBackReturn();
			}

			sendBuffer = sendBuffers[sendBufferIndex].get();
			if (sendBuffer->GetSize() < (uint32_t)size)
			{
				sendBuffer->Resize(GetNext2Power((uint32_t)size));
			}

			clientData.SendTaskIndices.PushBack(sendBufferIndex);
		}

		m_Mutex.unlock();

		// Indepently of what other threads do, 'sendBuffer' remains valid due to the unique_ptr.

		// Copying the data to the send buffer.
		std::memcpy(sendBuffer->GetArray(), buffer, size);

		// Note that the data will NOT be accessed, until this function is called, so the copying can stay
		// outside of the mutex block.
		m_ThreadPool->GetThread(1).Execute(&LanServer::_Send, this, clientIndex);
	}
	else
	{
		m_Mutex.unlock();
	}
}

void LanServer::_Send(uint32_t clientIndex)
{
	Core::ServerSocket* socket;
	uint32_t taskIndex;
	Core::ByteVectorU* sendBuffer;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		auto& clientData = m_Clients[clientIndex];
		socket = clientData.SendSocket.get();
		taskIndex = clientData.SendTaskIndices[0];
		clientData.SendTaskIndices.Remove(0);
		sendBuffer = clientData.SendBuffers[taskIndex].get();
	}

	// After creating the send socket, it is only accessed here.
	// Indepently of what other threads do, 'sendBuffer' remains valid due to the unique_ptr.

	socket->Send(sendBuffer->GetArray(), sendBuffer->GetSize());
}

void LanServer::_Receive(uint32_t clientIndex)
{
	Core::ByteVectorU receiveBuffer;
	receiveBuffer.Resize(c_ServerReceiveBufferSize);

	Core::ServerSocket* socket;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		auto& clientData = m_Clients[clientIndex];
		socket = clientData.ReceiveSocket.get();
	}

	// After creating the send socket, it is only accessed here.

	while (m_Running)
	{
		socket->Receive(receiveBuffer.GetArray(), receiveBuffer.GetSize());

		// @todo: process received data. Probably buffer pointer ping-pong has to be implemented here.
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LanClient::LanClient()
{
}

LanClient::~LanClient()
{
}

void LanClient::ListServers()
{
	// @todo
}

void LanClient::Start()
{
	// @todo
}

void LanClient::Reset()
{
	// @todo

	m_ConnectSocket.reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
