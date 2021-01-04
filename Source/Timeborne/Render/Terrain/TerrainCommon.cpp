// Timeborne/Render/Terrain/TerrainCommon.cpp

#include <Timeborne/Render/Terrain/TerrainCommon.h>

#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/Settings.h>

#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>
#include <EngineBuildingBlocks/Graphics/ViewFrustumCuller.h>

#include <queue>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Math;
using namespace DirectXRender;
using namespace DirectX11Render;

struct TerrainFieldCB
{
	unsigned FieldIndexShift;
	unsigned FieldIndexMask;
	glm::vec2 _Padding0;
	glm::vec4 FieldIndexToTexCoord;
	float ExplicitTessellationFactor;
};

void TerrainCommon::InitializeRendering(const ComponentRenderContext& context)
{
	m_TerrainFieldCB.Initialize(context.Device, sizeof(TerrainFieldCB), 1);
}

void TerrainCommon::UpdateCBData(const ComponentRenderContext& context, const Level* level, float zoomToDefaultFactor)
{
	auto& terrain = level->GetTerrain();
	auto countFields = terrain.GetCountFields();

	assert(Core::IsPowerOfTwo(countFields.x));
	float cx = 1.0f / countFields.x;
	float cy = 1.0f / countFields.y;
	auto& cbData = m_TerrainFieldCB.Access<TerrainFieldCB>();
	cbData.FieldIndexShift = (unsigned)std::round(std::log2(countFields.x));
	cbData.FieldIndexMask = countFields.x - 1;
	cbData.FieldIndexToTexCoord = glm::vec4(cx, cx * 0.5f, cy, cy * 0.5f);
	cbData.ExplicitTessellationFactor = context.Settings->InGame.TerrainTessellationBase / zoomToDefaultFactor;
	m_TerrainFieldCB.Update(context.DeviceContext);
}

void TerrainCommon::CreateGROnSizeChange(const ComponentRenderContext& context, const Level* level)
{
	auto& terrain = level->GetTerrain();
	auto countFields = terrain.GetCountFields();

	EngineBuildingBlocks::Graphics::VertexInputLayout inputLayout;
	inputLayout.Elements = { { "FieldIndex", VertexElementType::Uint32, sizeof(unsigned), 1 } };

	m_VertexBuffer = VertexBuffer();
	m_VertexBuffer.Initialize(context.Device, D3D11_USAGE_DYNAMIC, inputLayout, (countFields.x + 1) * (countFields.y + 1));

	Texture2DDescription desc(countFields.x, countFields.y, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1, D3D11_USAGE_DYNAMIC,
		DirectX11Render::TextureBindFlag::ShaderResource);
	for (int c = 0; c < 4; c++)
	{
		auto& texture = m_CoeffTextures[c];
		texture = Texture2D();
		texture.Initialize(context.Device, desc);

		m_SurfaceCoefficients[c].Resize(countFields.x * countFields.y);
	}
}

void TerrainCommon::UpdateHeights(const ComponentRenderContext& context, const Level* level,
	const glm::ivec2& changeStart, const glm::ivec2& changeEnd)
{
	auto& terrain = level->GetTerrain();
	auto countFields = terrain.GetCountFields();
	auto& surfaceCoeffs = terrain.GetSurfaceCoefficients();

	int sX2 = std::max(changeStart.x - 2, 0);
	int sY2 = std::max(changeStart.y - 2, 0);
	int eX2 = std::min(changeEnd.x + 2, (int)countFields.x - 1);
	int eY2 = std::min(changeEnd.y + 2, (int)countFields.y - 1);
	for (int y = sY2; y <= eY2; y++)
	{
		for (int x = sX2; x <= eX2; x++)
		{
			auto fieldIndex = y * countFields.x + x;
			auto coeffMatrixTr = glm::transpose(surfaceCoeffs[fieldIndex]);
			m_SurfaceCoefficients[0][fieldIndex] = coeffMatrixTr[0];
			m_SurfaceCoefficients[1][fieldIndex] = coeffMatrixTr[1];
			m_SurfaceCoefficients[2][fieldIndex] = coeffMatrixTr[2];
			m_SurfaceCoefficients[3][fieldIndex] = coeffMatrixTr[3];
		}
	}

	for (int c = 0; c < 4; c++)
	{
		m_CoeffTextures[c].SetData(context.DeviceContext, m_SurfaceCoefficients[c].GetArray(), 0);
	}
}

void TerrainCommon::Update(const ComponentRenderContext& context, const Level* level,
	bool isSizeChanged, const glm::ivec2& changeStart, const glm::ivec2& changeEnd)
{
	if (isSizeChanged)
	{
		CreateGROnSizeChange(context, level);
	}
	if (changeStart.x <= changeEnd.x && changeStart.y <= changeEnd.y)
	{
		UpdateHeights(context, level, changeStart, changeEnd);
	}
}

void TerrainCommon::PrepareFieldRendering(const ComponentRenderContext& context, const Level* level,
	DirectX11Render::ConstantBuffer* renderPassCB, bool isUsingHierarchicalRendering, float zoomToDefaultFactor)
{
	// Updating the visible field data.
	if (isUsingHierarchicalRendering)
	{
		m_VertexBuffer.SetData(context.DeviceContext, m_VisibleFields.GetArray(), m_VisibleFields.GetSizeInBytes());
	}

	UpdateCBData(context, level, zoomToDefaultFactor);

	ID3D11Buffer* cbs[] = { renderPassCB->GetBuffer(), m_TerrainFieldCB.GetBuffer() };
	context.DeviceContext->VSSetConstantBuffers(0, 2, cbs);
	context.DeviceContext->HSSetConstantBuffers(0, 2, cbs);
	context.DeviceContext->DSSetConstantBuffers(0, 2, cbs);
	context.DeviceContext->PSSetConstantBuffers(0, 2, cbs);

	ID3D11ShaderResourceView* srvs[] = {
		m_CoeffTextures[0].GetSRV(), m_CoeffTextures[1].GetSRV(), m_CoeffTextures[2].GetSRV(), m_CoeffTextures[3].GetSRV() };
	context.DeviceContext->HSSetShaderResources(0, 4, srvs);

	ID3D11Buffer* vbs[] = { m_VertexBuffer.GetBuffer() };
	unsigned strides[] = { m_VertexBuffer.GetVertexStride() };
	unsigned offsets[] = { 0U };
	context.DeviceContext->IASetVertexBuffers(0, 1, vbs, strides, offsets);
	context.DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	context.DeviceContext->IASetPrimitiveTopology(c_PrimitiveTopologyMap[(int)PrimitiveTopology::ControlPointPatchList_1]);
}

void TerrainCommon::UpdateHierarchicalRendering(Core::ThreadPool& threadPool, Camera& camera,
	const TerrainTree* terrainTree)
{
	if (terrainTree == nullptr) return;

	TerrainTree::CullParameters parameters{};
	parameters.outputType = TerrainTree::CullOutputType::Field;
	terrainTree->Cull(threadPool, camera, m_VisibleFields, parameters);
}

unsigned TerrainCommon::GetCountVisibleFields() const
{
	return m_VisibleFields.GetSize();
}
