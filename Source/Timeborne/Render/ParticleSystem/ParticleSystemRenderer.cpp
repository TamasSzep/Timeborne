// Timeborne/Render/ParticleSystem/ParticleSystemRenderer.cpp

#include <Timeborne/Render/ParticleSystem/ParticleSystemRenderer.h>

#include <Timeborne/ApplicationComponent.h>

#include <DirectX11Render/Managers.h>

using namespace EngineBuildingBlocks::Graphics;
using namespace DirectXRender;
using namespace DirectX11Render;

constexpr uint32_t c_MaxCountParticles = 1024 * 1024;

uint32_t ParticleSystemRenderer::AddParticleSystem(const ParticleSystem_SystemParameters& systemParameters, 
	const ParticleSystemParameters& parameters)
{
	uint32_t systemIndex = m_ParticleSystems.AddNoReinit();
	m_ParticleSystems[systemIndex].Reset(systemParameters, parameters);
	return systemIndex;
}

void ParticleSystemRenderer::RemoveParticleSystem(uint32_t systemIndex)
{
	m_ParticleSystems.RemoveNoReinit(systemIndex);
}

void ParticleSystemRenderer::CreatePrimitive(const ComponentRenderContext& context)
{
	m_Primitive = context.DX11M->PrimitiveManager.GetIndexedPrimitive(
		QuadDescription(PrimitiveRange::_Minus1_To_Plus1), context.Device);
}

void ParticleSystemRenderer::CreateInstanceBuffer(const ComponentRenderContext& context)
{
	auto device = context.Device;

	const VertexElement elements[] =
	{
		{ "PositionInCs", VertexElementType::Float, sizeof(float), 3 },
		{ "SizeInCs", VertexElementType::Float, sizeof(float), 2 },
		{ "Color", VertexElementType::Float, sizeof(float), 4 }
	};
	constexpr auto c_CountVertexElements = sizeof(elements) / sizeof(elements[0]);
	EngineBuildingBlocks::Graphics::VertexInputLayout inputLayout;
	inputLayout.Elements.insert(inputLayout.Elements.end(), elements, elements + c_CountVertexElements);

	m_InstanceBuffer.Initialize(context.Device, D3D11_USAGE_DYNAMIC, inputLayout, c_MaxCountParticles);

	const auto& vertexIL = m_Primitive.PVertexBuffer->GetInputLayout();
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

void ParticleSystemRenderer::CreatePipelineState(const ComponentRenderContext& context)
{
	auto vs = context.DX11M->ShaderManager.GetShaderSimple(context.Device,
		{ "Particle.hlsl", "VSMain", ShaderType::Vertex });
	auto ps = context.DX11M->ShaderManager.GetShaderSimple(context.Device,
		{ "Particle.hlsl", "PSMain", ShaderType::Pixel });

	PipelineStateDescription description;
	description.Shaders = { vs, ps };
	description.InputLayout = m_MergedIL;
	description.RasterizerState.DisableCulling();
	description.DepthStencilState.DisableDepthWriting();
	description.BlendState.SetToNonpremultipliedAlphaBlending();

	m_PipelineStateIndex = context.DX11M->PipelineStateManager.GetPipelineStateIndex(context.Device, description);
}

void ParticleSystemRenderer::InitializeRendering(const ComponentRenderContext& context)
{
	CreatePrimitive(context);
	CreateInstanceBuffer(context);
	CreatePipelineState(context);
}

void ParticleSystemRenderer::UpdateParticleInstances(const ComponentRenderContext& context)
{
	// @todo...
}

void ParticleSystemRenderer::RenderContent(const ComponentRenderContext& context)
{
	UpdateParticleInstances(context);

	if (m_ParticleData.IsEmpty()) return;

	auto d3dContext = context.DeviceContext;

	context.DX11M->PipelineStateManager.GetPipelineState(m_PipelineStateIndex).SetForContext(d3dContext);

	ID3D11Buffer* vbs[] = { m_Primitive.PVertexBuffer->GetBuffer(), m_InstanceBuffer.GetBuffer() };
	unsigned vbStrides[] = { m_Primitive.PVertexBuffer->GetVertexStride(), m_InstanceBuffer.GetVertexStride() };
	unsigned vbOffsets[] = { 0U, 0U };
	d3dContext->IASetVertexBuffers(0, 2, vbs, vbStrides, vbOffsets);
	d3dContext->IASetIndexBuffer(m_Primitive.PIndexBuffer->GetBuffer(), DXGI_FORMAT_R32_UINT, 0);
	d3dContext->IASetPrimitiveTopology(c_PrimitiveTopologyMap[(int)m_Primitive.PIndexBuffer->GetTopology()]);

	d3dContext->DrawIndexedInstanced(m_Primitive.CountIndices, m_ParticleData.GetSize(), m_Primitive.BaseIndex,
		m_Primitive.BaseVertex, 0U);
}
