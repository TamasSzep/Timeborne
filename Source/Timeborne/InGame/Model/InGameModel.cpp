// Timeborne/InGame/Model/InGameModel.cpp

#include <Timeborne/InGame/Model/InGameModel.h>

#include <Timeborne/InGame/Model/GameObjects/GameObjectModel.h>
#include <Timeborne/InGame/GameState/ClientGameState.h>
#include <Timeborne/InGame/Model/CommandListProcessor.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CommandListProcessor::Settings GetCommandListSettings()
{
	CommandListProcessor::Settings settings{};
	settings.ActionPointCapacity = 100000;
	settings.ActionPointsPerSecond = 10000;
	return settings;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

InGameModel::InGameModel(const Level& level, ClientGameState& clientGameState,
	CommandList& commandList, bool fromSaveFile)
	: m_CommandList(commandList)
	, m_GameObjectData{ &clientGameState.GetClientModelGameState() }
	, m_CommandListProcessor(std::make_unique<CommandListProcessor>(GetCommandListSettings(), commandList,
		m_GameObjectData))
	, m_GameObjectModel(std::make_unique<GameObjectModel>(level, clientGameState.GetGameCreationData(),
		m_GameObjectData, commandList, *m_CommandListProcessor, fromSaveFile))
{
	if (!fromSaveFile)
	{
		// @todo: load commands.
	}
}

InGameModel::~InGameModel()
{
}

void InGameModel::Tick(const TickContext& context)
{
	m_CommandListProcessor->Tick(context);
	m_GameObjectModel->Tick(context);
}
