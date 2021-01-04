// Timeborne/Misc/UserConfiguration.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <Core/DataStructures/Properties.h>

Core::Properties LoadUserConfiguration(
	const char* defaultConfigPath,
	EngineBuildingBlocks::ResourceDatabase* resourceDatabase);
