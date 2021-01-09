// Timeborne/LevelEditor/LevelEditor.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>
#include <Timeborne/LevelEditor/LevelEditorComponent.h>

#include <DirectX11Render/Resources/ConstantBuffer.h>

#include <memory>

class GameObjectLevelEditorModel;
class GameObjectLevelEditorView;
class Level;
class MainApplication;
class TerrainLevelEditorView;

class LevelEditor
{
	std::unique_ptr<Level> m_Level;

	void OnLevelLoaded(LevelEditorComponent::LevelDirtyFlags dirtyFlags);

private: // Load level.

	void LoadLevelMetadata(const EngineBuildingBlocks::PathHandler& pathHandler, const std::string& levelName);

private: // Save level.

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

public: // Tools.

	enum class ToolType {TerrainSpade, TerrainCopy, NewGameObject, ManageGameObjects};

	const char* GetCurrentToolName();
	const char* GetActiveToolInfoString();

	ToolType GetActiveToolType() const;
	void SetToolActive(ToolType toolType, bool active);

	void RenderToolGUI(const ComponentRenderContext& context, ToolType toolType);

private:

	unsigned m_ActiveToolIndex = Core::c_InvalidIndexU;
	std::vector<std::unique_ptr<LevelEditorComponent>> m_Tools;

	LevelEditorComponent* GetActiveTool();

	void HandleToolActivityChange(unsigned toolIndex);

public:

	void OnTerrainHeightsDirty(const glm::ivec2& changeStart, const glm::ivec2& changeEnd);

public:

	LevelEditor();
	~LevelEditor();

	void InitializeMain(const ComponentInitContext& context);
	void InitializeRendering(const ComponentRenderContext& context);
	void DestroyMain();
	void DestroyRendering();
	bool HandleEvent(const EngineBuildingBlocks::Event* _event);
	void PreUpdate(const ComponentPreUpdateContext& context);
	void PostUpdate(const ComponentPostUpdateContext& context);
	void RenderFullscreenClear(const ComponentRenderContext& context);
	void RenderContent(const ComponentRenderContext& context);
	void RenderGUI(const ComponentRenderContext& context);

public:

	bool TryEscapeActiveTool();

	bool IsShowingTerrainGrid() const;
	void SetShowingTerrainGrid(bool value);

	bool IsRenderingTerrainWithWireframe() const;
	void SetRenderingTerrainWithWireframe(bool value);

	void CreateNewLevel(MainApplication* application,
		const char* levelName, const glm::uvec2& size);
	void LoadLevel(const ComponentPostUpdateContext& context,
		const std::string& levelName, bool isForcingLoadedLevelRecomputations);
	void SaveLevel(const ComponentPostUpdateContext& context,
		const std::string& levelName);
};
