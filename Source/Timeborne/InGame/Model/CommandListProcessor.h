// Timeborne/InGame/Model/CommandListProcessor.h

#pragma once

#include <Timeborne/InGame/Model/CommandSource.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>

class CommandList;
struct GameObjectCommand;
struct GameObjectData;
struct TickContext;

class CommandListProcessor
{
public:

	struct Settings
	{
		unsigned ActionPointsPerSecond;
		unsigned ActionPointCapacity;
	};

private:

	CommandList& m_CommandList;
	const GameObjectData& m_GameObjectData;

	Settings m_Settings;

	unsigned m_ActionPoints = 0;
	unsigned m_ActiveCommandEnd = 0;
	unsigned m_TickIndex = 0;

	unsigned GetGameObjectCommandCost(const GameObjectCommand& command) const;

public:

	CommandListProcessor(const Settings& settings,
		CommandList& commandList, const GameObjectData& gameObjectData);
	~CommandListProcessor();

	void Tick(const TickContext& context);

	void GetAvailableCommands(CommandSource source, Core::IndexVectorU& commandIds) const;
};
