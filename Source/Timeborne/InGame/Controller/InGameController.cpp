// Timeborne/InGame/Controller/InGameController.cpp

#include <Timeborne/InGame/Controller/InGameController.h>

#include <Timeborne/InGame/Controller/GameObjects/GameObjectCommands.h>
#include <Timeborne/InGame/GameState/ClientGameState.h>

InGameController::InGameController(const Level& level, ClientGameState& clientGameState,
	CommandList& commandList, GameObjectVisibilityProvider& visibilityProvider,
	EngineBuildingBlocks::Graphics::Camera& camera, MainApplication& application, bool fromSaveFile)
	: m_GameObjectCommands(std::make_unique<GameObjectCommands>(level, clientGameState.GetGameCreationData(),
		clientGameState.GetSyncedGameState(), clientGameState.GetLocalGameState(),
		commandList, visibilityProvider, camera, application, fromSaveFile))
{
}

InGameController::~InGameController()
{
}

bool InGameController::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	return m_GameObjectCommands->HandleEvent(_event);
}

void InGameController::PreUpdate(const ComponentPreUpdateContext& context)
{
	m_GameObjectCommands->PreUpdate(context);
}
