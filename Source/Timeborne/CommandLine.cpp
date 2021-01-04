// Timeborne/CommandLine.cpp

#include <Core/String.hpp>
#include <Timeborne/Logger.h>
#include <Timeborne/CommandLine.h>

#include <cxxopts.hpp>

void CommandLine::executeCommand(const char* command)
{
	// Splitting to standard command line format.
	std::vector<std::string> argvVector;
	Core::Split(command, Core::IsWhitespace(), true, argvVector);
	std::vector<char*> argvCVector(argvVector.size() + 1);
	argvCVector[0] = nullptr;
	std::transform(argvVector.begin(), argvVector.end(), argvCVector.begin() + 1, [](const std::string& str) {return (char*)str.c_str();});
	auto argc = (int)argvCVector.size();
	auto argv = argvCVector.data();

	try
	{
		std::string flagsString;

		// @todo: this code just demonstrates the usage of the options parsing library.
		int a = 0, b = 0;
		cxxopts::Options options("Timeborne");
		options.add_options()("a,aa", "1", cxxopts::value<int>(a));
		options.add_options()("b,bb", "2", cxxopts::value<int>(b));
		auto res = options.parse(argc, argv);
		if (res.count("a"))
		{
			flagsString.append("a=").append(std::to_string(a)).append(",");
		}
		if(res.count("b"))
		{
			flagsString.append("b=").append(std::to_string(b));
		}
		Logger::Log([&](Logger::Stream& stream) {stream << "Command flags: " << flagsString;}, LogSeverity::Info);
	}
	catch (std::exception& ex)
	{
		Logger::Log([&](Logger::Stream& stream) {stream << "An exception has been occured in the command line parsing: " << ex.what();},
			LogSeverity::Info);
	}
}