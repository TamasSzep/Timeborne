// Timeborne/LevelEditor/Terrain/TerrainLevelEditorView.cpp

#include <Timeborne/LevelEditor/Terrain/TerrainLevelEditorView.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/Misc/ScreenResolution.h>
#include <Timeborne/Render/Terrain/TerrainCommon.h>
#include <Timeborne/Render/Terrain/TerrainField.h>
#include <Timeborne/Render/Terrain/TerrainWall.h>
#include <Timeborne/MainApplication.h>

#include <Core/Utility.hpp>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace DirectXRender;
using namespace DirectX11Render;

TerrainLevelEditorView::TerrainLevelEditorView()
	: m_TerrainCommon(std::make_unique<TerrainCommon>())
	, m_TerrainField(std::make_unique<TerrainField>())
	, m_TerrainWall(std::make_unique<TerrainWall>())
	, m_ChangeStart(0)
	, m_ChangeEnd(0)
{
}

TerrainLevelEditorView::~TerrainLevelEditorView()
{
}

void TerrainLevelEditorView::OnLevelLoaded(LevelDirtyFlags dirtyFlags)
{
	if (HasFlag(dirtyFlags, LevelDirtyFlags::Terrain))
	{
		m_IsSizeChanged = true;
		m_ChangeStart = glm::uvec2(0);
		m_ChangeEnd = m_Level->GetTerrain().GetCountFields() - glm::uvec2(1);
	}
}

void TerrainLevelEditorView::OnTerrainHeightsDirty(const glm::ivec2& changeStart, const glm::ivec2& changeEnd)
{
	m_ChangeStart = glm::min(m_ChangeStart, changeStart);
	m_ChangeEnd = glm::max(m_ChangeEnd, changeEnd);
}

bool TerrainLevelEditorView::IsShowingTerrainGrid() const
{
	return m_IsShowingTerrainGrid;
}

void TerrainLevelEditorView::SetShowingTerrainGrid(bool isShowingGrid)
{
	m_IsShowingTerrainGrid = isShowingGrid;
}

bool TerrainLevelEditorView::IsRenderingTerrainWithWireframe() const
{
	return m_IsRenderingTerrainWithWireframe;
}

void TerrainLevelEditorView::SetRenderingTerrainWithWireframe(bool wireframe)
{
	m_IsRenderingTerrainWithWireframe = wireframe;
}

void TerrainLevelEditorView::InitializeRendering(const ComponentRenderContext& context)
{
	m_TerrainCommon->InitializeRendering(context);
	m_TerrainField->InitializeRendering(context);
	m_TerrainWall->InitializeRendering(context);

	InitializeTerrainFieldMarkerGR(context);
}

void TerrainLevelEditorView::RenderContent(const ComponentRenderContext& context)
{
	assert(m_RenderPassCB != nullptr);

	auto& terrain = m_Level->GetTerrain();
	auto countFields = terrain.GetCountFields();
	if (countFields.x == 0 || countFields.y == 0) return;

	// Updating.
	m_TerrainCommon->Update(context, m_Level, m_IsSizeChanged, m_ChangeStart, m_ChangeEnd);
	m_TerrainWall->Update(context, m_Level, m_IsSizeChanged, m_ChangeStart, m_ChangeEnd);

	m_IsSizeChanged = false;
	m_ChangeStart = m_Level->GetTerrain().GetCountFields();
	m_ChangeEnd = glm::ivec2(-1);

	// Rendering.
	m_TerrainCommon->PrepareFieldRendering(context, m_Level, m_RenderPassCB, false, 0.0f);
	m_TerrainField->Render(context, m_Level, -1, m_IsRenderingTerrainWithWireframe, m_IsShowingTerrainGrid);
	m_TerrainWall->Render(context, m_Level, false, m_IsRenderingTerrainWithWireframe, m_IsShowingTerrainGrid);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct MarkerCB
{
	glm::uvec2 Start;
	glm::uvec2 End;
	unsigned CornerIndex;
	glm::uvec3 _Padding0;
	glm::vec4 Color;
};

inline unsigned CreateTerrainFieldMarkersPS(const ComponentRenderContext& context)
{
	auto vs = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "LevelEditor/BlockTool.hlsl", "VSMain", ShaderType::Vertex });
	auto hs = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "LevelEditor/BlockTool.hlsl", "HSMain", ShaderType::Hull });
	auto ds = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "LevelEditor/BlockTool.hlsl", "DSMain", ShaderType::Domain });
	auto ps = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "LevelEditor/BlockTool.hlsl", "PSMain", ShaderType::Pixel });

	PipelineStateDescription description;
	description.Shaders = { vs, hs, ds, ps };
	description.InputLayout = DirectX11Render::VertexInputLayout::Create(EngineBuildingBlocks::Graphics::VertexInputLayout::GetDummy());
	description.SamplerStates = { { SamplerStateDescription(), DirectXRender::ShaderFlagType::Hull } };
	description.RasterizerState.EnableMultisampling();
	description.BlendState.SetToNonpremultipliedAlphaBlending();
	description.DepthStencilState.DisableDepthWriting();
	description.RasterizerState.Description.DepthBias = -10;

	return context.DX11M->PipelineStateManager.GetPipelineStateIndex(context.Device, description);
}

