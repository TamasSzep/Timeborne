// Timeborne/Screens/InGameScreen.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

#include <Core/Constants.h>

#include <memory>

struct GameCreationData;
class InGame;
class InGameStatistics;
class LoadGameGUIControl;
class SaveGameGUIControl;

class InGameScreen : public ApplicationScreen
{
	std::unique_ptr<InGame> m_InGame;

	void Reset();

public:

	InGameScreen();
	~InGameScreen() override;

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

protected:

	void DerivedInitializeMain(const ComponentInitContext& context) override;

public: // Create.

	bool HasLevel(const char* levelName) const;
	void CreateNewGame(const GameCreationData& data);

public: // Load, save.

	bool IsSaveFileValid(const char* saveFileName) const;
	void SetupLoadGame(const char* saveFileName);

public: // Game result.

	bool IsGameEnded() const;
	const GameCreationData& GetGameCreationData() const;
	const InGameStatistics& GetStatistics() const;

private:

	void LoadGameFromSaveFile(const ComponentRenderContext& context, bool clearState);
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

	enum class LoadSource
	{
		Level, SaveFile, SaveFileInGame
	};
	LoadSource m_LoadSource = (LoadSource)Core::c_InvalidIndexU;

private: // GUI.

	bool m_InEndDialog = false;
	bool m_EndDialogHasBeenShown = false;
	bool m_InMainDialog = false;
	bool m_WasPausedBeforeMainDialog = false;

	std::chrono::steady_clock::time_point m_InputFreezeTime;

	void SetInMainDialog(bool inMainDialog);

	void CreateMainDialogGUI(const ComponentRenderContext& context);
	void CreateEndDialog(const ComponentRenderContext& context);
};
