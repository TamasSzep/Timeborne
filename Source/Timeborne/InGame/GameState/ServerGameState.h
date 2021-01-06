// Timeborne/InGame/GameState/ServerGameState.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectFightData.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectRoute.h>

#include <vector>

class ServerGameState
{
	uint32_t m_TickCount = 0;

	GameObjectList m_GameObjects;
	GameObjectRouteList m_Routes;
	GameObjectFightList m_FightList;

public:

	uint32_t GetTickCount() const;
	void IncreaseTickCount();

	const GameObjectList& GetGameObjects() const;
	GameObjectList& GetGameObjects();
	
	const GameObjectRouteList& GetRoutes() const;
	GameObjectRouteList& GetRoutes();

	const GameObjectFightList& GetFightList() const;
	GameObjectFightList& GetFightList();

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);

	void NotifyListenersWithFullState();
};