void TerrainLevelEditorView::InitializeTerrainFieldMarkerGR(const ComponentRenderContext& context)
{
	m_TerrainFieldMarkerCB.Initialize(context.Device, sizeof(MarkerCB), 1);
	m_TerrainFieldMarkersPS = CreateTerrainFieldMarkersPS(context);
}

void TerrainLevelEditorView::RenderFieldMarkers(const ComponentRenderContext& context,
	const glm::ivec2& start, const glm::ivec2& end, unsigned cornerIndex, const glm::vec4& color)
{
	assert(m_RenderPassCB != nullptr);

	auto countInstances = (end.x - start.x + 1) * (end.y - start.y + 1);

	auto& cbData = m_TerrainFieldMarkerCB.Access<MarkerCB>();
	cbData.Start = start;
	cbData.End = end;
	cbData.CornerIndex = cornerIndex;
	cbData.Color = color;
	m_TerrainFieldMarkerCB.Update(context.DeviceContext);

	m_TerrainCommon->PrepareFieldRendering(context, m_Level, m_RenderPassCB, false, 0.0f);

	// Setting the terrain field marker constant buffer ADDITIONALLY.
	ID3D11Buffer* cbs[] = { m_TerrainFieldMarkerCB.GetBuffer() };
	context.DeviceContext->HSSetConstantBuffers(2, 1, cbs);
	context.DeviceContext->DSSetConstantBuffers(2, 1, cbs);
	context.DeviceContext->PSSetConstantBuffers(2, 1, cbs);

	context.DX11M->PipelineStateManager.GetPipelineState(m_TerrainFieldMarkersPS).SetForContext(context.DeviceContext);
	context.DeviceContext->Draw(countInstances, 0);
}

void TerrainLevelEditorView::RenderFieldMarkers(const ComponentRenderContext& context,
	const Core::SimpleTypeVectorU<glm::ivec2>& fieldIndices, unsigned cornerIndex,
	const glm::vec4& color)
{
	// Batching markers together.

	auto countFieldIndices = fieldIndices.GetSize();
	if (countFieldIndices == 0)
	{
		return;
	}

	int z = fieldIndices[0].y;
	int startX = fieldIndices[0].x;
	int endX = startX;
	
	auto flushFieldRendering = [&] {
		RenderFieldMarkers(context, glm::ivec2(startX, z), glm::ivec2(endX, z), cornerIndex, color);
	};
	
	for (unsigned i = 0; i < countFieldIndices; i++)
	{
		auto& fieldIndex = fieldIndices[i];
		if (fieldIndex.x == endX + 1 && fieldIndex.y == z)
		{
			endX = fieldIndex.x;
		}
		else
		{
			flushFieldRendering();
			z = fieldIndex.y;
			startX = fieldIndex.x;
			endX = startX;
		}
	}
	flushFieldRendering();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TerrainLevelEditorView::RenderGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	char levelSizeStr[32];
	snprintf(levelSizeStr, 32, "%d x %d", m_Level->GetCountFields().x, m_Level->GetCountFields().y);

	auto mwStart = glm::vec2(0, c_LevelEditor_MainGUIWindowHeight + c_LevelEditor_ToolGUIWindowHeight);
	auto mwSize = glm::vec2(context.WindowSize.x - context.ContentSize.x, 200);
	auto labelSize = glm::vec2(100, 20);

	if (Nuklear_BeginWindow(ctx, "Level data", glm::vec2(mwStart.x, mwStart.y), glm::vec2(mwSize.x, mwSize.y)))
	{
		nk_layout_row_static(ctx, labelSize.y, (int)labelSize.x, 2);
		nk_label(ctx, "Name", NK_TEXT_LEFT);
		nk_label(ctx, m_Level->GetName().c_str(), NK_TEXT_LEFT);
		nk_label(ctx, "Size", NK_TEXT_LEFT);
		nk_label(ctx, levelSizeStr, NK_TEXT_LEFT);
	}
	nk_end(ctx);
}