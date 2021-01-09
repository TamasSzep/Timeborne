// Timeborne/Screens/LevelEditorScreen.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

#include <Core/Constants.h>

#include <memory>

class LevelEditor;
class Nuklear_TextBox;

class LevelEditorScreen : public ApplicationScreen
{
	std::unique_ptr<LevelEditor> m_LevelEditor;

protected:

	void DerivedInitializeMain(const ComponentInitContext& context) override;

public:

	LevelEditorScreen();
	~LevelEditorScreen() override;

	void InitializeRendering(const ComponentRenderContext& context) override;
	void DestroyMain() override;
	void DestroyRendering() override;
	bool HandleEvent(const EngineBuildingBlocks::Event* _event) override;
	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void PostUpdate(const ComponentPostUpdateContext& context) override;
	void RenderFullscreenClear(const ComponentRenderContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;

	void Enter(const ComponentRenderContext& context) override;
	void Exit() override;

private:

	void Reset();

public: // Input.

	unsigned m_AutoGrabECI = Core::c_InvalidIndexU;

private: // GUI.

	void CreateMainGUI(const ComponentRenderContext& context);
	void CreateFileGUI(const ComponentRenderContext& context);
	void CreateRenderGUI(const ComponentRenderContext& context);

	enum class MainTabs { File, Render, Terrain, GameObjects, COUNT };
	MainTabs m_SelectedGUITab = MainTabs::File;

	enum class FileTabs { NewLevel, LoadLevel, SaveLevel, COUNT };
	FileTabs m_SelectedFileTab = FileTabs::LoadLevel;

	enum class TerrainTabs { HeightTools, COUNT };
	TerrainTabs m_SelectedTerrainTab = TerrainTabs::HeightTools;

	enum class GameObjectTabs { NewObject, ManageObjects, COUNT };
	GameObjectTabs m_SelectedGameObjectsTab = GameObjectTabs::NewObject;

private: // New level GUI.

	std::unique_ptr<Nuklear_TextBox> m_NewLevelName;
	glm::ivec2 m_NewLevelSizeExp = glm::ivec2(10);

	void CreateNewLevelGUI(const ComponentRenderContext& context);

private: // Load level.

	bool m_IsLoadingLevel = false;
	bool m_IsForcingLoadedLevelRecomputations = false;
	std::unique_ptr<Nuklear_TextBox> m_LevelLoadFilePath;

private: // Save level.

	bool m_IsSavingLevel = false;
	std::unique_ptr<Nuklear_TextBox> m_LevelSaveFilePath;

private: // Tools.

	void DoAutoGrabTool();
	void CreateActiveToolGUI(const ComponentRenderContext& context);
};