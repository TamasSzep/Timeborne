// Timeborne/Settings.h

#pragma once

#include <Timeborne/Declarations/CoreDeclarations.h>

struct InGameSettings
{
	float TerrainTessellationBase;

	InGameSettings();
	void Load(const Core::Properties& configuration);

private:
	template <typename T>
	void TryGetConfiguration(const Core::Properties& configuration, const char* name, T& result)
	{
		configuration.TryGetRootConfigurationValue(std::string("InGame.") + name, result);
	}
};

struct Settings
{
	InGameSettings InGame;

	Settings();
	void Load(const Core::Properties& configuration);
};