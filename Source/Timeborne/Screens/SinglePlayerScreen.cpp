// Timeborne/Screens/SinglePlayerScreen.cpp

#include <Timeborne/Screens/SinglePlayerScreen.h>

#include <Timeborne/GUI/LoadSaveGUIControl.h>
#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/GUI/TimeborneGUI.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/Render/PlayerColors.h>
#include <Timeborne/Screens/InGameScreen.h>
#include <Timeborne/MainApplication.h>

constexpr uint32_t c_MaxCountAlliances = 8;

SinglePlayerScreen::SinglePlayerScreen()
	: m_LevelFilePath(std::make_unique<Nuklear_TextBox>(c_MaxLevelNameSize))
	, m_LoadGameGUIControl(std::make_unique<LoadGameGUIControl>(false, "Load game", (int)c_ButtonSize.x))
{
	m_LevelFilePath->SetText(c_DefaultLevelName);

	Reset();
}

SinglePlayerScreen::~SinglePlayerScreen()
{
}

void SinglePlayerScreen::Reset()
{
	m_NextScreen = ApplicationScreens::SinglePlayer;
	m_HasConsole = false;
	m_LogNonExistingLevel = false;
	m_LogIncompleteIndexMapping = false;
	m_LogInvalidSaveFile = false;
	mSelectedGUITab = MainTabs::NewGame;
	m_NewGameLevel.reset();
	m_NewGameData = {};
	m_LevelEditorIndices.clear();
	m_LoadGameGUIControl->Reset();
}

void SinglePlayerScreen::Enter(const ComponentRenderContext& context)
{
	context.Application->SetAllowGUIActiveTracking(false);

	Reset();
}

void SinglePlayerScreen::Exit()
{
}

void SinglePlayerScreen::DerivedInitializeMain(const ComponentInitContext& initContext)
{
}

void SinglePlayerScreen::InitializeRendering(const ComponentRenderContext& context)
{
}

void SinglePlayerScreen::DestroyMain()
{
}

void SinglePlayerScreen::DestroyRendering()
{
}

bool SinglePlayerScreen::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_EscapeECI)
	{
		m_NextScreen = ApplicationScreens::MainMenu;
		return true;
	}
	return false;
}

void SinglePlayerScreen::PreUpdate(const ComponentPreUpdateContext& context)
{
	if (m_LogNonExistingLevel)
	{
		m_LogNonExistingLevel = false;

		auto levelName = m_LevelFilePath->GetText();
		Logger::Log([&](Logger::Stream& ss) { ss << "Level '" << levelName << "' does not exist."; },
			LogSeverity::Warning, LogFlags::AddMessageBox);
	}
	else if (m_LogIncompleteIndexMapping)
	{
		m_LogIncompleteIndexMapping = false;

		Logger::Log("Incomplete player colors.", LogSeverity::Warning, LogFlags::AddMessageBox);
	}
	else if (m_LogInvalidSaveFile)
	{
		m_LogInvalidSaveFile = false;

		Logger::Log("Invalid save game file.", LogSeverity::Warning, LogFlags::AddMessageBox);
	}
}

void SinglePlayerScreen::PostUpdate(const ComponentPostUpdateContext& context)
{
}

void SinglePlayerScreen::RenderFullscreenClear(const ComponentRenderContext& context)
{
	glm::vec4 backgroundColor(0.0f, 0.3f, 1.0f, 1.0f);
	context.DeviceContext->ClearRenderTargetView(context.RTV, glm::value_ptr(backgroundColor));
}

void SinglePlayerScreen::RenderContent(const ComponentRenderContext& context)
{
}

