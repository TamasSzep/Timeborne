// Timeborne/Screens/InGameScreen.cpp

#include <Timeborne/Screens/InGameScreen.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/GUI/LoadSaveGUIControl.h>
#include <Timeborne/InGame/InGame.h>
#include <Timeborne/Misc/ScreenResolution.h>
#include <Timeborne/MainApplication.h>

constexpr float c_MainDialogHeightRatio = 0.21f;

InGameScreen::InGameScreen()
	: m_InGame(std::make_unique<InGame>())
	, m_LoadGameGUIControl(std::make_unique<LoadGameGUIControl>(true))
	, m_SaveGameGUIControl(std::make_unique<SaveGameGUIControl>())
{
	Reset();
}

InGameScreen::~InGameScreen()
{
}

void InGameScreen::Reset()
{
	m_NextScreen = ApplicationScreens::InGame;
	m_HasConsole = false;

	ResetLoadSaveSubDialogState();

	m_LoadError.clear();
	m_SaveTime = {};

	m_InMainDialog = false;
	m_WasPausedBeforeMainDialog = false;
}

void InGameScreen::DerivedInitializeMain(const ComponentInitContext& context)
{
	m_InGame->InitializeMain(context);
}

void InGameScreen::InitializeRendering(const ComponentRenderContext& context)
{
	m_InGame->InitializeRendering(context);
}

void InGameScreen::DestroyMain()
{
	m_InGame->DestroyMain();
}

void InGameScreen::DestroyRendering()
{
	m_InGame->DestroyRendering();
}

void InGameScreen::ResetLoadSaveSubDialogState()
{
	m_SubDialog = SubDialog::None;
	m_LoadGameGUIControl->Reset();
	m_SaveGameGUIControl->Reset();
}

void InGameScreen::Enter(const ComponentRenderContext& context)
{
	assert(m_LoadSource != LoadSource::SaveFileInGame);

	m_Application->SetAllowGUIActiveTracking(false);

	m_HasExternalLoadError = false;

	Reset();

	if (m_LoadSource == LoadSource::Level)
	{
		m_InGame->LoadGameFromLevel(context);
	}
	else
	{
		LoadGameFromSaveFile(context, true);
	}
}

void InGameScreen::Exit()
{
}

bool InGameScreen::HasLevel(const char* levelName) const
{
	assert(m_Application != nullptr && m_Application->GetPathHandler() != nullptr);
	return m_InGame->HasLevel(levelName, *m_Application->GetPathHandler());
}

void InGameScreen::CreateNewGame(const GameCreationData& data)
{
	m_InGame->CreateNewGame(data);

	m_LoadSource = LoadSource::Level;
}

bool InGameScreen::IsSaveFileValid(const char* saveFileName) const
{
	assert(m_Application != nullptr && m_Application->GetPathHandler() != nullptr);
	return m_InGame->IsSaveFileValid(saveFileName, *m_Application->GetPathHandler());
}

void InGameScreen::SetupLoadGame(const char* saveFileName)
{
	m_LoadSource = LoadSource::SaveFile;
	m_SaveFileName = (saveFileName != nullptr) ? saveFileName : "";
}

void InGameScreen::LoadGameFromSaveFile(const ComponentRenderContext& context, bool clearState)
{
	m_LoadError.clear();

	if (m_SaveFileName.empty())
	{
		m_LoadError = "No save file has been selected.";
	}

	if (m_LoadError.empty())
	{
		m_InGame->LoadGameFromSaveFile(context, clearState, m_SaveFileName.c_str(), m_LoadError);
	}

	if (m_LoadError.empty())
	{
		Reset();
		m_InputFreezeTime = std::chrono::steady_clock::now();
	}
	else if (m_LoadSource == LoadSource::SaveFile)
	{
		// Storing the external load error event explicitly, because the load error string will be cleared after showing it.
		m_HasExternalLoadError = true;
	}
}

bool InGameScreen::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	if (m_HasExternalLoadError) return false;

	if (_event->ClassId == m_EscapeECI)
	{
		if (m_SubDialog != SubDialog::None) ResetLoadSaveSubDialogState();
		else SetInMainDialog(!m_InMainDialog);
		return true;
	}

	constexpr unsigned c_FreezeTimeMs = 100;
	if (m_InMainDialog || (m_InputFreezeTime != decltype(m_InputFreezeTime)()
		&& std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - m_InputFreezeTime).count() <= c_FreezeTimeMs))
	{
		return false;
	}

	return m_InGame->HandleEvent(_event);
}

void InGameScreen::SetInMainDialog(bool inMainDialog)
{
	m_InMainDialog = inMainDialog;

	if (!m_InGame->IsMultiplayerGame())
	{
		bool paused = m_InGame->IsPaused();

		if (inMainDialog)
		{
			m_WasPausedBeforeMainDialog = paused;
			if (!paused) m_InGame->TriggerPauseSwitch();
		}
		else if (!m_WasPausedBeforeMainDialog)
		{
			assert(paused);
			m_InGame->TriggerPauseSwitch();
		}
	}

	m_InGame->SetControlsActive(!inMainDialog);

	if (inMainDialog)
	{
		ResetLoadSaveSubDialogState();
		m_SaveTime = {};
	}
}

void InGameScreen::PreUpdate(const ComponentPreUpdateContext& context)
{
	if (!m_HasExternalLoadError)
	{
		m_InGame->PreUpdate(context);
	}

	HandleLoadError();
}

