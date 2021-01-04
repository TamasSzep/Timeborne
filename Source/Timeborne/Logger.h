// Timeborne/Logger.h

#pragma once

#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

#include <Core/Enum.h>

enum class LogSeverity
{
	Critical, // Unstabilities, crashes are probable.
	Error,    // An important operation has failed or inconsistency in the data is detected.
			  // The behavior of the application is unpredictable.
	Warning,  // An operation failed, but the application can be further executed.
	Info,     // Information about operations and states.
	Debug     // Detailed information about operations and states.
};

enum class LogFlags
{
	None = 0x00,
	AddMessageBox = 0x01, // Adds a message box with a single OK button.
	OmitSeverity = 0x02   // The log severity is not added to the output.
};

UseEnumAsFlagSet(LogFlags);

template <typename TStream>
void CreateLogHeader(TStream& stream, LogSeverity severity, LogFlags flags)
{
	if (!HasFlag(flags, LogFlags::OmitSeverity))
	{
		const char* severityStrs[] = { "CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG" };
		stream << "[" << severityStrs[(int)severity] << "] ";
	}
}

class ILoggerListener
{
public:
	virtual ~ILoggerListener() {}
	virtual void OnLog(const char* message, LogSeverity severity, LogFlags flags) = 0;
};

class Logger
{
public:

	using Stream = std::stringstream;

private:

	struct ListenerData
	{
		ILoggerListener* Listener;
		LogSeverity Filter;
	};

	LogSeverity m_OutputFilter = LogSeverity::Debug;
	std::vector<ListenerData> m_Listeners;
	std::ofstream m_OS;
	mutable std::mutex m_Mutex;

	static thread_local Stream m_SS;

	void PrepareSS();
	void Flush(LogSeverity severity, LogFlags flags);
	bool IsLogNeeded(LogSeverity severity) const;

public:

	static Logger* GetInstance();

	Logger();

	void SetOutputFilter(LogSeverity severity);

	void SetListener(ILoggerListener* listener, LogSeverity filter);
	void RemoveListener(ILoggerListener* listener);

	static void Log(const char* message, LogSeverity severity, LogFlags flags = LogFlags::None);

	template <typename TLogFunctor>
	static void Log(TLogFunctor&& logFunctor, LogSeverity severity, LogFlags flags = LogFlags::None)
	{
		auto instance = GetInstance();
		if (instance->IsLogNeeded(severity))
		{
			instance->PrepareSS();
			logFunctor(m_SS);
			instance->Flush(severity, flags);
		}
	}
};