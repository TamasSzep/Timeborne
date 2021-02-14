// Timeborne/Networking/LanClient.cpp

#include <Timeborne/Networking/LanClient.h>

#include <Timeborne/Networking/LanCommon.h>

#include <Core/System/ThreadPool.h>
#include <EngineBuildingBlocks/ErrorHandling.h>

#include <asio.hpp>

#include <system_error>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RaiseException(const char* callerFunc, const std::error_code& errorCode)
{
	char buffer[1024];
	sprintf_s(buffer, "Error in %s: %s", callerFunc, errorCode.message().c_str());
	EngineBuildingBlocks::RaiseException(buffer);
}

using TStartCallback = std::function<void(const std::error_code&)>;

void StartAsync(std::unique_ptr<asio::ip::tcp::socket>& socket, asio::io_service& asioService,
	uint16_t portNumber, const char* hostName, TStartCallback callback)
{
	std::stringstream ssPortNumber;
	ssPortNumber << portNumber;

	asio::ip::tcp::resolver resolver(asioService);
	asio::ip::tcp::resolver::query query(
		asio::ip::tcp::v4(), hostName, ssPortNumber.str());
	asio::error_code errorCode;
	asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query, errorCode);
	if (errorCode)
	{
		RaiseException("StartAsync", errorCode);
	}

	socket = std::make_unique<asio::ip::tcp::socket>(asioService);
	socket->async_connect(iterator->endpoint(), callback);
}

void Send(std::unique_ptr<asio::ip::tcp::socket>& socket, const void* buffer, size_t size)
{
	asio::error_code errorCode;
	asio::write(*socket, asio::buffer(buffer, size), errorCode);
	if (errorCode)
	{
		RaiseException("Send", errorCode);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class LanClientImpl
{
	std::unique_ptr<asio::io_service> m_ASIOService;
	std::unique_ptr<Core::ThreadPool> m_ThreadPool;

	std::unique_ptr<asio::ip::tcp::socket> m_ConnectSocket;

	void ExecuteOne(uint32_t threadIndex)
	{
		m_ThreadPool->GetThread(threadIndex).Execute([this]() { m_ASIOService->run_one(); });
	}

	void ConnectCallback(const std::error_code& errorCode)
	{
		if (errorCode)
		{
			RaiseException("LanClientImpl::ConnectCallback", errorCode);
		}
		else
		{
			// DEBUG!!!
			char buffer[64];
			snprintf(buffer, 64, "hammerman");
			Send(m_ConnectSocket, buffer, 64);
			printf("Connected!\n");
		}
	}

public:

	LanClientImpl()
	{
	}

	~LanClientImpl()
	{
	}

	void Reset()
	{
		if (m_ASIOService != nullptr)
		{
			m_ASIOService->stop();
			m_ASIOService.reset();
		}

		if (m_ThreadPool != nullptr)
		{
			m_ThreadPool->Join();
			m_ThreadPool.reset();
		}

		m_ConnectSocket.reset();
		m_ASIOService.reset();
	}

	void ListServers()
	{
		// @todo
	}

	void Start()
	{
		auto portNumber = c_DispatchPort;
		auto hostName = "127.0.0.1";

		// Creating the asynchronous IO service.
		m_ASIOService = std::make_unique<asio::io_service>();

		// Creating and starting the connect thread.
		m_ThreadPool = std::make_unique<Core::ThreadPool>(1);

		// Creating the connect socket.
		StartAsync(m_ConnectSocket, *m_ASIOService, portNumber, hostName,
			std::bind(&LanClientImpl::ConnectCallback, this, std::placeholders::_1));

		// Executing the callback in thread 0.
		ExecuteOne(0);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LanClient::LanClient()
	: m_Implementor(std::make_unique<LanClientImpl>())
{
}

LanClient::~LanClient()
{
}

void LanClient::Reset()
{
	m_Implementor->Reset();
}

void LanClient::ListServers()
{
	m_Implementor->ListServers();
}

void LanClient::Start()
{
	m_Implementor->Start();
}
