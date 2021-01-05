// Timeborne/InGame/Model/GameObjects/GameObjectModel.h

#pragma once

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <memory>
#include <vector>

class ClientGameState;
class CommandList;
class CommandListProcessor;
struct GameCreationData;
struct GameObjectData;
struct GameObjectLevelData;
class GameObjectMovementSubsystem;
class GameObjectFightSubsystem;
class GameObjectSubsystem;
class GameObjectWorkSubsystem;
class Level;
class MainApplication;
struct TickContext;

class GameObjectModel
{
	GameObjectData& m_GameObjectData;

	const CommandList& m_CommandList;
	const CommandListProcessor& m_CommandListProcessor;

	std::unique_ptr<GameObjectMovementSubsystem> m_MovementSubsystem;
	std::unique_ptr<GameObjectFightSubsystem> m_FightSubsystem;
	std::unique_ptr<GameObjectWorkSubsystem> m_WorkSubsystem;
	std::vector<GameObjectSubsystem*> m_Subsystems;

	Core::IndexVectorU m_AvailableCommandIds;

	void ProcessCommands();

	void AddGameObject(const GameObjectLevelData& goData);
	void AddGameObjectsFromLevel(const Level& level);
	void LoadState();

public:
	GameObjectModel(const Level& level,
		const GameCreationData& gameCreationData, 
		GameObjectData& gameObjectData,
		const CommandList& commandList, 
		const CommandListProcessor& commandListProcessor,
		bool fromSaveFile);
	~GameObjectModel();

	void Tick(const TickContext& context);
};