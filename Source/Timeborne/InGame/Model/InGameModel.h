// Timeborne/InGame/Model/InGameModel.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>
#include <Timeborne/InGame/Model/GameObjects/GameObject.h>

#include <memory>

class ClientGameState;
class CommandList;
class CommandListProcessor;
class GameObjectModel;
class GameObjectVisibilityProvider;
class Level;
class MainApplication;
struct TickContext;

class InGameModel
{
	const CommandList& m_CommandList;

	GameObjectData m_GameObjectData;

	std::unique_ptr<CommandListProcessor> m_CommandListProcessor;
	std::unique_ptr<GameObjectModel> m_GameObjectModel;

public:
	InGameModel(const Level& level, ClientGameState& clientGameState,
		CommandList& commandList, bool fromSaveFile);
	~InGameModel();

	void Tick(const TickContext& context);
};
