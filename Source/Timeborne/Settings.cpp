// Timeborne/Settings.cpp

#include <Timeborne/Settings.h>

#include <Core/DataStructures/Properties.h>

InGameSettings::InGameSettings()
{
	TerrainTessellationBase = 16.0f;
}

#define TryGetInGameConfiguration(name) InGameSettings::TryGetConfiguration(configuration, #name, name)

void InGameSettings::Load(const Core::Properties& configuration)
{
	TryGetInGameConfiguration(TerrainTessellationBase);
}

Settings::Settings()
{
}

void Settings::Load(const Core::Properties& configuration)
{
	InGame.Load(configuration);
}