// Timeborne/Networking/LanServer.h

#pragma once

#include <Timeborne/Declarations/CoreDeclarations.h>

#include <Core/DataStructures/ResourceUnorderedVector.hpp>
#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

class LanServer
{
	std::unique_ptr<Core::ThreadPool> m_ThreadPool;
	std::atomic_bool m_Running = false;

	struct ClientData
	{
		uint32_t ClientId;
		std::unique_ptr<Core::ServerSocket> ReceiveSocket;
		std::unique_ptr<Core::ServerSocket> SendSocket;

		// unique_ptr makes sure that only pointers are copied.
		std::vector<std::unique_ptr<Core::ByteVectorU>> SendBuffers;

		Core::IndexVectorU UnusedSendBufferIndices;
		Core::IndexVectorU SendTaskIndices;
	};

	std::map<uint32_t, uint32_t> m_ClientIdToIndexMap;
	Core::ResourceUnorderedVectorU<ClientData> m_Clients;

	uint32_t m_NextClientId = 0;

	std::mutex m_Mutex;

private: // Dispatch socket.

	std::unique_ptr<Core::ServerSocket> m_DispatchSocket;
	std::mutex m_DispatchSocketMutex;
	std::atomic_bool m_Dispatching = false;

	void DispatcherStartCallback(const std::error_code& errorCode);
	bool ConnectSocket(uint32_t clientIndex, uint16_t port, std::unique_ptr<Core::ServerSocket>& socket);

public: // Only for internal calls.

	void _RunDispatcher();
	void _Receive(uint32_t clientIndex);
	void _Send(uint32_t clientIndex);

public:

	LanServer();
	~LanServer();

	void Reset();
	void Start();
	void Send(uint32_t clientId, const void* buffer, size_t size);
};