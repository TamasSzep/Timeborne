// Timeborne/InGame/Controller/CommandList.cpp

#include <Timeborne/InGame/Controller/CommandList.h>

CommandList::CommandList()
{
}

CommandList::~CommandList()
{
}

unsigned CommandList::GetCountCommands() const
{
	return (unsigned)m_Commands.size();
}

void CommandList::AddCommand(const GameObjectCommand& command)
{
	auto commandId = m_NextCommandId++;
	m_GameObjectCommands.Add(commandId) = command;
	m_Commands.push_back({ CommandSource::GameObject, commandId });
}

const CommandList::CommandData& CommandList::GetCommandForIndex(unsigned index) const
{
	assert(index < (unsigned)m_Commands.size());
	return m_Commands[index];
}

const CommandList::CommandData& CommandList::GetCommandForCommandId(unsigned commandId) const
{
	assert(commandId >= m_StartCommandId);
	auto& command = GetCommandForIndex(commandId - m_StartCommandId);
	assert(command.CommandId == commandId);
	return command;
}

const CommandList::CommandData& CommandList::GetLastCommand() const
{
	assert(!m_Commands.empty());
	return m_Commands.back();
}

void CommandList::RemoveFirstCommand()
{
	assert(!m_Commands.empty());
	auto& command = m_Commands.front();
	if (command.Source == CommandSource::GameObject)
	{
		m_GameObjectCommands.Remove(command.CommandId);
	}
	m_Commands.pop_front();
	m_StartCommandId++;
}

const GameObjectCommand& CommandList::GetGameObjectCommand(unsigned commandId) const
{
	auto commandData = m_GameObjectCommands.Get(commandId);
	assert(commandData != nullptr);
	return *commandData;
}
