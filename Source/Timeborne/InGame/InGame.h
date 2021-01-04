// Timeborne/InGame/InGame.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>
#include <Timeborne/GameCreation/GameCreationData.h>

#include <chrono>
#include <memory>
#include <vector>

class ClientGameState;
class CommandList;
class GameCamera;
class InGameComponent;
class InGameController;
class InGameModel;
class InGameView;
class Level;
class Nuklear_TextBox;
class LoadGameGUIControl;
class SaveGameGUIControl;

class InGame : public ApplicationScreen
{
	const EngineBuildingBlocks::PathHandler& m_PathHandler;

private: // Level data.

	std::unique_ptr<Level> m_Level;

	void LoadLevel();

private: // Components.

	std::unique_ptr<InGameView> m_View;
	std::unique_ptr<InGameModel> m_Model;
	std::unique_ptr<InGameController> m_Controller;

private:

	std::unique_ptr<CommandList> m_CommandList;

private: // Input.

	unsigned m_PauseECI;

protected:

	void DerivedInitializeMain(const ComponentInitContext& context);

public:

	explicit InGame(const EngineBuildingBlocks::PathHandler& pathHandler);
	~InGame() override;

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

private: // Client-server state syncing.

	std::unique_ptr<ClientGameState> m_ClientGameState;

	void SyncWithServerData();

public: // Create.

	bool HasLevel(const char* levelName) const;
	void CreateNewGame(const GameCreationData& data);
	void RemapPlayerIndices();

	enum class LoadSource
	{
		Level, SaveFile, SaveFileInGame
	};
	LoadSource m_LoadSource;

public: // Load, save.

	bool IsSaveFileValid(const char* saveFileName) const;
	void LoadGame(const char* saveFileName);

private:

	void InitializeOnLoading(const ComponentRenderContext& context);
	std::string GetSavePath(const char* fileName) const;
	void LoadGame(const ComponentRenderContext& context);
	void HandleLoadError();
	void SaveGame();
	void ResetLoadSaveSubDialogState();

	enum class SubDialog { None, Load, Save } m_SubDialog = SubDialog::None;

	std::unique_ptr<LoadGameGUIControl> m_LoadGameGUIControl;
	std::unique_ptr<SaveGameGUIControl> m_SaveGameGUIControl;

	std::chrono::steady_clock::time_point m_SaveTime;

	std::string m_SaveFileName;

	bool m_HasExternalLoadError = false;

	void CreateLoadSubDialog(const ComponentRenderContext& context);
	void CreateSaveSubDialog(const ComponentRenderContext& context);
	void CreateGameSavedLabel(const ComponentRenderContext& context);

	std::string m_LoadError;

private: // Rendering.

	std::unique_ptr<EngineBuildingBlocks::SceneNodeHandler> m_CameraSceneNodeHandler;
	std::unique_ptr<GameCamera> m_Camera;

	void CreateCamera(const ComponentRenderContext& context, bool isLoadingFromSaveFile);

private: // Game update.

	using UpdateClock = std::chrono::steady_clock;

	static constexpr unsigned c_UpdateIntervalInMillis = 10; // Updating 100 times per second.

	UpdateClock::time_point mLastTime;
	UpdateClock::time_point m_NextUpdateTime;
	int m_CountPauseSwitches = 0;
	bool m_Paused = false;

	void ResetGameUpdate();
	void DoGameUpdate();
	void DirectUpdate(double dt);
	void Tick();

private: // GUI.

	bool m_InMainDialog = false;
	bool m_WasPausedBeforeMainDialog = false;

	std::chrono::steady_clock::time_point m_InputFreezeTime;

	void SetInMainDialog(bool inMainDialog);

	void CreateMainDialogGUI(const ComponentRenderContext& context);
};