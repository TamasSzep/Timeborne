// Timeborne/Render/Terrain/TerrainField.cpp

#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/Render/Terrain/TerrainField.h>

#include <DirectX11Render/Managers.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace DirectXRender;
using namespace DirectX11Render;

inline unsigned CreateTerrainFieldPS(const ComponentRenderContext& context, bool isInGame, bool wireframe, bool showGrid)
{
	std::vector<DirectXRender::ShaderDefine> defines;
	defines.push_back({ "IS_SHOWING_GRID", showGrid ? "1" : "0" });
	defines.push_back({ "IS_INGAME", isInGame ? "1" : "0" });
	auto vs = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "TerrainField.hlsl", "VSMain", ShaderType::Vertex, defines });
	auto hs = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "TerrainField.hlsl", "HSMain", ShaderType::Hull, defines });
	auto ds = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "TerrainField.hlsl", "DSMain", ShaderType::Domain, defines });
	auto ps = context.DX11M->ShaderManager.GetShaderSimple(context.Device, { "TerrainField.hlsl", "PSMain", ShaderType::Pixel, defines });

	PipelineStateDescription description;
	description.Shaders = { vs, hs, ds, ps };
	description.InputLayout = DirectX11Render::VertexInputLayout::Create(EngineBuildingBlocks::Graphics::VertexInputLayout::GetDummy());
	description.SamplerStates = { { SamplerStateDescription(), DirectXRender::ShaderFlagType::Hull } };
	description.RasterizerState.EnableMultisampling();
	if (wireframe) description.RasterizerState.SetWireframeFillMode();

	return context.DX11M->PipelineStateManager.GetPipelineStateIndex(context.Device, description);
}

unsigned& TerrainField::GetPSIndex(bool isInGame, bool wireframe, bool showGrid)
{
	return m_PSIndices[(int)isInGame * 4 + (int)wireframe * 2 + (int)showGrid];
}

void TerrainField::InitializeRendering(const ComponentRenderContext& context)
{
	for (int i = 0; i < 8; i++)
	{
		bool hasFieldIndices = (bool)(i & 4);
		bool wireframe = (bool)(i & 2);
		bool showGrid = (bool)(i & 1);
		GetPSIndex(hasFieldIndices, wireframe, showGrid) = CreateTerrainFieldPS(context, hasFieldIndices, wireframe, showGrid);
	}
}

void TerrainField::Render(const ComponentRenderContext& context, const Level* level, int countFieldsToRender, bool wireframe, bool showGrid)
{
	auto& terrain = level->GetTerrain();
	auto countFields = terrain.GetCountFields();
	
	// Checking whether visible field indices are provided in the vertex buffer.
	bool isInGame = (countFieldsToRender >= 0);
	auto countInstances = isInGame ? countFieldsToRender : (countFields.x * countFields.y);

	context.DX11M->PipelineStateManager.GetPipelineState(GetPSIndex(isInGame, wireframe, showGrid)).SetForContext(context.DeviceContext);
	context.DeviceContext->Draw(countInstances, 0);
}