void SinglePlayerScreen::RenderGUI(const ComponentRenderContext& context)
{
	assert(m_Application->GetPathHandler() != nullptr);

	auto ctx = (nk_context*)context.NuklearContext;

	auto mwSize = glm::vec2(context.WindowSize.x, context.WindowSize.y);

	if (Nuklear_BeginWindow(ctx, "Single player", glm::vec2(0.0f, 0.0f), glm::vec2(mwSize.x, mwSize.y)))
	{
		auto prevSelectedGUITab = mSelectedGUITab;
		Nuklear_CreateTabs(ctx, &mSelectedGUITab, { "New game", "Load game" }, c_ButtonSize.y, (int)MainTabs::COUNT + 1);
		if (TimeborneGUI_CreateExitButton(ctx)) m_NextScreen = ApplicationScreens::MainMenu;

		switch (mSelectedGUITab)
		{
			case MainTabs::NewGame:
			{
				CreateNewGameGUI(context);
				break;
			}
			case MainTabs::LoadGame:
			{
				if (prevSelectedGUITab != MainTabs::LoadGame)
				{
					m_LoadGameGUIControl->OnScreenEnter(*m_Application->GetPathHandler());
				}
				CreateLoadGameGUI(context);
				break;
			}
		}
	}
	nk_end(ctx);
}

void SinglePlayerScreen::CreateNewGameGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	TimeborneGUI_CreateLevelInput(ctx, *m_LevelFilePath);

	nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 1);
	if (nk_button_label(ctx, "Load level") != 0)
	{
		LoadNewLevel();
	}

	if (m_NewGameLevel != nullptr)
	{
		const int c_ComboBoxWidth = 300;
		const int c_ComboBoxHeight = 300;

		nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 1);
		nk_label(ctx, "", NK_LEFT);

		nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 3);
		nk_label(ctx, "Name", NK_TEXT_LEFT);
		nk_label(ctx, "Color", NK_TEXT_LEFT);
		nk_label(ctx, "Team", NK_TEXT_LEFT);
		
		uint32_t countPlayers = m_NewGameData.Players.GetCountPlayers();
		for (uint32_t i = 0; i < countPlayers; i++)
		{
			auto& playerData = m_NewGameData.Players[i];

			nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 3);

			nk_label(ctx, playerData.Name.c_str(), NK_LEFT);
			
			auto levelEditorIndex = playerData.LevelEditorIndex;
			assert(levelEditorIndex == Core::c_InvalidIndexU || levelEditorIndex < c_CountPlayerColors);

			auto playerColor = (levelEditorIndex != Core::c_InvalidIndexU)
				? c_PlayerColors[levelEditorIndex]
				: glm::vec3(0.5f, 0.5f, 0.5f);
			nk_color color = ToNKColor(glm::vec4(playerColor, 1.0f));
			if (nk_combo_begin_color(ctx, color, { (float)c_ComboBoxWidth, (float)c_ComboBoxHeight }))
			{
				int selectedColorIndex = -1;

				nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
				const char* colorNames[] = {"blue", "red", "green", "yellow", "pink", "turquoise", "orange", "dark green"};
				int c = 0;
				for (auto levelEditorIndex : m_LevelEditorIndices)
				{
					// @todo: use nk_combo_item_image_label with 8 small images, each with a single color.
					if(nk_combo_item_label(ctx, colorNames[levelEditorIndex], NK_LEFT)) selectedColorIndex = c;
					c++;
				}
				nk_combo_end(ctx);

				if (selectedColorIndex != -1)
				{
					auto iIt = m_LevelEditorIndices.begin();
					std::advance(iIt, selectedColorIndex);
					SetPlayerLevelEditorIndex(i, *iIt);
				}
			}
			
			const char* allianceStrs[] = {"A", "B", "C", "D", "E", "F", "G", "H"};
			int allianceIndex = (int)playerData.AllianceIndex;
			nk_combobox(ctx, allianceStrs, (int)std::min(countPlayers, c_MaxCountAlliances), &allianceIndex,
				(int)c_ButtonSize.y, { (float)c_ComboBoxWidth, (float)c_ComboBoxHeight });
			if (allianceIndex != (int)playerData.AllianceIndex)
			{
				m_NewGameData.Players.SetAllianceIndex(i, (uint32_t)allianceIndex);
			}
		}

		nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 1);
		if (nk_button_label(ctx, "Start game") != 0)
		{
			CreateGame();
		}
	}
}

