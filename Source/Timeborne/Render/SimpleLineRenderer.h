// Timeborne/Render/SimpleLineRenderer.h

#pragma once

#include <Core/DataStructures/ResourceUnorderedVector.hpp>
#include <Core/Constants.h>
#include <DirectX11Render/Resources/ConstantBuffer.h>
#include <DirectX11Render/Resources/IndexBuffer.h>
#include <DirectX11Render/Resources/VertexBuffer.h>

#include <cstdint>

struct ComponentRenderContext;

class SimpleLineRenderer
{
public:

	struct LineData
	{
		glm::vec3 Color;
		Core::SimpleTypeVectorU<glm::vec3> Positions;
	};

private:

	DirectX11Render::ConstantBuffer* m_RenderPassCB = nullptr;

	DirectX11Render::VertexInputLayout m_VertexInputLayout;
	DirectX11Render::VertexBuffer m_VertexBuffer;
	DirectX11Render::IndexBuffer m_IndexBuffer;
	uint32_t m_PipelineStateIndex = Core::c_InvalidIndexU;

	bool m_Dirty = false;

	Core::ResourceUnorderedVectorU<LineData> m_Lines;

	void CreateBuffers(const ComponentRenderContext& context);
	void CreatePipelineState(const ComponentRenderContext& context);

private:

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Color;
	};

	Core::SimpleTypeVectorU<Vertex> m_VertexData;
	Core::IndexVectorU m_IndexData;

	void UpdateBuffers(const ComponentRenderContext& context);

public:

	SimpleLineRenderer();
	~SimpleLineRenderer();

	uint32_t AddLine();
	LineData& AccessLine(uint32_t index);
	void RemoveLine(uint32_t index);
	void Clear();

	void InitializeRendering(const ComponentRenderContext& context,
		DirectX11Render::ConstantBuffer* renderPassCB);
	void RenderContent(const ComponentRenderContext& context);
};