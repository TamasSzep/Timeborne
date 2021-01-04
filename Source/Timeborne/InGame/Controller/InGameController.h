// Timeborne/InGame/Controller/InGameController.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <memory>

class ClientGameState;
class CommandList;
struct ComponentPreUpdateContext;
class GameObjectCommands;
class GameObjectVisibilityProvider;
class Level;
class MainApplication;

class InGameController
{
	std::unique_ptr<GameObjectCommands> m_GameObjectCommands;

public:
	InGameController(const Level& level, ClientGameState& clientGameState,	
		CommandList& commandList, GameObjectVisibilityProvider& visibilityProvider, 
		EngineBuildingBlocks::Graphics::Camera& camera,
		MainApplication& application, bool fromSaveFile);
	~InGameController();

	bool HandleEvent(const EngineBuildingBlocks::Event* _event);

	void PreUpdate(const ComponentPreUpdateContext& context);
};