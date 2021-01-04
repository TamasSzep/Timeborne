// Timeborne/LevelEditor/Terrain/TerrainLevelEditorView.h

#pragma once

#include <Timeborne/LevelEditor/LevelEditorComponent.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <DirectX11Render/Resources/ConstantBuffer.h>

class TerrainCommon;
class TerrainField;
class TerrainWall;

class TerrainLevelEditorView : public LevelEditorComponent
{
private: // Terrain.

	bool m_IsShowingTerrainGrid = false;
	bool m_IsRenderingTerrainWithWireframe = false;
	
	std::unique_ptr<TerrainCommon> m_TerrainCommon;
	std::unique_ptr<TerrainField> m_TerrainField;
	std::unique_ptr<TerrainWall> m_TerrainWall;

private: // Marker rendering.

	DirectX11Render::ConstantBuffer m_TerrainFieldMarkerCB;
	unsigned m_TerrainFieldMarkersPS = Core::c_InvalidIndexU;

	void InitializeTerrainFieldMarkerGR(const ComponentRenderContext& context);

public:

	void RenderFieldMarkers(const ComponentRenderContext& context,
		const glm::ivec2& start, const glm::ivec2& end, unsigned cornerIndex,
		const glm::vec4& color);

	// Expectes SORTED (first Z, second X) field indices.
	void RenderFieldMarkers(const ComponentRenderContext& context,
		const Core::SimpleTypeVectorU<glm::ivec2>& fieldIndices, unsigned cornerIndex,
		const glm::vec4& color);

private: // Updating terrain.

	bool m_IsSizeChanged = true;
	glm::ivec2 m_ChangeStart, m_ChangeEnd;

public:

	TerrainLevelEditorView();
	~TerrainLevelEditorView() override;

	void InitializeRendering(const ComponentRenderContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;

	void OnLevelLoaded(LevelDirtyFlags dirtyFlags) override;
	void OnTerrainHeightsDirty(const glm::ivec2& changeStart, const glm::ivec2& changeEnd) override;

	bool IsShowingTerrainGrid() const;
	void SetShowingTerrainGrid(bool isShowingGrid);

	bool IsRenderingTerrainWithWireframe() const;
	void SetRenderingTerrainWithWireframe(bool wireframe);
};
