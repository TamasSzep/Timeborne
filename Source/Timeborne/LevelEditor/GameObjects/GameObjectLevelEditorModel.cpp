// Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorModel.cpp

#include <Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorModel.h>

#include <Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/GroundObjectTerrainTreeNodeMapping.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>
#include <Timeborne/InGame/Model/Level.h>

GameObjectLevelEditorModel::GameObjectLevelEditorModel()
{
}

GameObjectLevelEditorModel::~GameObjectLevelEditorModel()
{
}

Core::FastStdMap<GameObjectId, GameObjectLevelEditorData>& GameObjectLevelEditorModel::GetObjectData()
{
	return m_ObjectData;
}

void GameObjectLevelEditorModel::AddListener(IGameObjectLevelEditorModelListener* listener)
{
	m_Listeners.push_back(listener);
}

GameObjectId GameObjectLevelEditorModel::AddForNodeMapping(const GameObjectLevelData& goData)
{
	assert(m_ObjectToNodeMapping != nullptr);

	auto objectId = GameObjectId::MakeId(m_NextGameObjectId++);

	GameObject gameObject;
	gameObject.Id = objectId;
	gameObject.Data.PlayerIndex = goData.PlayerIndex;
	gameObject.Data.TypeIndex = goData.TypeIndex;
	gameObject.Data.Pose = goData.Pose;

	m_ObjectToNodeMapping->AddObject(objectId, goData.TypeIndex, goData.Pose);

	return objectId;
}

GameObjectId GameObjectLevelEditorModel::AddObjectInternally(const GameObjectLevelData& goData,
	unsigned levelGameObjectIndex, float alpha)
{
	GameObjectId objectId = AddForNodeMapping(goData);

	GameObjectLevelEditorData objectData;
	objectData.LevelData = goData;
	objectData.LevelGameObjectIndex = levelGameObjectIndex;
	objectData.Alpha = alpha;

	m_ObjectData[objectId] = objectData;

	for (auto listener : m_Listeners)
	{
		listener->OnObjectAdded(objectId, objectData);
	}

	return objectId;
}

void GameObjectLevelEditorModel::RemoveObjectInternally(GameObjectId objectId)
{
	assert(m_ObjectToNodeMapping != nullptr);

	m_ObjectData.erase(objectId);
	m_ObjectToNodeMapping->RemoveObject(objectId);

	for (auto listener : m_Listeners)
	{
		listener->OnObjectRemoved(objectId);
	}
}

void GameObjectLevelEditorModel::OnLevelLoaded(LevelDirtyFlags dirtyFlags)
{
	assert(m_Level != nullptr);
	assert(m_Level->GetTerrainTree() != nullptr);
	auto& terrainTree = *m_Level->GetTerrainTree();

	if (HasFlag(dirtyFlags, LevelDirtyFlags::GameObjects)) // Unloading ALL game objects.
	{
		while (!m_ObjectData.empty())
		{
			RemoveObjectInternally(m_ObjectData.begin()->first);
		}
	}

	if (HasFlag(dirtyFlags, LevelDirtyFlags::TerrainTree))
	{
		// The terrain tree that is loaded here is not complete, however it contains all data required for the
		// functionality used here. See the comment TerrainTree.h for further details.
		
		if (HasFlag(dirtyFlags, LevelDirtyFlags::Terrain))
		{
			m_ObjectToNodeMapping = std::make_unique<GroundObjectTerrainTreeNodeMapping>(terrainTree);
		}
		else
		{
			assert(m_ObjectToNodeMapping != nullptr);
			m_ObjectToNodeMapping->SetTerrainTree_KeepingMappings(terrainTree);

		}
	}

	if (HasFlag(dirtyFlags, LevelDirtyFlags::GameObjects))
	{
		auto& newObjects = m_Level->GetGameObjects();
		auto goEnd = newObjects.GetEndIterator();
		for (auto goIt = newObjects.GetBeginIterator(); goIt != goEnd; ++goIt)
		{
			AddObjectInternally(*goIt, newObjects.ToIndex(goIt), 1.0f);
		}
	}
}

void GameObjectLevelEditorModel::OnTerrainHeightsDirty(const glm::ivec2& changeStart, const glm::ivec2& changeEnd)
{
	assert(m_Level != nullptr);
	auto& terrain = m_Level->GetTerrain();
	auto& levelGameObjects = m_Level->GetGameObjects();

	// Note that this can be implemented more effieciently with object-to-node mapping.

	auto compareChangeStart = changeStart - 1;
	auto compareChangeEnd = changeEnd + 2;

	auto& prototypes = GameObjectPrototype::GetPrototypes();

	for (auto& gameObjectData : m_ObjectData)
	{
		auto& objectData = gameObjectData.second;
		auto& oPose = objectData.LevelData.Pose;
		auto position2d = oPose.GetPosition2d();

		if (position2d.x < compareChangeStart.x || position2d.x > compareChangeEnd.x
			|| position2d.y < compareChangeStart.y || position2d.y > compareChangeEnd.y) continue;

		auto& prototype = *prototypes[(uint32_t)objectData.LevelData.TypeIndex];
		auto& movementPrototype = prototype.GetMovement();

		GameObjectPose pose;
		pose.SetPosition(terrain, position2d, movementPrototype.FlyHeight);
		pose.SetOrientationFromTerrain(terrain, oPose.GetYaw());

		if (objectData.LevelGameObjectIndex != Core::c_InvalidIndexU)
		{
			levelGameObjects[objectData.LevelGameObjectIndex].Pose = pose;
		}

		SetNonLevelObjectPose(gameObjectData.first, pose);
	}
}

