// imeborne/InGame/View/GameObjects/GameObjectInGameView.h

#pragma once

#include <Timeborne/InGame/View/InGameViewComponent.h>

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>

#include <Core/SingleElementPoolAllocator.hpp>
#include <Core/DataStructures/SimpleTypeUnorderedVector.hpp>
#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <memory>
#include <set>
#include <vector>

class GameObjectRenderer;
class GameObjectTerrainTreeNodeMapping;

class GameObjectInGameView : public InGameViewComponent
	, public GameObjectExistenceListener
	, public GameObjectPoseListener
	, public GameObjectVisibilityProvider
{
private:

	struct GameObjectData
	{
		GameObject Object;
		unsigned RendererIndex;
		bool IsDynamic;
	};

	Core::FastStdMap<GameObjectId, GameObjectData> m_GameObjects;

	Core::FastStdSet<GameObjectId> m_StaticObjectIds, m_DynamicObjectIds;

	std::unique_ptr<GameObjectRenderer> m_GameObjectRenderer;

	std::unique_ptr<GameObjectTerrainTreeNodeMapping> m_ObjectNodeMapping;

	Core::IndexVectorU m_VisibleTerrainNodeIndices;
	Core::SimpleTypeVectorU<GameObjectId> m_VisibleObjectIds;
	Core::IndexVectorU m_VisibleObjectRendererIndices;

	void UpdateObjectPoseInRenderer(GameObjectRenderer& renderer,
		unsigned rendererIndex, const GameObjectPose& pose);
	void UpdateDynamicObjectPoses();
	void UpdateVisibleTerrainNodeIndices(
		const ComponentPreUpdateContext& context,
		EngineBuildingBlocks::Graphics::Camera& camera);
	void UpdateObjectNodeMapping(GameObjectRenderer& renderer,
		GameObjectId objectId, unsigned rendererIndex);
	void UpdateObjectNodeMapping();
	void UpdateVisibleObjectIndices();

public:
	GameObjectInGameView();
	~GameObjectInGameView() override;

public: // InGameViewComponent IF.

	void Initialize(const ComponentRenderContext& context) override;
	void Destroy() override;

	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;

protected:

	void OnLoading(const ComponentRenderContext& context) override;

public: // GameObjectExistenceListener IF.

	void OnGameObjectAdded(const GameObject& object) override;
	void OnGameObjectRemoved(GameObjectId objectId) override;

public: // GameObjectPoseListener IF.

	void OnGameObjectPoseChanged(GameObjectId objectId, const GameObjectPose& pose) override;

private: // Game object visibility provider IF.

	std::vector<GameObjectVisibilityListener*> m_VisibilityListeners;

public:

	void AddVisibilityListener(GameObjectVisibilityListener& listener);

public: // GameObjectVisibilityProvider IF.

	EngineBuildingBlocks::Math::AABoundingBox GetTransformedBox(
		GameObjectId id) override;
};
