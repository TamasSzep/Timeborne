// Timeborne/LevelEditor/LevelEditor.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>
#include <Timeborne/LevelEditor/LevelEditorComponent.h>

#include <DirectX11Render/Resources/ConstantBuffer.h>

#include <memory>

struct nk_context;

class GameObjectLevelEditorModel;
class GameObjectLevelEditorView;
class Level;
class Nuklear_TextBox;
class TerrainCopyTool;
class TerrainLevelEditorView;
class TerrainSpadeTool;

class LevelEditor : public ApplicationScreen
{
	std::unique_ptr<Level> m_Level;

	void OnLevelLoaded(LevelEditorComponent::LevelDirtyFlags dirtyFlags);

private: // Load level.

	bool m_IsLoadingLevel = false;
	bool m_IsForcingLoadedLevelRecomputations = false;
	std::unique_ptr<Nuklear_TextBox> m_LevelLoadFilePath;

	void LoadLevelMetadata(const EngineBuildingBlocks::PathHandler& pathHandler, const std::string& levelName);

private: // Save level.

	bool m_IsSavingLevel = false;
	std::unique_ptr<Nuklear_TextBox> m_LevelSaveFilePath;

	void SaveLevelMetadata(const EngineBuildingBlocks::PathHandler& pathHandler, const std::string& levelName) const;

private: // Rendering.

	std::unique_ptr<TerrainLevelEditorView> m_TerrainLevelEditorView;
	std::unique_ptr<GameObjectLevelEditorModel> m_GameObjectLevelEditorModel;
	std::unique_ptr<GameObjectLevelEditorView> m_GameObjectLevelEditorView;

	std::vector<LevelEditorComponent*> m_Components;

	std::unique_ptr<EngineBuildingBlocks::SceneNodeHandler> m_CameraSceneNodeHandler;
	std::unique_ptr<EngineBuildingBlocks::Graphics::FreeCamera> m_Camera;

	DirectX11Render::ConstantBuffer m_LightingCB;
	DirectX11Render::ConstantBuffer m_RenderPassCB;

	void SetLightingCBData(const ComponentRenderContext& context);
	void UpdateRenderPassCBData();

private: // Tools.

	unsigned m_ActiveToolIndex = Core::c_InvalidIndexU;
	std::vector<std::unique_ptr<LevelEditorComponent>> m_Tools;

	LevelEditorComponent* GetActiveTool();
	void RenderToolGUI(const ComponentRenderContext& context,
		unsigned toolIndex);

	std::string m_ActiveToolInfo;

	unsigned m_TerrainSpadeToolIndex = Core::c_InvalidIndexU;
	unsigned m_TerrainCopyToolIndex = Core::c_InvalidIndexU;

	unsigned m_NewGameObjectToolIndex = Core::c_InvalidIndexU;
	unsigned m_ManageGameObjectsToolIndex = Core::c_InvalidIndexU;

	const char* GetCurrentToolName();
	void UpdateActiveToolInfoString();
	void HandleToolActivityChange(unsigned toolIndex);
	void DoAutoGrabTool();

public: // Input.

	unsigned m_AutoGrabECI = Core::c_InvalidIndexU;

public:

	void OnTerrainHeightsDirty(const glm::ivec2& changeStart, const glm::ivec2& changeEnd);

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

	void CreateActiveToolGUI(const ComponentRenderContext& context);

private: // New level GUI.

	std::unique_ptr<Nuklear_TextBox> m_NewLevelName;
	glm::ivec2 m_NewLevelSizeExp = glm::ivec2(10);

	void CreateNewLevelGUI(const ComponentRenderContext& context);

protected:

	void DerivedInitializeMain(const ComponentInitContext& context);

public:

	LevelEditor();
	~LevelEditor() override;

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
};
