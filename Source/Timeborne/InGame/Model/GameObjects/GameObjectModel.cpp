// Timeborne/InGame/Model/GameObjects/GameObjectModel.cpp

#include <Timeborne/InGame/Model/GameObjects/GameObjectModel.h>

#include <Timeborne/InGame/Controller/CommandList.h>
#include <Timeborne/InGame/GameState/ClientGameState.h>
#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectFightSubsystem.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectMovementSubsystem.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectWorkSubsystem.h>
#include <Timeborne/InGame/Model/CommandListProcessor.h>
#include <Timeborne/InGame/Model/Level.h>

using namespace EngineBuildingBlocks::Graphics;

GameObjectModel::GameObjectModel(const Level& level, const GameCreationData& gameCreationData,
	GameObjectData& gameObjectData, const CommandList& commandList,
	const CommandListProcessor& commandListProcessor, bool fromSaveFile)
	: m_GameObjectData(gameObjectData)
	, m_CommandList(commandList)
	, m_CommandListProcessor(commandListProcessor)
{
	// Creating the subsystems here AFTER setting game state in the game object data.
	m_MovementSubsystem = std::make_unique<GameObjectMovementSubsystem>(level, gameObjectData);
	m_FightSubsystem = std::make_unique<GameObjectFightSubsystem>(level, gameCreationData, gameObjectData,
		*m_MovementSubsystem);
	m_WorkSubsystem = std::make_unique<GameObjectWorkSubsystem>(level);

	if (fromSaveFile)
	{
		LoadState();
	}
	else
	{
		AddGameObjectsFromLevel(level);
	}
}

GameObjectModel::~GameObjectModel()
{
}

void GameObjectModel::ProcessCommands()
{
	assert(m_GameObjectData.ClientModelGameState != nullptr);
	const auto& gameObjectsMap = m_GameObjectData.ClientModelGameState->GetGameObjects().Get();

	m_CommandListProcessor.GetAvailableCommands(CommandSource::GameObject, m_AvailableCommandIds);
	auto countAvailableCommands = m_AvailableCommandIds.GetSize();

	auto& prototypes = GameObjectPrototype::GetPrototypes();

	for (unsigned i = 0; i < countAvailableCommands; i++)
	{
		unsigned commandId = m_AvailableCommandIds[i];
		assert(m_CommandList.GetCommandForCommandId(commandId).Source == CommandSource::GameObject);
		auto& commandData = m_CommandList.GetGameObjectCommand(commandId);

		if (commandData.Type == GameObjectCommand::Type::ObjectToTerrain)
		{
			m_MovementSubsystem->ProcessCommand(commandData);
		}
		else // ObjectToObject
		{
			auto oIt = gameObjectsMap.find(commandData.TargetId);
			assert(oIt != gameObjectsMap.end());
			auto targetTypeIndex = oIt->second.Data.TypeIndex;
			auto targetType = prototypes[(uint32_t)targetTypeIndex]->GetType();
			if (targetType == GameObjectPrototype::Type::Resource)
			{
				// Note: if repairing own buildings is needed, this has to be handled here.
				m_WorkSubsystem->ProcessCommand(commandData);
			}
			else
			{
				m_FightSubsystem->ProcessCommand(commandData);
			}
		}
	}
}

void GameObjectModel::Tick(const TickContext& context)
{
	ProcessCommands();

	m_MovementSubsystem->Tick(context);
	m_FightSubsystem->Tick(context);
	m_WorkSubsystem->Tick(context);
}

void GameObjectModel::AddGameObject(const GameObjectLevelData& goData)
{
	assert(m_GameObjectData.ClientModelGameState != nullptr);
	auto& gameObjects = m_GameObjectData.ClientModelGameState->GetGameObjects();
	auto& fightList = m_GameObjectData.ClientModelGameState->GetFightList();

	auto typeIndex = goData.TypeIndex;
	auto& prototype = *GameObjectPrototype::GetPrototypes()[(uint32_t)typeIndex];

	auto id = GameObjectId::MakeId(m_GameObjectData.NextGameObjectId++);

	// @todo: consider whether the object must have a fight component.
	GameObjectFightData fightData;
	m_FightSubsystem->InitializeForNewObject(fightData, prototype);
	uint32_t fightIndex = fightList.Add(fightData);

	GameObject obj;
	obj.Id = id;
	obj.Data.TypeIndex = typeIndex;
	obj.Data.PlayerIndex = goData.PlayerIndex;
	obj.Data.Pose = goData.Pose;
	obj.FightIndex = fightIndex;

	gameObjects.Add(obj);
}

void GameObjectModel::AddGameObjectsFromLevel(const Level& level)
{
	auto& gameObjects = level.GetGameObjects();
	auto gEnd = gameObjects.GetEndConstIterator();
	for (auto gIt = gameObjects.GetBeginConstIterator(); gIt != gEnd; ++gIt)
	{
		AddGameObject(*gIt);
	}
}

void GameObjectModel::LoadState()
{
	assert(m_GameObjectData.ClientModelGameState != nullptr);

	// Adding the game objects.
	uint32_t maxId = 0;
	auto& gameObjects = m_GameObjectData.ClientModelGameState->GetGameObjects().Get();
	for (auto& gameObjectData : gameObjects)
	{
		maxId = std::max(maxId, (uint32_t)gameObjectData.first);
	}
	m_GameObjectData.NextGameObjectId = gameObjects.empty() ? 0 : maxId + 1U;

	// No need to add routes explicitly.

	// Notifying the listeners.
	m_GameObjectData.ClientModelGameState->NotifyListenersWithFullState();
}
