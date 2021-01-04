// Timeborne/LevelEditor/Terrain/TerrainCopyTool.h

#pragma once

#include <Timeborne/LevelEditor/Terrain/TerrainEditing.h>
#include <Timeborne/LevelEditor/LevelEditorComponent.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <memory>

class TerrainLevelEditorView;
class Nuklear_OnOffButton;

class TerrainCopyTool : public LevelEditorComponent
{
	TerrainLevelEditorView& m_TerrainLevelEditorView;

	DragDifference m_DragDiff;

	bool m_HasSourceIntersection, m_HasTargetIntersection;
	glm::ivec2 m_Source1, m_Source2, m_TargetStart, m_TargetEnd;

	int m_BlockSize;

	std::unique_ptr<Nuklear_OnOffButton> m_OnOffButton;

	enum class State
	{
		Source1,
		Source2,
		Target,
		End,
	} m_State = State::Source1;

	Core::SimpleTypeVectorU<glm::vec4> m_CopyBuffer;

	void ResetTarget();
	void CopyTerrain();

public:
	explicit TerrainCopyTool(TerrainLevelEditorView& terrainLevelEditorView);
	~TerrainCopyTool() override;

	void SetActive(bool active) override;

	bool HandleEvent(const EngineBuildingBlocks::Event* _event) override;
	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;
};