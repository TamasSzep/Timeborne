// Timeborne/Logger.cpp

#include <Timeborne/Logger.h>

#include <algorithm>

thread_local Logger::Stream Logger::m_SS;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Logger* Logger::GetInstance()
{
	static Logger instance;
	return &instance;
}

Logger::Logger()
	: m_OS("../log.txt")
{
}

void Logger::SetOutputFilter(LogSeverity severity)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_OutputFilter = severity;
}

void Logger::SetListener(ILoggerListener* listener, LogSeverity filter)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = std::find_if(m_Listeners.begin(), m_Listeners.end(), [listener](const ListenerData& data){return data.Listener == listener;});
	if (it == m_Listeners.end())
	{
		m_Listeners.push_back({ listener, filter });
	}
	else
	{
		it->Filter = filter;
	}
}

void Logger::RemoveListener(ILoggerListener* listener)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	auto it = std::find_if(m_Listeners.begin(), m_Listeners.end(), [listener](const ListenerData& data) {return data.Listener == listener;});
	m_Listeners.erase(it, m_Listeners.end());
}

bool Logger::IsLogNeeded(LogSeverity severity) const
{
	auto isLogNeeded = [severity](LogSeverity filter) { return filter >= severity; };
	std::lock_guard<std::mutex> lock(m_Mutex);
	bool needed = isLogNeeded(m_OutputFilter);
	for (auto& listenerData : m_Listeners)
	{
		needed |= isLogNeeded(listenerData.Filter);
	}
	return needed;
}

void Logger::Log(const char* message, LogSeverity severity, LogFlags flags)
{
	auto instance = GetInstance();
	if (instance->IsLogNeeded(severity))
	{
		instance->PrepareSS();
		m_SS << message;
		instance->Flush(severity, flags);
	}
}

void Logger::PrepareSS()
{
	m_SS.str("");
	m_SS.clear();
}

void Logger::Flush(LogSeverity severity, LogFlags flags)
{
	auto str = m_SS.str();

	// Locking the mutex for the stream output and notifications.
	auto isLogNeeded = [severity](LogSeverity filter) { return filter >= severity; };
	std::lock_guard<std::mutex> lock(m_Mutex);

	if (isLogNeeded(m_OutputFilter))
	{	
		CreateLogHeader(m_OS, severity, flags);
		m_OS << str << "\n";
	}
	for (auto& listenerData : m_Listeners)
	{
		if (isLogNeeded(listenerData.Filter))
		{
			listenerData.Listener->OnLog(str.c_str(), severity, flags);
		}
	}
}