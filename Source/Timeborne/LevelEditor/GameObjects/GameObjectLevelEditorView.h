// Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorView.h

#pragma once

#include <Timeborne/LevelEditor/LevelEditorComponent.h>

#include <Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorModel.h>

#include <memory>

class GameObjectRenderer;

class GameObjectLevelEditorView : public LevelEditorComponent,
                                      public IGameObjectLevelEditorModelListener
{
private:

	GameObjectLevelEditorModel& m_GameObjectModel;

	// The renderer's storage is held SoA with the game object's storage.
	std::unique_ptr<GameObjectRenderer> m_GameObjectRenderer;

	Core::FastStdMap<GameObjectId, unsigned> m_IndexMapping;

	Core::IndexVectorU m_AllGameObjectIndices;
	bool m_IndicesDirty = false;

	unsigned GetRendererIndex(GameObjectId objectId) const;

public:

	explicit GameObjectLevelEditorView(GameObjectLevelEditorModel& model);
	~GameObjectLevelEditorView() override;
	
public: // LevelEditorComponent IF.

	void InitializeRendering(const ComponentRenderContext& context) override;
	void DestroyRendering() override;

	void PreUpdate(const ComponentPreUpdateContext& context) override;

	void RenderContent(const ComponentRenderContext& context) override;

public: // IGameObjectLevelEditorModelListener IF.

	void OnObjectAdded(GameObjectId objectId, const GameObjectLevelEditorData& objectData) override;
	void OnObjectRemoved(GameObjectId objectId) override;
	void OnObjectPoseChanged(GameObjectId objectId, const GameObjectPose& pose) override;
	void OnObjectAlphaChanged(GameObjectId objectId, float alpha) override;
};