// Timeborne/Screens/SinglePlayerScreen.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

#include <Timeborne/GameCreation/GameCreationData.h>

#include <memory>

class Level;
class Nuklear_TextBox;
class LoadGameGUIControl;

class SinglePlayerScreen : public ApplicationScreen
{
public:

	SinglePlayerScreen();

private:

	void Reset();

public: // ApplicationComponent IF.

	~SinglePlayerScreen() override;

	void InitializeRendering(const ComponentRenderContext& context) override;
	void DestroyMain() override;
	void DestroyRendering() override;
	bool HandleEvent(const EngineBuildingBlocks::Event* _event) override;
	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void PostUpdate(const ComponentPostUpdateContext& context) override;
	void RenderFullscreenClear(const ComponentRenderContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;

protected:

	void DerivedInitializeMain(const ComponentInitContext& initContext);

public: // ApplicationScreen IF.

	void Enter(const ComponentRenderContext& context) override;
	void Exit() override;

private: // GUI.

	enum class MainTabs { NewGame, LoadGame, COUNT };
	MainTabs mSelectedGUITab = MainTabs::NewGame;

private: // New game GUI.

	std::unique_ptr<Level> m_NewGameLevel;
	GameCreationData m_NewGameData;

	std::set<uint32_t> m_LevelEditorIndices;

	std::unique_ptr<Nuklear_TextBox> m_LevelFilePath;

	bool m_LogNonExistingLevel = false;
	bool m_LogIncompleteIndexMapping = false;

	void CreateNewGameGUI(const ComponentRenderContext& context);
	void LoadNewLevel();
	void SetupNewGameData(const std::string& levelName);
	void SetPlayerLevelEditorIndex(uint32_t playerIndex, uint32_t levelEditorIndex);
	void CreateGame();

private: // Load game GUI.

	std::unique_ptr<LoadGameGUIControl> m_LoadGameGUIControl;

	bool m_LogInvalidSaveFile = false;

	void CreateLoadGameGUI(const ComponentRenderContext& context);
	void LoadSavedGame();
};