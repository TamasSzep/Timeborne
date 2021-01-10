// Timeborne/InGame/Model/GameObjects/GameObject.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/GameObjectId.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectPose.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectTypeIndex.h>

#include <Core/Constants.h>
#include <Core/SingleElementPoolAllocator.hpp>
#include <Core/DataStructures/ResourceUnorderedVector.hpp>
#include <EngineBuildingBlocks/Math/AABoundingBox.h>
#include <EngineBuildingBlocks/Math/GLM.h>

#include <cstdint>
#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct GameObjectLevelData
{
	unsigned PlayerIndex;
	GameObjectTypeIndex TypeIndex;
	GameObjectPose Pose;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct GameObject
{
	GameObjectId Id;
	GameObjectLevelData Data;
	uint32_t FightIndex;

	GameObject();

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};

using GameObjectMap = Core::FastStdMap<GameObjectId, GameObject>;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ServerGameState;

struct GameObjectData
{
	ServerGameState* ClientModelGameState = nullptr;
	unsigned NextGameObjectId = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class GameObjectExistenceListener
{
public:
	virtual ~GameObjectExistenceListener() {}
	virtual void OnGameObjectAdded(const GameObject& object) = 0;
	virtual void OnGameObjectRemoved(GameObjectId objectId) = 0;
};

class GameObjectPoseListener
{
public:
	virtual ~GameObjectPoseListener() {}
	virtual void OnGameObjectPoseChanged(GameObjectId objectId, const GameObjectPose& pose) = 0;
};

struct GameObjectFightData;

class GameObjectFightListener
{
public:
	virtual ~GameObjectFightListener() {}
	virtual void OnGameObjectDestroyed(const GameObject& sourceObject, const GameObject& targetObject) = 0;
	virtual void OnGameObjectFightStateChanged(const GameObject& object, const GameObjectFightData& fightData) = 0;
};

class GameObjectList
{
	std::vector<GameObjectExistenceListener*> m_ExistenceListeners;
	std::vector<GameObjectPoseListener*> m_PoseListeners;
	std::vector<GameObjectFightListener*> m_FightListeners;

	GameObjectMap m_GameObjects;

public:

	void AddExistenceListenerOnce(GameObjectExistenceListener& listener);
	void AddPoseListenerOnce(GameObjectPoseListener& listener);
	void AddFightListenerOnce(GameObjectFightListener& listener);

	void Add(const GameObject& gameObject);
	void Remove(GameObjectId id);
	void Clear();

	void SetPose(GameObjectId id, const GameObjectPose& pose);
	
	void NotifyGameObjectDestroyed(const GameObject& source, const GameObject& target);
	void NotifyFightStateChanged(const GameObject& object, const GameObjectFightData& fightData);

	const GameObjectMap& Get() const;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);

	void NotifyListenersWithFullState();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class GameObjectVisibilityProvider;

struct GameObjectVisibilityData
{
	GameObjectId Id;
	EngineBuildingBlocks::Math::AABoundingBox Box;
};

class GameObjectVisibilityListener
{
protected:

	GameObjectVisibilityProvider* m_VisibilityProvider = nullptr;

public:

	explicit GameObjectVisibilityListener(GameObjectVisibilityProvider& provider);
	void _OnProviderDeleted();

	virtual ~GameObjectVisibilityListener();
	virtual void OnGameObjectVisibilityChanged(
		const Core::SimpleTypeVectorU<GameObjectVisibilityData>& visibleGameObjects) = 0;
};

class GameObjectVisibilityProvider
{
protected:

	std::vector<GameObjectVisibilityListener*> m_Listeners;
	Core::SimpleTypeVectorU<GameObjectVisibilityData> m_VisibleGameObjects;

	void NotifyGameObjectVisibilityChanged();

public:

	GameObjectVisibilityProvider();

	void _AddListener(GameObjectVisibilityListener& listener);
	void _RemoveListener(GameObjectVisibilityListener& listener);

	virtual ~GameObjectVisibilityProvider();
	virtual EngineBuildingBlocks::Math::AABoundingBox GetTransformedBox(
		GameObjectId id) = 0;
};