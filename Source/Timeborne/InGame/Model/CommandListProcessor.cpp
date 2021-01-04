// Timeborne/InGame/Model/CommandListProcessor.cpp

#include <Timeborne/InGame/Model/CommandListProcessor.h>

#include <Timeborne/InGame/Controller/GameObjects/GameObjectCommand.h>
#include <Timeborne/InGame/Controller/CommandList.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>
#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/InGame/Model/TickContext.h>

// @todo: player-reference is missing.

CommandListProcessor::CommandListProcessor(const Settings& settings, CommandList& commandList,
	const GameObjectData& gameObjectData)
	: m_CommandList(commandList)
	, m_GameObjectData(gameObjectData)
	, m_Settings(settings)
	, m_ActionPoints(settings.ActionPointCapacity)
{
}

CommandListProcessor::~CommandListProcessor()
{
}

void CommandListProcessor::Tick(const TickContext& context)
{
	// The LAST commands are removed first.
	for (unsigned i = 0; i < m_ActiveCommandEnd; i++)
	{
		m_CommandList.RemoveFirstCommand();
	}

	// Updating the available action points.
	auto n = m_Settings.ActionPointsPerSecond;
	auto d = 1000 / context.updateIntervalInMillis;
	auto currentActionPoints = (n / d) + ((m_TickIndex < n % d) ? 1 : 0);
	m_TickIndex = (m_TickIndex + 1) % d;
	m_ActionPoints = std::min(m_ActionPoints + currentActionPoints, m_Settings.ActionPointCapacity);

	// Determining the index, until which (exclusively) the commands are available for execution.
	unsigned i = 0;
	auto countCommands = (unsigned)m_CommandList.GetCountCommands();
	for (; i < countCommands; i++)
	{
		auto& command = m_CommandList.GetCommandForIndex(i);

		// Computing the command's cost.
		unsigned cost = 0;
		if (command.Source == CommandSource::GameObject)
		{
			cost = GetGameObjectCommandCost(m_CommandList.GetGameObjectCommand(command.CommandId));
		}

		if (cost <= m_ActionPoints) m_ActionPoints -= cost;
		else break;
	}
	m_ActiveCommandEnd = i;
}

unsigned CommandListProcessor::GetGameObjectCommandCost(const GameObjectCommand& command) const
{
	assert(m_GameObjectData.ClientModelGameState != nullptr);

	const auto& prototypes = GameObjectPrototype::GetPrototypes();
	const auto& gameObjectsMap = m_GameObjectData.ClientModelGameState->GetGameObjects().Get();

	unsigned cost = 0;
	unsigned countSources = command.SourceIds.GetSize();
	for (unsigned j = 0; j < countSources; j++)
	{
		auto objectId = command.SourceIds[j];
		auto oIt = gameObjectsMap.find(objectId);
		assert(oIt != gameObjectsMap.end());
		const auto& object = oIt->second;
		cost += prototypes[(uint32_t)object.Data.TypeIndex]->GetActionCost();
	}
	return cost;
}

void CommandListProcessor::GetAvailableCommands(CommandSource source, Core::IndexVectorU& commandIds) const
{
	commandIds.Clear();
	for (unsigned i = 0; i < m_ActiveCommandEnd; i++)
	{
		auto& command = m_CommandList.GetCommandForIndex(i);
		if (command.Source == source)
		{
			commandIds.PushBack(command.CommandId);
		}
	}
}