void InGameScreen::PostUpdate(const ComponentPostUpdateContext& context)
{
	if (m_HasExternalLoadError) return;

	m_InGame->PostUpdate(context);
}

void InGameScreen::RenderFullscreenClear(const ComponentRenderContext& context)
{
	m_InGame->RenderFullscreenClear(context);
}

void InGameScreen::RenderContent(const ComponentRenderContext& context)
{
	if (m_HasExternalLoadError) return;

	m_InGame->RenderContent(context);
}

void InGameScreen::RenderGUI(const ComponentRenderContext& context)
{
	if (m_HasExternalLoadError) return;

	m_InGame->RenderGUI(context);

	CreateMainDialogGUI(context);
}

void InGameScreen::CreateMainDialogGUI(const ComponentRenderContext& context)
{
	if (!m_InMainDialog) return;

	assert(m_Application->GetPathHandler() != nullptr);

	auto ctx = (nk_context*)context.NuklearContext;

	auto mwStart = RatioToPixels(glm::vec2(0.4f, 0.4f), context.WindowSize);
	auto mwSize = RatioToPixels(glm::vec2(0.2f, c_MainDialogHeightRatio), context.WindowSize);

	if (Nuklear_BeginWindow(ctx, "Menu", glm::vec2(mwStart.x, mwStart.y), glm::vec2(mwSize.x, mwSize.y)))
	{
		nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
		if (nk_button_label(ctx, "Resume game")) SetInMainDialog(false);

		if (!m_InGame->IsMultiplayerGame())
		{
			nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
			if (nk_button_label(ctx, "Load game"))
			{
				m_LoadGameGUIControl->OnScreenEnter(*m_Application->GetPathHandler());
				m_SubDialog = SubDialog::Load;
			}
			nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
			if (nk_button_label(ctx, "Save game"))
			{
				m_SaveGameGUIControl->OnScreenEnter(*m_Application->GetPathHandler());
				m_SubDialog = SubDialog::Save;
			}
		}

		nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
		if (nk_button_label(ctx, "Exit")) m_NextScreen = ApplicationScreens::MainMenu;

		CreateGameSavedLabel(context);
	}
	nk_end(ctx);

	if (m_SubDialog == SubDialog::Load)
	{
		CreateLoadSubDialog(context);
	}
	else if (m_SubDialog == SubDialog::Save)
	{
		CreateSaveSubDialog(context);
	}
}

void InGameScreen::CreateLoadSubDialog(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	auto mwStart = RatioToPixels(glm::vec2(0.6f, 0.4f), context.WindowSize);
	auto mwSize = RatioToPixels(glm::vec2(0.2f, c_MainDialogHeightRatio), context.WindowSize);

	if (Nuklear_BeginWindow(ctx, "Load", glm::vec2(mwStart.x, mwStart.y), glm::vec2(mwSize.x, mwSize.y)))
	{
		auto action = m_LoadGameGUIControl->Render(ctx);

		if (action == LoadGameGUIControl::Action::Load)
		{
			m_LoadSource = LoadSource::SaveFileInGame;
			auto saveFileName = m_LoadGameGUIControl->GetFileName();
			m_SaveFileName = (saveFileName != nullptr) ? saveFileName : "";
			LoadGameFromSaveFile(context, false);
		}
		else if (action == LoadGameGUIControl::Action::Exit) ResetLoadSaveSubDialogState();
	}
	nk_end(ctx);
}

void InGameScreen::CreateSaveSubDialog(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	auto mwStart = RatioToPixels(glm::vec2(0.6f, 0.4f), context.WindowSize);
	auto mwSize = RatioToPixels(glm::vec2(0.2f, c_MainDialogHeightRatio), context.WindowSize);

	if (Nuklear_BeginWindow(ctx, "Load", glm::vec2(mwStart.x, mwStart.y), glm::vec2(mwSize.x, mwSize.y)))
	{
		auto action = m_SaveGameGUIControl->Render(ctx);

		if (action == SaveGameGUIControl::Action::Save)
		{
			SaveGame();
			ResetLoadSaveSubDialogState();
		}
		else if (action == SaveGameGUIControl::Action::Exit) ResetLoadSaveSubDialogState();
	}
	nk_end(ctx);
}

void InGameScreen::CreateGameSavedLabel(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	if (m_SaveTime != decltype(m_SaveTime)()
		&& std::chrono::steady_clock::now() - m_SaveTime <= std::chrono::milliseconds(1500))
	{
		nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
		nk_label(ctx, "The game has been saved.", NK_TEXT_CENTERED);
	}
}

void InGameScreen::HandleLoadError()
{
	if (!m_LoadError.empty())
	{
		Logger::Log([&](Logger::Stream& ss) { ss << m_LoadError; }, LogSeverity::Warning, LogFlags::AddMessageBox);
		m_LoadError.clear();

		if (m_LoadSource == LoadSource::SaveFile)
		{
			assert(m_HasExternalLoadError);
			m_NextScreen = m_PreviousScreen;
		}
	}
}

void InGameScreen::SaveGame()
{
	assert(m_Application != nullptr && m_Application->GetPathHandler() != nullptr);

	m_InGame->SaveGame(m_SaveGameGUIControl->GetFileName().c_str(), *m_Application->GetPathHandler());

	m_SaveTime = std::chrono::steady_clock::now();
}
