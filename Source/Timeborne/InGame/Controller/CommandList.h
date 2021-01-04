// Timeborne/InGame/Controller/CommandList.h

#pragma once

#include <Timeborne/DataStructures/FastReusableResourceMap.h>
#include <Timeborne/InGame/Controller/GameObjects/GameObjectCommand.h>
#include <Timeborne/InGame/Model/CommandSource.h>

#include <deque>

class CommandList
{
	using GameObjectCommandMap = FastReusableResourceMap<unsigned, GameObjectCommand>;

private:

	struct CommandData
	{
		CommandSource Source;
		unsigned CommandId;
	};

	std::deque<CommandData> m_Commands;
	GameObjectCommandMap m_GameObjectCommands;

	unsigned m_StartCommandId = 0;
	unsigned m_NextCommandId = 0;

public:

	CommandList();
	~CommandList();

	unsigned GetCountCommands() const;

	void AddCommand(const GameObjectCommand& command);
	
	// Other command types can be added here.
	
	const CommandData& GetCommandForIndex(unsigned index) const;
	const CommandData& GetCommandForCommandId(unsigned commandId) const;
	void RemoveFirstCommand();

	const GameObjectCommand& GetGameObjectCommand(unsigned commandId) const;
	const CommandData& GetLastCommand() const;
};

