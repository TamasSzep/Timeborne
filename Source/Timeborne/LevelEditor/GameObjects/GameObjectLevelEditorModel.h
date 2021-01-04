// Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorModel.h

#pragma once

#include <Timeborne/LevelEditor/LevelEditorComponent.h>

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectId.h>

#include <Core/DataStructures/SimpleTypeUnorderedVector.hpp>

#include <memory>

class GroundObjectTerrainTreeNodeMapping;

struct GameObjectLevelEditorData
{
	GameObjectLevelData LevelData;
	unsigned LevelGameObjectIndex;
	float Alpha;
};

class IGameObjectLevelEditorModelListener
{
public:
	virtual ~IGameObjectLevelEditorModelListener() {}
	virtual void OnObjectAdded(GameObjectId objectId,
		const GameObjectLevelEditorData& objectData) = 0;
	virtual void OnObjectRemoved(GameObjectId objectId) = 0;
	virtual void OnObjectPoseChanged(GameObjectId objectId,
		const GameObjectPose& pose) = 0;
	virtual void OnObjectAlphaChanged(GameObjectId objectId, float alpha) {}
};

class GameObjectLevelEditorModel : public LevelEditorComponent
{
private:

	std::vector<IGameObjectLevelEditorModelListener*> m_Listeners;

	Core::FastStdMap<GameObjectId, GameObjectLevelEditorData> m_ObjectData;

	GameObjectId AddObjectInternally(const GameObjectLevelData& goData,
		unsigned levelGameObjectIndex, float alpha);
	void RemoveObjectInternally(GameObjectId objectId);

private: // Object placement checking.

	GameObjectId AddForNodeMapping(const GameObjectLevelData& goData);
	bool ObjectCollidesWithLevelObject(const GameObjectLevelData& goData);

	unsigned m_NextGameObjectId = 0;
	std::unique_ptr<GroundObjectTerrainTreeNodeMapping> m_ObjectToNodeMapping;

public:

	GameObjectLevelEditorModel();
	~GameObjectLevelEditorModel() override;

	Core::FastStdMap<GameObjectId, GameObjectLevelEditorData>&
		GetObjectData();

	const GameObjectPose& GetObjectPose(GameObjectId objectId) const;

	void SetObjectAlpha(GameObjectId objectId, float alpha);
	void SetNonLevelObjectPose(GameObjectId objectId, const GameObjectPose& pose);

	GameObjectId TryAddLevelObject(const GameObjectLevelData& goData);
	GameObjectId AddNonLevelObject(const GameObjectLevelData& goData, float alpha);
	void RemoveObject(GameObjectId objectId);

	void GetObjectIdsForTerrain(const glm::ivec2& start,
		const glm::ivec2& end, Core::SimpleTypeVectorU<GameObjectId>& resObjectIds);

public: // Listener functionality.

	void AddListener(IGameObjectLevelEditorModelListener* listener);

public: // LevelEditorComponent IF.

	void OnLevelLoaded(LevelDirtyFlags dirtyFlags) override;
	void OnTerrainHeightsDirty(const glm::ivec2& changeStart, const glm::ivec2& changeEnd) override;
};