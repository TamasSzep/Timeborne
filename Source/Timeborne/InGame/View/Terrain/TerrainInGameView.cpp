// Timeborne/InGame/View/Terrain/TerrainInGameView.cpp

#include <Timeborne/InGame/View/Terrain/TerrainInGameView.h>

#include <Timeborne/Render/Terrain/TerrainCommon.h>
#include <Timeborne/Render/Terrain/TerrainField.h>
#include <Timeborne/Render/Terrain/TerrainWall.h>
#include <Timeborne/InGame/GameCamera/GameCamera.h>
#include <Timeborne/InGame/Model/Level.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace DirectXRender;
using namespace DirectX11Render;

TerrainInGameView::TerrainInGameView()
	: m_TerrainCommon(std::make_unique<TerrainCommon>())
	, m_TerrainField(std::make_unique<TerrainField>())
	, m_TerrainWall(std::make_unique<TerrainWall>())
{
}

TerrainInGameView::~TerrainInGameView()
{
}

void TerrainInGameView::Initialize(const ComponentRenderContext& context)
{
	m_TerrainCommon->InitializeRendering(context);
	m_TerrainField->InitializeRendering(context);
	m_TerrainWall->InitializeRendering(context);
}

void TerrainInGameView::OnLoading(const ComponentRenderContext& context)
{
	if (m_Level == nullptr) return;

	auto& terrain = m_Level->GetTerrain();
	auto countFields = terrain.GetCountFields();
	if (countFields.x == 0 || countFields.y == 0) return;

	m_TerrainCommon->Update(context, m_Level, true, glm::uvec2(0), countFields - glm::uvec2(1));
	m_TerrainWall->Update(context, m_Level, true, glm::uvec2(0), countFields - glm::uvec2(1));
}

void TerrainInGameView::PreUpdate(const ComponentPreUpdateContext& context)
{
	auto camera = dynamic_cast<GameCamera*>(m_Camera);
	if (camera == nullptr) return;

	m_TerrainCommon->UpdateHierarchicalRendering(*context.ThreadPool, *m_Camera, m_Level->GetTerrainTree());
}

void TerrainInGameView::RenderContent(const ComponentRenderContext& context)
{
	if (m_Level == nullptr) return;

	auto camera = dynamic_cast<GameCamera*>(m_Camera);
	if (camera == nullptr) return;

	auto& terrain = m_Level->GetTerrain();
	auto countFields = terrain.GetCountFields();
	if (countFields.x == 0 || countFields.y == 0) return;

	// Rendering.
	m_TerrainCommon->PrepareFieldRendering(context, m_Level, m_RenderPassCB, true, camera->GetZoomToDefaultFactor());
	m_TerrainField->Render(context, m_Level, m_TerrainCommon->GetCountVisibleFields(), false, m_IsShowingTerrainGrid);
	m_TerrainWall->Render(context, m_Level, true, false, m_IsShowingTerrainGrid);
}