void SinglePlayerScreen::LoadNewLevel()
{
	assert(m_Application->GetPathHandler() != nullptr);

	auto inGameScreen = GetScreen<InGameScreen>(ApplicationScreens::InGame, m_Application);
	auto levelName = m_LevelFilePath->GetText();

	if (inGameScreen->HasLevel(levelName.c_str()))
	{
		m_NewGameLevel = std::make_unique<Level>();
		m_NewGameLevel->Load(*m_Application->GetPathHandler(), levelName, false);
		
		SetupNewGameData(levelName);
	}
	else
	{
		m_LogNonExistingLevel = true;
	}
}

void SinglePlayerScreen::SetupNewGameData(const std::string& levelName)
{
	m_NewGameData = {};
	m_NewGameData.LocalPlayerIndex = 0;
	m_NewGameData.LevelName = levelName;

	m_LevelEditorIndices.clear();
	auto& gameObjects = m_NewGameLevel->GetGameObjects();
	uint32_t countGameObjects = gameObjects.GetSize();
	for (uint32_t i = 0; i < countGameObjects; i++)
	{
		auto levelEditorIndex = gameObjects[i].PlayerIndex;
		if (levelEditorIndex != Core::c_InvalidIndexU)
		{
			m_LevelEditorIndices.insert(levelEditorIndex);
		}
	}

	char buffer[16];
	for (auto levelEditorIndex : m_LevelEditorIndices)
	{
		uint32_t playerIndex = m_NewGameData.Players.AddPlayer();
		assert(levelEditorIndex < c_CountPlayerColors);
		m_NewGameData.Players.SetLevelEditorIndex(playerIndex, levelEditorIndex);

		if (playerIndex > 0)
		{
			sprintf_s(buffer, "Computer %d", playerIndex);
			m_NewGameData.Players.SetPlayerName(playerIndex, buffer);
		}
	}

	m_NewGameData.Players.SetPlayerName(0, "User");
	m_NewGameData.Players.SetPlayerType(0, PlayerType::User);
	m_NewGameData.Players.SetFreeForAll(c_MaxCountAlliances);
}

void SinglePlayerScreen::SetPlayerLevelEditorIndex(uint32_t playerIndex, uint32_t levelEditorIndex)
{
	uint32_t countPlayers = m_NewGameData.Players.GetCountPlayers();
	for (uint32_t i = 0; i < countPlayers; i++)
	{
		if (m_NewGameData.Players[i].LevelEditorIndex == levelEditorIndex)
		{
			m_NewGameData.Players.SetLevelEditorIndex(i, Core::c_InvalidIndexU);
			break;
		}
	}
	m_NewGameData.Players.SetLevelEditorIndex(playerIndex, levelEditorIndex);
}

void SinglePlayerScreen::CreateGame()
{
	auto inGameScreen = GetScreen<InGameScreen>(ApplicationScreens::InGame, m_Application);
	auto levelName = m_LevelFilePath->GetText();

	if (!inGameScreen->HasLevel(levelName.c_str()))
	{
		m_LogNonExistingLevel = true;
		return;
	}

	uint32_t countPlayers = m_NewGameData.Players.GetCountPlayers();
	for (uint32_t i = 0; i < countPlayers; i++)
	{
		if (m_NewGameData.Players[i].LevelEditorIndex == Core::c_InvalidIndexU)
		{
			m_LogIncompleteIndexMapping = true;
			return;
		}
	}

	inGameScreen->CreateNewGame(m_NewGameData);
	m_NextScreen = ApplicationScreens::InGame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SinglePlayerScreen::CreateLoadGameGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	auto action = m_LoadGameGUIControl->Render(ctx);
	if (action == LoadGameGUIControl::Action::Load) LoadSavedGame();
	else if(action == LoadGameGUIControl::Action::Exit) m_NextScreen = ApplicationScreens::MainMenu;
}

void SinglePlayerScreen::LoadSavedGame()
{
	auto inGameScreen = GetScreen<InGameScreen>(ApplicationScreens::InGame, m_Application);
	auto saveGameFile = m_LoadGameGUIControl->GetFileName();

	if (inGameScreen->IsSaveFileValid(saveGameFile))
	{
		inGameScreen->SetupLoadGame(saveGameFile);

		m_NextScreen = ApplicationScreens::InGame;
	}
	else
	{
		m_LogInvalidSaveFile = true;
	}
}
