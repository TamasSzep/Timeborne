// Timeborne/InGame/View/InGameView.cpp

#include <Timeborne/InGame/View/InGameView.h>

#include <Timeborne/InGame/View/GameObjects/AttackLineView.h>
#include <Timeborne/InGame/View/GameObjects/GameObjectInGameView.h>
#include <Timeborne/InGame/View/GameObjects/HealthPointBarView.h>
#include <Timeborne/InGame/View/GameObjects/PathView.h>
#include <Timeborne/InGame/View/Terrain/TerrainInGameView.h>
#include <Timeborne/InGame/View/BottomControl.h>
#include <Timeborne/InGame/GameState/ClientGameState.h>
#include <Timeborne/ApplicationComponent.h>

#include <EngineBuildingBlocks/Graphics/Lighting/Lighting1.h>
#include <DirectX11Render/Utilities/RenderPassCB.h>

using namespace EngineBuildingBlocks::Graphics;
using namespace DirectX11Render;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr unsigned c_MaxCountLights = 16;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

InGameView::InGameView()
{
	m_GameObjectView = new GameObjectInGameView();
	
	// The list of components defines the render order.
	m_Components.push_back(std::make_unique<TerrainInGameView>());
	m_Components.push_back(std::make_unique<PathView>());
	m_Components.push_back(std::make_unique<AttackLineView>());
	m_Components.push_back(std::unique_ptr<GameObjectInGameView>(m_GameObjectView));
	m_Components.push_back(std::make_unique<HealthPointBarView>(*m_GameObjectView));

	// GUI views.
	m_Components.push_back(std::make_unique<BottomControl>());
}

InGameView::~InGameView()
{
}

GameObjectVisibilityProvider& InGameView::GetGameObjectVisibilityProvider()
{
	assert(m_GameObjectView != nullptr);
	return *m_GameObjectView;
}

void InGameView::InitializeRendering(const ComponentRenderContext& context)
{
	m_RenderPassCB = CreateRenderPassCB(context.Device);
	m_LightingCB.Initialize(context.Device, sizeof(Lighting_Dir_Spot_CBType1<c_MaxCountLights>), 1);

	SetLightingCBData(context);

	for (auto& component : m_Components)
	{
		component->SetGraphicsResources(&m_RenderPassCB, &m_LightingCB);
		component->Initialize(context);
	}
}

void InGameView::DestroyRendering()
{
	for (auto& component : m_Components)
	{
		component->Destroy();
	}
}

void InGameView::Load(const Level& level, ClientGameState& clientGameState,
	EngineBuildingBlocks::Graphics::Camera& camera, const ComponentRenderContext& context)
{
	m_Camera = &camera;

	auto& gameCreationData = clientGameState.GetGameCreationData();
	auto& viewGameState = clientGameState.GetSyncedGameState();
	auto& localGameState = clientGameState.GetLocalGameState();

	for (auto& component : m_Components)
	{
		component->Load(camera, level, gameCreationData, viewGameState, localGameState, context);
	}
}

void InGameView::RenderFullscreenClear(const ComponentRenderContext& context)
{
	glm::vec4 backgroundColor(0.0f, 0.0f, 0.0f, 1.0f);
	context.DeviceContext->ClearRenderTargetView(context.RTV, glm::value_ptr(backgroundColor));
}

void InGameView::PreUpdate(const ComponentPreUpdateContext& context)
{
	UpdateRenderPassCBData();

	for (auto& component : m_Components)
	{
		component->PreUpdate(context);
	}
}

void InGameView::PostUpdate(const ComponentPostUpdateContext& context)
{
	for (auto& component : m_Components)
	{
		component->PostUpdate(context);
	}
}

void InGameView::RenderContent(const ComponentRenderContext& context)
{
	m_RenderPassCB.Update(context.DeviceContext);

	for (auto& component : m_Components)
	{
		component->RenderContent(context);
	}
}

void InGameView::RenderGUI(const ComponentRenderContext& context)
{
	for (auto& component : m_Components)
	{
		component->RenderGUI(context);
	}
}

void InGameView::SetLightingCBData(const ComponentRenderContext& context)
{
	auto& lightingCBData = m_LightingCB.Access<Lighting_Dir_Spot_CBType1<c_MaxCountLights>>(0);
	CreateDefaultLighting1(lightingCBData);
	m_LightingCB.Update(context.DeviceContext);
}

void InGameView::UpdateRenderPassCBData()
{
	assert(m_Camera != nullptr);

	UpdateRenderPassCB(m_Camera, &m_RenderPassCB);
}
