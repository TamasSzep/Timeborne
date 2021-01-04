// Timeborne/Render/Terrain/TerrainCommon.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>
#include <Timeborne/ApplicationComponent.h>

#include <DirectX11Render/Resources/ConstantBuffer.h>
#include <DirectX11Render/Resources/Texture2D.h>
#include <DirectX11Render/Resources/VertexBuffer.h>

class Level;
class TerrainTree;

class TerrainCommon
{
	// The vertex buffer is only used for the hierarchical rendering.
	DirectX11Render::VertexBuffer m_VertexBuffer;

	DirectX11Render::ConstantBuffer m_TerrainFieldCB;
	DirectX11Render::Texture2D m_CoeffTextures[4];

	Core::SimpleTypeVectorU<glm::vec4> m_SurfaceCoefficients[4];

private: // Updating.

	void UpdateCBData(const ComponentRenderContext& context, const Level* level, float zoomToDefaultFactor);
	void CreateGROnSizeChange(const ComponentRenderContext& context, const Level* level);
	void UpdateHeights(const ComponentRenderContext& context, const Level* level,
		const glm::ivec2& changeStart, const glm::ivec2& changeEnd);

public:
	void InitializeRendering(const ComponentRenderContext& context);
	void Update(const ComponentRenderContext& context, const Level* level,
		bool isSizeChanged, const glm::ivec2& changeStart, const glm::ivec2& changeEnd);
	void PrepareFieldRendering(const ComponentRenderContext& context,
		const Level* level,
		DirectX11Render::ConstantBuffer* renderPassCB,
		bool isUsingHierarchicalRendering, float zoomToDefaultFactor);

private: // Hierarchical rendering.

	Core::IndexVectorU m_VisibleFields;

public:

	void UpdateHierarchicalRendering(
		Core::ThreadPool& threadPool,
		EngineBuildingBlocks::Graphics::Camera& camera,
		const TerrainTree* terrainTree);

	unsigned GetCountVisibleFields() const;
};