const GameObjectPose& GameObjectLevelEditorModel::GetObjectPose(GameObjectId objectId) const
{
	auto oIt = m_ObjectData.find(objectId);
	assert(oIt != m_ObjectData.end());
	return oIt->second.LevelData.Pose;
}

void GameObjectLevelEditorModel::SetObjectAlpha(GameObjectId objectId, float alpha)
{
	auto oIt = m_ObjectData.find(objectId);
	assert(oIt != m_ObjectData.end());
	oIt->second.Alpha = alpha;

	for (auto listener : m_Listeners)
	{
		listener->OnObjectAlphaChanged(objectId, alpha);
	}
}

void GameObjectLevelEditorModel::SetNonLevelObjectPose(GameObjectId objectId, const GameObjectPose& pose)
{
	assert(m_ObjectToNodeMapping != nullptr);

	assert(m_Level != nullptr);
	auto& terrain = m_Level->GetTerrain();

	auto oIt = m_ObjectData.find(objectId);
	assert(oIt != m_ObjectData.end());

	oIt->second.LevelData.Pose = pose;

	m_ObjectToNodeMapping->SetObject(objectId, oIt->second.LevelData.TypeIndex, pose);

	for (auto listener : m_Listeners)
	{
		listener->OnObjectPoseChanged(objectId, pose);
	}
}

bool GameObjectLevelEditorModel::ObjectCollidesWithLevelObject(const GameObjectLevelData& goData)
{
	assert(m_ObjectToNodeMapping != nullptr);

	// Adding a temporary object.
	GameObjectId objectId = AddForNodeMapping(goData);

	// Checking the collision.
	bool result = m_ObjectToNodeMapping->IsObjectColliding(objectId, false, [this](GameObjectId otherObjectIndex) {

		auto oIt = m_ObjectData.find(otherObjectIndex);
		assert(oIt != m_ObjectData.end());

		// The other object index cannot be the temporary object. Checking whether it's a level object.
		return oIt->second.LevelGameObjectIndex != Core::c_InvalidIndexU;
	});

	// Cleaning up after the check.
	m_ObjectToNodeMapping->RemoveObject(objectId);

	return result;
}

GameObjectId GameObjectLevelEditorModel::TryAddLevelObject(const GameObjectLevelData& goData)
{
	if (!ObjectCollidesWithLevelObject(goData))
	{
		unsigned levelGameObjectIndex = m_Level->GetGameObjects().Add(goData);
		return AddObjectInternally(goData, levelGameObjectIndex, 1.0f);
	}
	return c_InvalidGameObjectId;
}

GameObjectId GameObjectLevelEditorModel::AddNonLevelObject(const GameObjectLevelData& goData, float alpha)
{
	return AddObjectInternally(goData, Core::c_InvalidIndexU, alpha);
}

void GameObjectLevelEditorModel::RemoveObject(GameObjectId objectId)
{
	auto oIt = m_ObjectData.find(objectId);
	assert(oIt != m_ObjectData.end());

	auto levelGameObjectIndex = oIt->second.LevelGameObjectIndex;
	if (levelGameObjectIndex != Core::c_InvalidIndexU)
	{
		m_Level->GetGameObjects().Remove(levelGameObjectIndex);
	}

	RemoveObjectInternally(objectId);
}

void GameObjectLevelEditorModel::GetObjectIdsForTerrain(const glm::ivec2& start, const glm::ivec2& end,
	Core::SimpleTypeVectorU<GameObjectId>& resObjectIds)
{
	assert(m_ObjectToNodeMapping != nullptr);

	auto terrainTree = m_Level->GetTerrainTree();
	assert(terrainTree != nullptr);

	resObjectIds.Clear();
	for (int i = start.x; i <= end.x; i++)
	{
		for (int j = start.y; j <= end.y; j++)
		{
			auto nodeIndex = terrainTree->GetNodeIndexForField({ i, j });
			auto objectIdsPtr = m_ObjectToNodeMapping->GetObjectsForNode(nodeIndex);
			if (objectIdsPtr != nullptr)
			{
				auto countObjects = objectIdsPtr->GetSize();
				auto objectIds = objectIdsPtr->GetArray();
				for (unsigned k = 0; k < countObjects; k++)
				{
					resObjectIds.PushBack(objectIds[k]);
				}
			}
		}
	}
	resObjectIds.SortAndRemoveDuplicates();
}
