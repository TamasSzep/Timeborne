// Timeborne/Render/Hud/HudRectangleRenderer.cpp

#include <Timeborne/Render/Hud/HudRectangleRenderer.h>

#include <DirectX11Render/Managers.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace DirectXRender;
using namespace DirectX11Render;

void HudRectangleRenderer::CreatePrimitive(const ComponentRenderContext& context)
{
	m_Primitive = context.DX11M->PrimitiveManager.GetIndexedPrimitive(
		QuadDescription(PrimitiveRange::_Minus1_To_Plus1), context.Device);
}

void HudRectangleRenderer::CreateRectangleInstanceBuffer(const ComponentRenderContext& context)
{
	auto device = context.Device;

	const VertexElement elements[] =
	{
		{ "PositionInCs", VertexElementType::Float, sizeof(float), 2 },
		{ "SizeInCs", VertexElementType::Float, sizeof(float), 2 },
		{ "Color", VertexElementType::Float, sizeof(float), 4 }
	};
	EngineBuildingBlocks::Graphics::VertexInputLayout inputLayout;
	inputLayout.Elements.insert(inputLayout.Elements.end(), elements, elements + 3);

	m_RectangleInstanceBuffer.Initialize(context.Device, D3D11_USAGE_DYNAMIC, inputLayout, c_MaxCountRectangles);

	auto& vertexIL = m_Primitive.PVertexBuffer->GetInputLayout();
	const EngineBuildingBlocks::Graphics::VertexInputLayout* inputLayouts[]
		= { &vertexIL, &inputLayout };
	auto mergedInputLayout = EngineBuildingBlocks::Graphics::VertexInputLayout::Merge(inputLayouts, 2);
	Core::IndexVector inputSlots;
	Core::SimpleTypeVector<bool> isPerVertex;
	inputSlots.PushBack(0U, vertexIL.Elements.size());
	inputSlots.PushBack(1U, inputLayout.Elements.size());
	isPerVertex.PushBack(true, vertexIL.Elements.size());
	isPerVertex.PushBack(false, inputLayout.Elements.size());
	m_MergedIL = DirectX11Render::VertexInputLayout::Create(mergedInputLayout,
		inputSlots.GetArray(), isPerVertex.GetArray());
}

void HudRectangleRenderer::CreatePipelineState(const ComponentRenderContext& context)
{
	auto vs = context.DX11M->ShaderManager.GetShaderSimple(context.Device,
		{ "HudRectangle.hlsl", "VSMain", ShaderType::Vertex });
	auto ps = context.DX11M->ShaderManager.GetShaderSimple(context.Device,
		{ "HudRectangle.hlsl", "PSMain", ShaderType::Pixel });

	PipelineStateDescription description;
	description.Shaders = { vs, ps };
	description.InputLayout = m_MergedIL;
	description.RasterizerState.DisableCulling();
	description.DepthStencilState.DisableDepthWriting();
	description.DepthStencilState.Description.DepthEnable = FALSE;
	description.BlendState.SetToNonpremultipliedAlphaBlending();

	m_PipelineStateIndex = context.DX11M->PipelineStateManager.GetPipelineStateIndex(context.Device, description);
}

void HudRectangleRenderer::InitializeRendering(const ComponentRenderContext& context)
{
	CreatePrimitive(context);
	CreateRectangleInstanceBuffer(context);
	CreatePipelineState(context);
}

void HudRectangleRenderer::Update(const ComponentRenderContext& context)
{
	if (m_Dirty)
	{
		m_RenderOrder.Clear();
		auto it = m_Rectangles.GetBeginIterator();
		auto end = m_Rectangles.GetEndIterator();
		for (; it != end; ++it)
		{
			m_RenderOrder.PushBack({it->z, m_Rectangles.ToIndex(it)});
		}

		std::sort(m_RenderOrder.GetArray(), m_RenderOrder.GetEndPointer(), [](const auto& lhs, const auto& rhs) {
			return lhs.first < rhs.first;
		});

		m_RectangleVector.Clear();
		auto countRectangles = m_Rectangles.GetSize();
		for (unsigned i = 0; i < countRectangles; i++)
		{
			auto& source = m_Rectangles[m_RenderOrder[i].second];
			auto& target = m_RectangleVector.PushBackPlaceHolder();
			target.middleInCs = source.middleInCs;
			target.sizeInCs = source.sizeInCs;
			target.color = source.color;
		}

		m_RectangleInstanceBuffer.SetData(context.DeviceContext, m_RectangleVector.GetArray(),
			m_RectangleVector.GetSizeInBytes());

		m_Dirty = false;
	}
}

unsigned HudRectangleRenderer::AddRectangle(const Rectangle& rectangle)
{
	auto index = m_Rectangles.Add(rectangle);
	m_Dirty = true;
	return index;
}

void HudRectangleRenderer::RemoveRectangle(unsigned index)
{
	m_Rectangles.Remove(index);
	m_Dirty = true;
}

void HudRectangleRenderer::RemoveRectangles(const Core::IndexVectorU& indices)
{
	auto count = indices.GetSize();
	for (unsigned i = 0; i < count; i++)
	{
		m_Rectangles.Remove(indices[i]);
	}
	if (count > 0)
	{
		m_Dirty = true;
	}
}

void HudRectangleRenderer::ClearRectangles()
{
	m_Rectangles.Clear();
	m_Dirty = true;
}

void HudRectangleRenderer::RenderContent(const ComponentRenderContext& context)
{
	Update(context);

	if (m_RectangleVector.IsEmpty()) return;

	auto d3dContext = context.DeviceContext;

	context.DX11M->PipelineStateManager.GetPipelineState(m_PipelineStateIndex).SetForContext(d3dContext);

	ID3D11Buffer* vbs[] = { m_Primitive.PVertexBuffer->GetBuffer(), m_RectangleInstanceBuffer.GetBuffer() };
	unsigned vbStrides[] = { m_Primitive.PVertexBuffer->GetVertexStride(), m_RectangleInstanceBuffer.GetVertexStride() };
	unsigned vbOffsets[] = { 0U, 0U };
	d3dContext->IASetVertexBuffers(0, 2, vbs, vbStrides, vbOffsets);
	d3dContext->IASetIndexBuffer(m_Primitive.PIndexBuffer->GetBuffer(), DXGI_FORMAT_R32_UINT, 0);
	d3dContext->IASetPrimitiveTopology(c_PrimitiveTopologyMap[(int)m_Primitive.PIndexBuffer->GetTopology()]);

	d3dContext->DrawIndexedInstanced(m_Primitive.CountIndices, m_RectangleVector.GetSize(), m_Primitive.BaseIndex,
		m_Primitive.BaseVertex, 0U);
}