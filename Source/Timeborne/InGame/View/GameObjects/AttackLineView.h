// Timeborne/InGame/View/GameObjects/AttackLineView.h

#pragma once

#include <Timeborne/InGame/View/InGameViewComponent.h>

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>

#include <Core/SingleElementPoolAllocator.hpp>

#include <cstdint>

class SimpleLineRenderer;

class AttackLineView : public InGameViewComponent
	, public GameObjectPoseListener
	, public GameObjectFightListener
{
	std::unique_ptr<SimpleLineRenderer> m_LineRenderer;

	Core::FastStdMap<GameObjectId, uint32_t> m_RendererIndices;
	Core::SimpleTypeVectorU<GameObjectId> m_SourceGameObjectIds;
	Core::SimpleTypeVectorU<GameObjectId> m_DirtyIds;

	void UpdateSourceGameObjectIds();
	void UpdateDirtyObjects();

public:

	AttackLineView();
	~AttackLineView() override;

public: // InGameViewComponent IF.

	void Initialize(const ComponentRenderContext& context) override;
	void Destroy() override;

	void OnLoading(const ComponentRenderContext& context) override;

	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;

public: // GameObjectPoseListener IF.

	void OnGameObjectPoseChanged(GameObjectId objectId, const GameObjectPose& pose) override;

public: // GameObjectFightListener IF.

	void OnGameObjectFightStateChanged(const GameObject& object, const GameObjectFightData& fightData) override;
};
