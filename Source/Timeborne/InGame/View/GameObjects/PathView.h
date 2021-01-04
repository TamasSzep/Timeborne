// Timeborne/InGame/View/GameObjects/PathView.h

#pragma once

#include <Timeborne/InGame/View/InGameViewComponent.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectRoute.h>

#include <cstdint>

class SimpleLineRenderer;

class PathView : public InGameViewComponent
               , public GameObjectRouteListener
{
	std::unique_ptr<SimpleLineRenderer> m_LineRenderer;

	Core::FastStdMap<GameObjectId, uint32_t> m_RendererIndices;

public:

	PathView();
	~PathView() override;

public: // InGameViewComponent IF.

	void Initialize(const ComponentRenderContext& context) override;
	void Destroy() override;

	void OnLoading(const ComponentRenderContext& context) override;

	void RenderContent(const ComponentRenderContext& context) override;

public: // GameObjectRouteListener IF.

	void OnRouteAdded(GameObjectId objectId, const GameObjectRoute& route) override;
	void OnRouteRemoved(GameObjectId objectId, RouteRemoveReason reason) override;
};
