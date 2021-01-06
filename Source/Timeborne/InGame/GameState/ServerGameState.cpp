// Timeborne/InGame/GameState/ServerGameState.cpp

#include <Timeborne/InGame/GameState/ServerGameState.h>

#include <Core/SimpleBinarySerialization.hpp>

uint32_t ServerGameState::GetTickCount() const
{
	return m_TickCount;
}

void ServerGameState::IncreaseTickCount()
{
	++m_TickCount;
}

const GameObjectList& ServerGameState::GetGameObjects() const
{
	return m_GameObjects;
}

GameObjectList& ServerGameState::GetGameObjects()
{
	return m_GameObjects;
}

const GameObjectRouteList& ServerGameState::GetRoutes() const
{
	return m_Routes;
}

GameObjectRouteList& ServerGameState::GetRoutes()
{
	return m_Routes;
}

const GameObjectFightList& ServerGameState::GetFightList() const
{
	return m_FightList;
}

GameObjectFightList& ServerGameState::GetFightList()
{
	return m_FightList;
}

void ServerGameState::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, m_TickCount);
	Core::SerializeSB(bytes, m_GameObjects);
	Core::SerializeSB(bytes, m_Routes);
	Core::SerializeSB(bytes, m_FightList);
}

void ServerGameState::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, m_TickCount);
	Core::DeserializeSB(bytes, m_GameObjects);
	Core::DeserializeSB(bytes, m_Routes);
	Core::DeserializeSB(bytes, m_FightList);
}

void ServerGameState::NotifyListenersWithFullState()
{
	m_GameObjects.NotifyListenersWithFullState();
	m_Routes.NotifyListenersWithFullState();
}
