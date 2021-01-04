// Timeborne/Misc/DefaultedProperties.cpp

#include <Timeborne/Misc/UserConfiguration.h>

#include <EngineBuildingBlocks/ResourceDatabase.h>

#include <filesystem>

using namespace EngineBuildingBlocks;

Core::Properties LoadUserConfiguration(const char* defaultConfigPath, ResourceDatabase* resourceDatabase)
{
	ResourceDescription defaultConfigDesc;
	defaultConfigDesc.ResourceFilePath = defaultConfigPath;
	BuiltResourceDescription userConfigDesc;
	resourceDatabase->GetBuiltResourceDescription(defaultConfigDesc, userConfigDesc);
	const auto& userConfigPath = userConfigDesc.BuiltResourceFilePath;

	if (!userConfigDesc.IsUpToDate)
	{
		// Either the user configuration doesn't exist or the default configuration was modified.
		// We create a copy of the default configuration in both cases.
		namespace fs = std::filesystem;
		fs::copy_file(defaultConfigPath, userConfigPath, fs::copy_options::overwrite_existing);
		fs::last_write_time(userConfigPath, fs::file_time_type::clock::now());
	}

	Core::Properties properties;
	properties.LoadFromXml(userConfigPath.c_str());
	return properties;
}
