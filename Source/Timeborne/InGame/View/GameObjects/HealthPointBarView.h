// Timeborne/InGame/View/GameObjects/HealthPointBarView.h

#pragma once

#include <Timeborne/InGame/View/InGameViewComponent.h>

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectId.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <memory>
#include <sstream>
#include <string>

class HudRectangleRenderer;

class HealthPointBarView : public InGameViewComponent
                         , public GameObjectVisibilityListener
{
	Core::SimpleTypeVectorU<GameObjectId> m_SourceGameObjectIds;

private: // Rendering.

	std::unique_ptr<HudRectangleRenderer> m_HealthPointRenderer;
	Core::IndexVectorU m_HealtPointRectangleIndices;

	void UpdateSourceGameObjects();
	void UpdateHealthPointRendering(const ComponentPreUpdateContext& context,
		EngineBuildingBlocks::Graphics::Camera& camera);

public:

	explicit HealthPointBarView(GameObjectVisibilityProvider& visibilityProvider);
	~HealthPointBarView() override;

public: // InGameViewComponent IF.

	void Initialize(const ComponentRenderContext& context) override;
	void Destroy() override;

	void OnLoading(const ComponentRenderContext& context) override;

	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;

public: // GameObjectVisibilityListener IF.

	// Not listening to this function, only using the GetTransformedBox function
	// of the visibility provider.
	virtual void OnGameObjectVisibilityChanged(
		const Core::SimpleTypeVectorU<GameObjectVisibilityData>&
		visibleGameObjects) {}
};