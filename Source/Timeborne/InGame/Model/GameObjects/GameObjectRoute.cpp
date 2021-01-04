// Timeborne/InGame/Model/GameObjects/GameObjectRoute.cpp

#include <Timeborne/InGame/Model/GameObjects/GameObjectRoute.h>

#include <Core/SimpleBinarySerialization.hpp>

#include <cmath>

bool GameObjectRoute::IsOrienting() const
{
	return !std::isnan(OrientationTarget.x);
}

void GameObjectRoute::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Path);
	Core::SerializeSB(bytes, NextFieldIndex);
	Core::SerializeSB(bytes, Core::ToPlaceHolder(OrientationTarget));
}

void GameObjectRoute::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Path);
	Core::DeserializeSB(bytes, NextFieldIndex);
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(OrientationTarget));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GameObjectRouteListener::GameObjectRouteListener()
{
}

GameObjectRouteListener::~GameObjectRouteListener()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GameObjectRouteList::AddListenerOnce(GameObjectRouteListener& listener)
{
	if (std::find(m_Listeners.begin(), m_Listeners.end(), &listener) == m_Listeners.end())
	{
		m_Listeners.push_back(&listener);
	}
}

GameObjectRoute& GameObjectRouteList::BeginAdd(GameObjectId objectId)
{
	return m_Routes.Add(objectId);
}

void GameObjectRouteList::FinishAdd(GameObjectId objectId)
{
	for (auto& listener : m_Listeners)
	{
		auto* route = m_Routes.Get(objectId);
		assert(route != nullptr);
		listener->OnRouteAdded(objectId, *route);
	}
}

void GameObjectRouteList::AbortAdd(GameObjectId objectId)
{
	m_Routes.Remove(objectId);
}

void GameObjectRouteList::Remove(GameObjectId objectId, RouteRemoveReason reason)
{
	m_Routes.Remove(objectId);

	for (auto& listener : m_Listeners)
	{
		listener->OnRouteRemoved(objectId, reason);
	}
}

const GameObjectRoute* GameObjectRouteList::GetRoute(GameObjectId objectId) const
{
	return m_Routes.Get(objectId);
}

GameObjectRoute& GameObjectRouteList::AccessRoute(GameObjectId objectId)
{
	auto data = m_Routes.Get(objectId);
	assert(data != nullptr);
	return *data;
}

const FastReusableResourceMap<GameObjectId, GameObjectRoute>& GameObjectRouteList::GetRoutes() const
{
	return m_Routes;
}

void GameObjectRouteList::SerializeSB(Core::ByteVector& bytes) const
{
	// The serialized state only consists of the game objects, NOT the listeners.

	Core::SerializeSB(bytes, (unsigned)m_Routes.GetSize());
	auto& routes = m_Routes.GetElements();
	auto rEnd = routes.GetEndConstIterator();
	for (auto rIt = routes.GetBeginConstIterator(); rIt != rEnd; ++rIt)
	{
		Core::SerializeSB(bytes, rIt->Data);
	}
}

void GameObjectRouteList::DeserializeSB(const unsigned char*& bytes)
{
	// The serialized state only consists of the game objects, NOT the listeners.

	m_Routes.Clear();
	unsigned countRoutes;
	Core::DeserializeSB(bytes, countRoutes);
	for (unsigned i = 0; i < countRoutes; i++)
	{
		GameObjectRoute route;
		Core::DeserializeSB(bytes, route);
		m_Routes.Add(route.Path.ObjectId) = route;
	}
}

void GameObjectRouteList::NotifyListenersWithFullState()
{
	auto& routes = m_Routes.GetElements();
	auto rEnd = routes.GetEndConstIterator();
	for (auto rIt = routes.GetBeginConstIterator(); rIt != rEnd; ++rIt)
	{
		for (auto listener : m_Listeners)
		{
			listener->OnRouteAdded(rIt->Key, rIt->Data);
		}
	}
}
