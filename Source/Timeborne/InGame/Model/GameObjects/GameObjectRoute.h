// Timeborne/InGame/Model/GameObjects/GameObjectRoute.h

#pragma once

#include <Timeborne/DataStructures/FastReusableResourceMap.h>
#include <Timeborne/InGame/Model/GameObjects/PathFinding/PathFinding.h>

#include <vector>

struct GameObjectRoute
{
	GameObjectPath Path;
	unsigned NextFieldIndex;
	glm::dvec2 OrientationTarget;

	static constexpr glm::dvec2 c_InvalidOrientationTarget
		= glm::dvec2(std::numeric_limits<double>::quiet_NaN());

	bool IsOrienting() const;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};

class GameObjectRouteList;

enum class RouteRemoveReason
{
	Ended, ObjectRemoved, Aborted
};

class GameObjectRouteListener
{
public:
	GameObjectRouteListener();
	virtual ~GameObjectRouteListener();

	virtual void OnRouteAdded(GameObjectId objectId, const GameObjectRoute& route) = 0;
	virtual void OnRouteRemoved(GameObjectId objectId, RouteRemoveReason reason) = 0;
};

class GameObjectRouteList
{
	std::vector<GameObjectRouteListener*> m_Listeners;

	FastReusableResourceMap<GameObjectId, GameObjectRoute> m_Routes;

public:

	void AddListenerOnce(GameObjectRouteListener& listener);

	GameObjectRoute& BeginAdd(GameObjectId objectId);
	void FinishAdd(GameObjectId objectId);
	void AbortAdd(GameObjectId objectId);

	void Remove(GameObjectId objectId, RouteRemoveReason reason);

	const GameObjectRoute* GetRoute(GameObjectId objectId) const;

	GameObjectRoute& AccessRoute(GameObjectId objectId); // Changes are not listened.

	const FastReusableResourceMap<GameObjectId, GameObjectRoute>& GetRoutes() const;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);

	void NotifyListenersWithFullState();
};