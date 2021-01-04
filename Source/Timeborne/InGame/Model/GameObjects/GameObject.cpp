// Timeborne/InGame/Model/GameObjects/GameObject.cpp

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>

#include <Core/Constants.h>
#include <Core/SimpleBinarySerialization.hpp>

#include <cassert>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GameObjectLevelData::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, PlayerIndex);
	Core::SerializeSB(bytes, TypeIndex);
	Core::SerializeSB(bytes, Pose);
}

void GameObjectLevelData::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, PlayerIndex);
	Core::DeserializeSB(bytes, TypeIndex);
	Core::DeserializeSB(bytes, Pose);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GameObject::GameObject()
	: Id(c_InvalidGameObjectId)
	, FightIndex(Core::c_InvalidIndexU)
{
	Data.PlayerIndex = Core::c_InvalidIndexU;
	Data.TypeIndex = (GameObjectTypeIndex)Core::c_InvalidIndexU;

	// The pose is default initialized.
}

void GameObject::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Id);
	Core::SerializeSB(bytes, Data);
	Core::SerializeSB(bytes, FightIndex);
}

void GameObject::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Id);
	Core::DeserializeSB(bytes, Data);
	Core::DeserializeSB(bytes, FightIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void AddListenerOnce(std::vector<T*>& listenerVector, T& listener)
{
	if (std::find(listenerVector.begin(), listenerVector.end(), &listener) == listenerVector.end())
	{
		listenerVector.push_back(&listener);
	}
}

void GameObjectList::AddExistenceListenerOnce(GameObjectExistenceListener& listener)
{
	::AddListenerOnce(m_ExistenceListeners, listener);
}

void GameObjectList::AddPoseListenerOnce(GameObjectPoseListener& listener)
{
	::AddListenerOnce(m_PoseListeners, listener);
}

void GameObjectList::AddFightListenerOnce(GameObjectFightListener& listener)
{
	::AddListenerOnce(m_FightListeners, listener);
}

void GameObjectList::Add(const GameObject& gameObject)
{
	assert(m_GameObjects.find(gameObject.Id) == m_GameObjects.end());
	m_GameObjects[gameObject.Id] = gameObject;
	for (auto listener : m_ExistenceListeners)
	{
		listener->OnGameObjectAdded(gameObject);
	}
}

void GameObjectList::Remove(GameObjectId id)
{
	auto gIt = m_GameObjects.find(id);
	assert(gIt != m_GameObjects.end());
	m_GameObjects.erase(gIt);
	for (auto listener : m_ExistenceListeners)
	{
		listener->OnGameObjectRemoved(id);
	}
}

void GameObjectList::Clear()
{
	while (!m_GameObjects.empty())
	{
		Remove(m_GameObjects.begin()->second.Id);
	}
}

void GameObjectList::SetPose(GameObjectId id, const GameObjectPose& pose)
{
	auto gIt = m_GameObjects.find(id);
	assert(gIt != m_GameObjects.end());
	gIt->second.Data.Pose = pose;
	for (auto listener : m_PoseListeners)
	{
		listener->OnGameObjectPoseChanged(id, pose);
	}
}

void GameObjectList::NotifyFightStateChanged(const GameObject& object, const GameObjectFightData& fightData)
{
	for (auto listener : m_FightListeners)
	{
		listener->OnGameObjectFightStateChanged(object, fightData);
	}
}

const GameObjectMap& GameObjectList::Get() const
{
	return m_GameObjects;
}

void GameObjectList::SerializeSB(Core::ByteVector& bytes) const
{
	// The serialized state only consists of the game objects, NOT the listeners.

	Core::SerializeSB(bytes, (unsigned)m_GameObjects.size());
	for (auto& gameObjectData : m_GameObjects)
	{
		Core::SerializeSB(bytes, gameObjectData.second);
	}
}

void GameObjectList::DeserializeSB(const unsigned char*& bytes)
{
	// The serialized state only consists of the game objects, NOT the listeners.

	m_GameObjects.clear();
	unsigned countGameObjects;
	Core::DeserializeSB(bytes, countGameObjects);
	for (unsigned i = 0; i < countGameObjects; i++)
	{
		GameObject gameObject;
		Core::DeserializeSB(bytes, gameObject);
		m_GameObjects[gameObject.Id] = gameObject;
	}
}

void GameObjectList::NotifyListenersWithFullState()
{
	for (auto& gameObject : m_GameObjects)
	{
		for (auto listener : m_ExistenceListeners)
		{
			listener->OnGameObjectAdded(gameObject.second);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GameObjectVisibilityListener::GameObjectVisibilityListener(GameObjectVisibilityProvider& provider)
{
	m_VisibilityProvider = &provider;

	provider._AddListener(*this);
}

void GameObjectVisibilityListener::_OnProviderDeleted()
{
	m_VisibilityProvider = nullptr;
}

GameObjectVisibilityListener::~GameObjectVisibilityListener()
{
	if (m_VisibilityProvider != nullptr)
	{
		m_VisibilityProvider->_RemoveListener(*this);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GameObjectVisibilityProvider::GameObjectVisibilityProvider()
{
}

GameObjectVisibilityProvider::~GameObjectVisibilityProvider()
{
	for (auto listener : m_Listeners)
	{
		listener->_OnProviderDeleted();
	}
}

void GameObjectVisibilityProvider::_AddListener(GameObjectVisibilityListener& listener)
{
	assert(std::find(m_Listeners.begin(), m_Listeners.end(), &listener) == m_Listeners.end());

	m_Listeners.push_back(&listener);
}

void GameObjectVisibilityProvider::_RemoveListener(GameObjectVisibilityListener& listener)
{
	m_Listeners.erase(std::remove(m_Listeners.begin(), m_Listeners.end(), &listener), m_Listeners.end());
}

void GameObjectVisibilityProvider::NotifyGameObjectVisibilityChanged()
{
	for (auto listener : m_Listeners)
	{
		listener->OnGameObjectVisibilityChanged(m_VisibleGameObjects);
	}
}
