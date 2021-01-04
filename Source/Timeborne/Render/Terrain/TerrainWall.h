// Timeborne/Render/Terrain/TerrainWall.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

#include <DirectX11Render/Resources/VertexBuffer.h>

class Level;

class TerrainWall
{
private:

	DirectX11Render::VertexBuffer m_TerrainWallVB;
	unsigned m_PSIndices[8];

	struct WallVBType
	{
		glm::uvec2 FieldIndices;
		unsigned IsXWall;
	};

	std::vector<Core::SimpleTypeVectorU<WallVBType>> m_XWallVBData, m_ZWallVBData;
	unsigned m_ChunkSize;
	unsigned m_CountWalls;

	Core::SimpleTypeVectorU<WallVBType> m_WallVBData;

	unsigned& GetPSIndex(bool isInGame, bool wireframe, bool showGrid);

private: // Updating.

	void CreateGROnSizeChange(const ComponentRenderContext& context, const Level* level);
	void UpdateHeights(const ComponentRenderContext& context, const Level* level,
		const glm::ivec2& changeStart, const glm::ivec2& changeEnd);

public:

	void InitializeRendering(const ComponentRenderContext& context);
	void Update(const ComponentRenderContext& context, const Level* level,
		bool isSizeChanged, const glm::ivec2& changeStart, const glm::ivec2& changeEnd);
	void Render(const ComponentRenderContext& context, const Level* level, bool isInGame, bool wireframe, bool showGrid);
};