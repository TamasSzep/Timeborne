// Timeborne/InGame/View/InGameView.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <DirectX11Render/Resources/ConstantBuffer.h>

#include <memory>
#include <vector>

struct ComponentPreUpdateContext;
struct ComponentPostUpdateContext;
struct ComponentRenderContext;

class Level;
class ClientGameState;

class InGameViewComponent;
class GameObjectInGameView;
class GameObjectVisibilityProvider;

class InGameView
{
	std::vector<std::unique_ptr<InGameViewComponent>> m_Components;
	GameObjectInGameView* m_GameObjectView = nullptr;

private: // Loaded components.

	EngineBuildingBlocks::Graphics::Camera* m_Camera = nullptr;

private: // Rendering.

	DirectX11Render::ConstantBuffer m_LightingCB;
	DirectX11Render::ConstantBuffer m_RenderPassCB;

	void SetLightingCBData(const ComponentRenderContext& context);
	void UpdateRenderPassCBData();

public:

	InGameView();
	~InGameView();

	void Load(const Level& level, ClientGameState& clientGameState,
		EngineBuildingBlocks::Graphics::Camera& camera,
		const ComponentRenderContext& context);

	GameObjectVisibilityProvider& GetGameObjectVisibilityProvider();

	void InitializeRendering(const ComponentRenderContext& context);
	void DestroyRendering();

	void RenderFullscreenClear(const ComponentRenderContext& context);

	void PreUpdate(const ComponentPreUpdateContext& context);
	void PostUpdate(const ComponentPostUpdateContext& context);
	void RenderContent(const ComponentRenderContext& context);
	void RenderGUI(const ComponentRenderContext& context);
};