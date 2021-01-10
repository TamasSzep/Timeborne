// Timeborne/InGame/View/InGameViewComponent.h

#pragma once

#include <Timeborne/Declarations/DirectX11RenderDeclarations.h>
#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <cassert>

struct ComponentPreUpdateContext;
struct ComponentPostUpdateContext;
struct ComponentRenderContext;
struct GameCreationData;
class Level;
class LocalGameState;
class ServerGameState;

class InGameViewComponent
{
protected: // Resource data.

	DirectX11Render::ConstantBuffer* m_RenderPassCB = nullptr;
	DirectX11Render::ConstantBuffer* m_LightingCB = nullptr;

protected: // Loaded data.

	EngineBuildingBlocks::Graphics::Camera* m_Camera = nullptr;
	const Level* m_Level = nullptr;
	const GameCreationData* m_GameCreationData = nullptr;
	ServerGameState* m_GameState = nullptr;
	const LocalGameState* m_LocalGameState = nullptr;

public:

	virtual ~InGameViewComponent() {}

	void SetGraphicsResources(DirectX11Render::ConstantBuffer* renderPassCB,
		DirectX11Render::ConstantBuffer* lightingCB)
	{
		assert(renderPassCB != nullptr && lightingCB != nullptr);

		m_RenderPassCB = renderPassCB;
		m_LightingCB = lightingCB;
	}

	void Load(EngineBuildingBlocks::Graphics::Camera& camera,
		const Level& level, const GameCreationData& gameCreationData,
		ServerGameState& gameState, const LocalGameState& localGameState,
		const ComponentRenderContext& context)
	{
		m_Camera = &camera;
		m_Level = &level;
		m_GameCreationData = &gameCreationData;
		m_GameState = &gameState;
		m_LocalGameState = &localGameState;
		OnLoading(context);
	}

	virtual void Initialize(const ComponentRenderContext& context) {}
	virtual void Destroy() {}

	virtual void OnLoading(const ComponentRenderContext& context) {}

	virtual void PreUpdate(const ComponentPreUpdateContext& context) {}
	virtual void PostUpdate(const ComponentPostUpdateContext& context) {}
	virtual void RenderContent(const ComponentRenderContext& context) {}
	virtual void RenderGUI(const ComponentRenderContext& context) {}
};