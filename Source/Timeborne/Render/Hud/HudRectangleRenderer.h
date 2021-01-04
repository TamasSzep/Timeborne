// Timeborne/Render/Hud/HudRectangleRenderer.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

#include <Core/DataStructures/SimpleTypeUnorderedVector.hpp>
#include <EngineBuildingBlocks/Math/GLM.h>
#include <DirectX11Render/Primitive.h>
#include <DirectX11Render/Resources/VertexBuffer.h>

class HudRectangleRenderer
{
public:

	struct Rectangle
	{
		glm::vec2 middleInCs;
		glm::vec2 sizeInCs;
		glm::vec4 color;
		int z;
	};

private:

	static constexpr unsigned c_MaxCountRectangles = 64 * 1024;

	struct RectangleInstanceData
	{
		glm::vec2 middleInCs;
		glm::vec2 sizeInCs;
		glm::vec4 color;
	};

	DirectX11Render::IndexedPrimitive m_Primitive;
	DirectX11Render::VertexInputLayout m_MergedIL;
	DirectX11Render::VertexBuffer m_RectangleInstanceBuffer;
	unsigned m_PipelineStateIndex;

	bool m_Dirty = false;

	Core::SimpleTypeUnorderedVectorU<Rectangle> m_Rectangles;
	Core::SimpleTypeVectorU<std::pair<int, unsigned>> m_RenderOrder;
	Core::SimpleTypeVectorU<RectangleInstanceData> m_RectangleVector;

	void CreatePrimitive(const ComponentRenderContext& context);
	void CreateRectangleInstanceBuffer(const ComponentRenderContext& context);
	void CreatePipelineState(const ComponentRenderContext& context);

	void Update(const ComponentRenderContext& context);

public:

	unsigned AddRectangle(const Rectangle& rectangle);
	void RemoveRectangle(unsigned index);
	void RemoveRectangles(const Core::IndexVectorU& indices);
	void ClearRectangles();

	void InitializeRendering(const ComponentRenderContext& context);
	void RenderContent(const ComponentRenderContext& context);
};