// Timeborne/LevelEditor/GameObjects/ManageGameObjectsTool.h

#pragma once

#include <Timeborne/LevelEditor/LevelEditorComponent.h>
#include <Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorModel.h>

class Nuklear_OnOffButton;
class TerrainLevelEditorView;

class ManageGameObjectsTool : public LevelEditorComponent
{
	GameObjectLevelEditorModel& m_GameObjectModel;
	TerrainLevelEditorView& m_TerrainLevelEditorView;

	enum class SelectionState { SelectingStart, SelectingEnd, Selected }
		m_SelectionState = SelectionState::SelectingStart;

	glm::ivec2 m_TerrainStart = glm::ivec2(-1);
	glm::ivec2 m_TerrainEnd = glm::ivec2(-1);
	bool m_HasTerrainIntersection = false;
	Core::SimpleTypeVectorU<GameObjectId> m_SelectedObjectIds;

	void Reset();
	void ResetSelectionState();

	void SetSelectedObjectAlpha(float alpha);
	void DeleteSelectedObjects();
	void CreateSelection();
	void ExitSelection();

private: // GUI.

	std::unique_ptr<Nuklear_OnOffButton> m_OnOffButton;

private: // Input.

	unsigned m_ActionECI = Core::c_InvalidIndexU;
	unsigned m_MouseMoveEventECI = Core::c_InvalidIndexU;
	unsigned m_DeleteECI = Core::c_InvalidIndexU;

	bool m_DeleteActive = false;
	std::vector<bool> m_ActionEvents;
	glm::vec2 m_CursorPositionInScreen;

public:

	ManageGameObjectsTool(GameObjectLevelEditorModel& model,
		TerrainLevelEditorView& terrainLevelEditorView);
	~ManageGameObjectsTool() override;

public: // LevelEditorComponent IF.

	void SetActive(bool active) override;

	bool HandleEvent(const EngineBuildingBlocks::Event* _event);
	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;

protected:

	void DerivedInitializeMain(const ComponentInitContext& context) override;
};