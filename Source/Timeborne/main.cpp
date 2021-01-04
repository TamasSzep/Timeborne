// Timeborne.cpp : Defines the entry point for the console application.

#include <Timeborne/MainApplication.h>

int main(int argc, char *argv[])
{
	try
	{
		MainApplication application(argc, argv);
		return application.Run();
	}
	catch (const std::exception& ex)
	{
		EngineBuildingBlocks::PrintException(ex);
	}
	return 2;
}