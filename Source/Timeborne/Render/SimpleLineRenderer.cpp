// Timeborne/Render/SimpleLineRenderer.cpp

#include <Timeborne/Render/SimpleLineRenderer.h>

#include <Timeborne/ApplicationComponent.h>

#include <DirectX11Render/Managers.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace DirectXRender;
using namespace DirectX11Render;

constexpr unsigned c_MaxCountPoints = 1024 * 1024;

SimpleLineRenderer::SimpleLineRenderer()
{
}

SimpleLineRenderer::~SimpleLineRenderer()
{
}

uint32_t SimpleLineRenderer::AddLine()
{
	m_Dirty = true;
	return m_Lines.AddNoReinit();
}

SimpleLineRenderer::LineData& SimpleLineRenderer::AccessLine(uint32_t index)
{
	m_Dirty = true;
	return m_Lines[index];
}

void SimpleLineRenderer::RemoveLine(uint32_t index)
{
	m_Dirty = true;
	m_Lines.RemoveNoReinit(index);
}

void SimpleLineRenderer::Clear()
{
	m_Dirty = true;
	m_Lines.Clear();
}

void SimpleLineRenderer::UpdateBuffers(const ComponentRenderContext& context)
{
	if (m_Dirty)
	{
		m_Dirty = false;

		m_VertexData.Clear();
		m_IndexData.Clear();
		auto it = m_Lines.GetBeginIterator();
		auto end = m_Lines.GetEndIterator();
		unsigned vertexIndex = 0;
		for (; it != end; ++it)
		{
			auto& line = *it;
			auto& linePositions = line.Positions;
			unsigned countCurrentPositions = linePositions.GetSize();
			if (countCurrentPositions > 0)
			{
				for (unsigned i = 0; i < countCurrentPositions; i++)
				{
					m_VertexData.PushBack({ linePositions[i], line.Color });
				}

				m_IndexData.PushBack(vertexIndex++);
				unsigned lastPosition = countCurrentPositions - 1;
				for (unsigned i = 1; i < lastPosition; i++, vertexIndex++)
				{
					m_IndexData.PushBack(vertexIndex, 2);
				}
				m_IndexData.PushBack(vertexIndex++);
			}
		}

		m_VertexBuffer.SetData(context.DeviceContext, m_VertexData.GetArray(), m_VertexData.GetSizeInBytes());
		m_IndexBuffer.SetData(context.DeviceContext, m_IndexData.GetArray(), 0, m_IndexData.GetSize());
	}
}

void SimpleLineRenderer::CreateBuffers(const ComponentRenderContext& context)
{
	EngineBuildingBlocks::Graphics::VertexInputLayout inputLayout;
	inputLayout.Elements.push_back(EngineBuildingBlocks::Graphics::c_PositionVertexElement);
	inputLayout.Elements.push_back({ "VertexColor", VertexElementType::Float, sizeof(float), 3 });

	m_VertexInputLayout = DirectX11Render::VertexInputLayout::Create(inputLayout);

	m_VertexBuffer.Initialize(context.Device, D3D11_USAGE_DYNAMIC, inputLayout, c_MaxCountPoints);
	m_IndexBuffer.Initialize(context.Device, D3D11_USAGE_DYNAMIC, PrimitiveTopology::LineList, 2 * c_MaxCountPoints);
}

void SimpleLineRenderer::CreatePipelineState(const ComponentRenderContext& context)
{
	auto vs = context.DX11M->ShaderManager.GetShaderSimple(context.Device,
		{ "SimpleLine.hlsl", "VSMain", ShaderType::Vertex });
	auto ps = context.DX11M->ShaderManager.GetShaderSimple(context.Device,
		{ "SimpleLine.hlsl", "PSMain", ShaderType::Pixel });

	PipelineStateDescription description;
	description.Shaders = { vs, ps };
	description.InputLayout = m_VertexInputLayout;
	description.RasterizerState.DisableCulling();
	description.DepthStencilState.DisableDepthWriting();
	description.DepthStencilState.Description.DepthEnable = FALSE;

	m_PipelineStateIndex = context.DX11M->PipelineStateManager.GetPipelineStateIndex(context.Device, description);
}

void SimpleLineRenderer::InitializeRendering(const ComponentRenderContext& context,
	DirectX11Render::ConstantBuffer* renderPassCB)
{
	m_RenderPassCB = renderPassCB;

	CreateBuffers(context);
	CreatePipelineState(context);
}

void SimpleLineRenderer::RenderContent(const ComponentRenderContext& context)
{
	assert(m_RenderPassCB != nullptr);

	UpdateBuffers(context);

	if (m_VertexData.IsEmpty()) return;

	auto d3dContext = context.DeviceContext;

	context.DX11M->PipelineStateManager.GetPipelineState(m_PipelineStateIndex).SetForContext(d3dContext);

	ID3D11Buffer* vbs[] = { m_VertexBuffer.GetBuffer() };
	unsigned vbStrides[] = { m_VertexBuffer.GetVertexStride() };
	unsigned vbOffsets[] = { 0U };
	d3dContext->IASetVertexBuffers(0, 1, vbs, vbStrides, vbOffsets);
	d3dContext->IASetIndexBuffer(m_IndexBuffer.GetBuffer(), DXGI_FORMAT_R32_UINT, 0);
	d3dContext->IASetPrimitiveTopology(c_PrimitiveTopologyMap[(int)m_IndexBuffer.GetTopology()]);

	ID3D11Buffer* cbs[] = { m_RenderPassCB->GetBuffer() };
	context.DeviceContext->VSSetConstantBuffers(0, 1, cbs);

	d3dContext->DrawIndexed(m_IndexData.GetSize(), 0, 0);
}
