// Timeborne/LevelEditor/Terrain/TerrainSpadeTool.h

#pragma once

#include <Timeborne/InGame/Model/Terrain/FieldHeightQuadTree.h>
#include <Timeborne/LevelEditor/Terrain/TerrainEditing.h>
#include <Timeborne/LevelEditor/LevelEditorComponent.h>

#include <memory>

class TerrainLevelEditorView;
class Nuklear_OnOffButton;

class TerrainSpadeTool : public LevelEditorComponent
{
	TerrainLevelEditorView& m_TerrainLevelEditorView;

	enum class BlockType { Square, Circle } m_BlockType = BlockType::Square;

	DragDifference m_DragDiff;
	TerrainBlockIntersection m_Intersection;
	Core::SimpleTypeVectorU<glm::ivec2> m_EditedFieldIndices;

	int m_BlockSize;

	std::unique_ptr<Nuklear_OnOffButton> m_OnOffButton;

public:

	explicit TerrainSpadeTool(TerrainLevelEditorView& terrainLevelEditorView);
	~TerrainSpadeTool() override;

	void SetActive(bool active) override;

	bool HandleEvent(const EngineBuildingBlocks::Event* _event) override;
	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;
};
