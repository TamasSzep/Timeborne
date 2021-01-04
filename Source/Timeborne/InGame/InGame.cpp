// Timeborne/InGame/InGame.cpp

#include <Timeborne/InGame/InGame.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/GUI/LoadSaveGUIControl.h>
#include <Timeborne/InGame/Controller/CommandList.h>
#include <Timeborne/InGame/Controller/InGameController.h>
#include <Timeborne/InGame/GameCamera/GameCamera.h>
#include <Timeborne/InGame/GameState/ClientGameState.h>
#include <Timeborne/InGame/Model/InGameModel.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/InGame/Model/TickContext.h>
#include <Timeborne/InGame/View/InGameView.h>
#include <Timeborne/Misc/ScreenResolution.h>
#include <Timeborne/MainApplication.h>

#include <Core/System/Filesystem.h>
#include <Core/System/SimpleIO.h>
#include <EngineBuildingBlocks/Input/DefaultInputBinder.h>
#include <EngineBuildingBlocks/SceneNode.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Input;
using namespace EngineBuildingBlocks::Math;

constexpr float c_MainDialogHeightRatio = 0.21f;

InGame::InGame(const EngineBuildingBlocks::PathHandler& pathHandler)
	: m_PathHandler(pathHandler)
	, m_LoadGameGUIControl(std::make_unique<LoadGameGUIControl>(true))
	, m_SaveGameGUIControl(std::make_unique<SaveGameGUIControl>())
{
	Reset();
}

InGame::~InGame()
{
}

void InGame::Reset()
{
	m_Model.reset();
	m_Controller.reset();

	m_NextScreen = ApplicationScreens::InGame;
	m_HasConsole = false;

	m_Level.reset();
	m_Camera.reset();
	m_CommandList.reset();

	ResetLoadSaveSubDialogState();

	m_LoadError.clear();
	m_SaveTime = {};

	ResetGameUpdate();

	m_InMainDialog = false;
	m_WasPausedBeforeMainDialog = false;
}

bool InGame::HasLevel(const char* levelName) const
{
	return Core::FileExists(Level::GetPath(m_PathHandler, levelName));
}

void InGame::LoadLevel()
{
	auto levelName = m_ClientGameState->GetGameCreationData().LevelName;
	bool levelExists = HasLevel(levelName.c_str());
	if (levelExists)
	{
		m_Level = std::make_unique<Level>();
		m_Level->Load(m_PathHandler, levelName, false);

		Logger::Log([&](Logger::Stream& ss) { ss << "Level '" << levelName << "' has been loaded in InGame."; },
			LogSeverity::Info);
	}

	assert(levelExists);
}

void InGame::CreateCamera(const ComponentRenderContext& context, bool isLoadingFromSaveFile)
{
	auto eventManager = m_Application->GetEventManager();
	auto keyHandler = m_Application->GetKeyHandler();
	auto mouseHandler = m_Application->GetMouseHandler();

	m_CameraSceneNodeHandler = std::make_unique<EngineBuildingBlocks::SceneNodeHandler>();
	m_Camera = std::make_unique<GameCamera>(m_ClientGameState->GetLocalGameState().GameCameraState,
		m_CameraSceneNodeHandler.get(), eventManager,
		keyHandler, mouseHandler, context.ContentSize, isLoadingFromSaveFile);
}

void InGame::InitializeOnLoading(const ComponentRenderContext& context)
{
	bool isLoadingFromSaveFile = m_LoadSource != LoadSource::Level;
	bool isRemappingPlayerIndices = m_LoadSource == LoadSource::Level;

	Reset();

	LoadLevel();

	if (isRemappingPlayerIndices)
	{
		RemapPlayerIndices();
	}

	CreateCamera(context, isLoadingFromSaveFile);

	// Creating the command list.
	m_CommandList = std::make_unique<CommandList>();

	assert(m_Level != nullptr && m_ClientGameState != nullptr && m_Camera != nullptr && m_CommandList != nullptr
		&& context.Application != nullptr);

	// The view must be loaded before the model, because the loading clears the view and sets up the connections,
	// so when the level is loaded in the model, the view can listen the model's events.
	m_View->Load(*m_Level, *m_ClientGameState, *m_Camera, context);

	// Creating the model.
	m_Model = std::make_unique<InGameModel>(*m_Level, *m_ClientGameState, *m_CommandList, isLoadingFromSaveFile);

	// Creating the controller.
	m_Controller = std::make_unique<InGameController>(*m_Level, *m_ClientGameState, *m_CommandList,
		m_View->GetGameObjectVisibilityProvider(), *m_Camera, *context.Application, isLoadingFromSaveFile);
}

void InGame::Enter(const ComponentRenderContext& context)
{
	context.Application->SetAllowGUIActiveTracking(false);

	assert(m_LoadSource != LoadSource::SaveFileInGame);

	m_HasExternalLoadError = false;

	if (m_LoadSource == LoadSource::Level)
	{
		InitializeOnLoading(context);
	}
	else
	{
		LoadGame(context);
	}
}

void InGame::Exit()
{
}

void InGame::DerivedInitializeMain(const ComponentInitContext& context)
{
	auto keyHandler = context.Application->GetKeyHandler();

	// Registering state key events.
	m_PauseECI = keyHandler->RegisterStateKeyEventListener("InGame.Pause", m_Application);

	// Binding the key events.
	keyHandler->BindEventToKey(m_PauseECI, Keys::Pause);

	// Creating the view.
	m_View = std::make_unique<InGameView>();
}

void InGame::InitializeRendering(const ComponentRenderContext& context)
{
	m_View->InitializeRendering(context);
}

void InGame::DestroyMain()
{
}

void InGame::DestroyRendering()
{
	m_View->DestroyRendering();
}

bool InGame::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	bool handled = false;
	auto eci = _event->ClassId;

	if (m_HasExternalLoadError) return false;

	if (eci == m_EscapeECI)
	{
		if (m_SubDialog != SubDialog::None) ResetLoadSaveSubDialogState();
		else SetInMainDialog(!m_InMainDialog);
		handled = true;
	}

	constexpr unsigned c_FreezeTimeMs = 100;
	if (m_InMainDialog || (m_InputFreezeTime != decltype(m_InputFreezeTime)()
		&& std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - m_InputFreezeTime).count() <= c_FreezeTimeMs))
	{
		return handled;
	}
	
	if (eci == m_PauseECI)
	{
		m_CountPauseSwitches++;
		handled = true;
	}

	if (m_Paused) return handled;

	if (!handled)
	{
		if (m_Controller->HandleEvent(_event))
		{
			handled = true;
		}
	}
	return handled;
}

void InGame::ResetLoadSaveSubDialogState()
{
	m_SubDialog = SubDialog::None;
	m_LoadGameGUIControl->Reset();
	m_SaveGameGUIControl->Reset();
}

void InGame::SetInMainDialog(bool inMainDialog)
{
	// Not setting 'm_Paused' directly, generating the input to set it in order to reuse the same pausing mechanism.

	m_InMainDialog = inMainDialog;

	if (!m_ClientGameState->GetGameCreationData().Players.IsMultiplayerGame())
	{
		if (m_InMainDialog)
		{
			m_WasPausedBeforeMainDialog = m_Paused;
			if (!m_Paused) m_CountPauseSwitches++;
		}
		else if (!m_WasPausedBeforeMainDialog)
		{
			assert(m_Paused);
			m_CountPauseSwitches++;
		}
	}

	m_Camera->SetActive(!inMainDialog);

	if (inMainDialog)
	{
		ResetLoadSaveSubDialogState();
		m_SaveTime = {};
	}
}

void InGame::Tick()
{
	TickContext context;
	context.updateIntervalInMillis = c_UpdateIntervalInMillis;

	m_Model->Tick(context);
}

void InGame::ResetGameUpdate()
{
	mLastTime = UpdateClock::time_point();
	m_NextUpdateTime = UpdateClock::time_point();
	m_CountPauseSwitches = 0;
	m_Paused = false;
}

void InGame::DoGameUpdate()
{
	static constexpr int c_MaxUpdates = 10;

	auto currentTime = UpdateClock::now();

	auto ResetNextUpdateTime = [this, currentTime]() {
		// We update exactly after a whole period.
		// On the first call it's practical to release pressure.
		// After resuming from pausing it can prevent cheating.
		m_NextUpdateTime = currentTime + std::chrono::milliseconds(c_UpdateIntervalInMillis);
	};

	// Direct update.
	if (mLastTime == UpdateClock::time_point())
	{
		mLastTime = currentTime;
	}
	auto dt = std::chrono::duration<double>(currentTime - mLastTime).count();
	DirectUpdate(dt);
	mLastTime = currentTime;

	// Pause handling.
	if (m_CountPauseSwitches & 1)
	{
		if (m_Paused) ResetNextUpdateTime();
		m_Paused = !m_Paused;
	}
	m_CountPauseSwitches = 0;
	if (m_Paused) return;

	// The fix interval update's initialization.
	if (m_NextUpdateTime == UpdateClock::time_point())
	{
		ResetNextUpdateTime();
	}

	// Stepping when the interval is completely expired.
	for (int i = 0; i < c_MaxUpdates && m_NextUpdateTime <= currentTime; i++)
	{
		Tick();
		m_NextUpdateTime += std::chrono::milliseconds(c_UpdateIntervalInMillis);
	}

	if (m_NextUpdateTime <= currentTime)
	{
		// Happens around 10 FPS.
		Logger::Log([&](Logger::Stream& ss) { ss << "Running slow in InGame."; }, LogSeverity::Warning);
	}
}

void InGame::DirectUpdate(double dt)
{
	m_Camera->Update(dt);
}

void InGame::PreUpdate(const ComponentPreUpdateContext& context)
{
	if (!m_HasExternalLoadError)
	{
		assert(m_CameraSceneNodeHandler != nullptr && m_Level != nullptr);

		// Updating the controller first.
		m_Controller->PreUpdate(context);

		// Updating the model.
		DoGameUpdate();

		// Syncing currently here. When multiplayer mode is implemented, this should be done upon receiving
		// the server game state.
		SyncWithServerData();

		m_CameraSceneNodeHandler->UpdateTransformations();

		m_View->PreUpdate(context);
	}

	HandleLoadError();
}

void InGame::PostUpdate(const ComponentPostUpdateContext& context)
{
	if (m_HasExternalLoadError) return;

	m_View->PostUpdate(context);
}

void InGame::RenderFullscreenClear(const ComponentRenderContext& context)
{
	m_View->RenderFullscreenClear(context);
}

void InGame::RenderContent(const ComponentRenderContext& context)
{
	if (m_HasExternalLoadError) return;

	m_View->RenderContent(context);
}

void InGame::RenderGUI(const ComponentRenderContext& context)
{
	if (m_HasExternalLoadError) return;

	m_View->RenderGUI(context);

	CreateMainDialogGUI(context);
}

void InGame::CreateMainDialogGUI(const ComponentRenderContext& context)
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

		if (!m_ClientGameState->GetGameCreationData().Players.IsMultiplayerGame())
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

void InGame::CreateLoadSubDialog(const ComponentRenderContext& context)
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
			LoadGame(context);
		}
		else if (action == LoadGameGUIControl::Action::Exit) ResetLoadSaveSubDialogState();
	}
	nk_end(ctx);
}

void InGame::CreateSaveSubDialog(const ComponentRenderContext& context)
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

void InGame::CreateGameSavedLabel(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	if (m_SaveTime != decltype(m_SaveTime)()
		&& std::chrono::steady_clock::now() - m_SaveTime <= std::chrono::milliseconds(1500))
	{
		nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
		nk_label(ctx, "The game has been saved.", NK_TEXT_CENTERED);
	}
}

void InGame::SyncWithServerData()
{
	m_ClientGameState->Sync();
}

std::string InGame::GetSavePath(const char* fileName) const
{
	return m_Application->GetPathHandler()->GetPathFromResourcesDirectory(
		std::string("SavedGames/") + fileName + ".bin");
}

void InGame::CreateNewGame(const GameCreationData& data)
{
	m_ClientGameState = std::make_unique<ClientGameState>();
	m_ClientGameState->SetGameCreationData(data);

	m_LoadSource = LoadSource::Level;
}

void InGame::RemapPlayerIndices()
{
	assert(m_ClientGameState != nullptr);

	auto& players = m_ClientGameState->GetGameCreationData().Players;

	std::map<uint32_t, uint32_t> levelEditorToPlayerIndexMap;
	uint32_t countPlayers = players.GetCountPlayers();
	for (uint32_t i = 0; i < countPlayers; i++)
	{
		levelEditorToPlayerIndexMap[players[i].LevelEditorIndex] = i;
	}

	auto& gameObjects = m_Level->GetGameObjects();
	auto gEnd = gameObjects.GetEndIterator();
	for (auto gIt = gameObjects.GetBeginIterator(); gIt != gEnd; ++gIt)
	{
		auto pIt = levelEditorToPlayerIndexMap.find(gIt->PlayerIndex);
		assert(pIt != levelEditorToPlayerIndexMap.end());
		gIt->PlayerIndex = pIt->second;
	}
}

void InGame::LoadGame(const char* saveFileName)
{
	m_ClientGameState.reset();

	m_LoadSource = LoadSource::SaveFile;
	m_SaveFileName = (saveFileName != nullptr) ? saveFileName : "";
}

bool InGame::IsSaveFileValid(const char* saveFileName) const
{
	if (saveFileName == nullptr) return false;

	auto saveFilePath = GetSavePath(saveFileName);
	return Core::FileExists(saveFilePath);
}

void InGame::LoadGame(const ComponentRenderContext& context)
{
	m_LoadError.clear();

	auto onError = [this](const std::string& loadError) {
		m_LoadError = loadError;
		if (m_LoadSource == LoadSource::SaveFile)
		{
			// Storing the external load error event explicitly, because the load error string will be cleared after showing it.
			m_HasExternalLoadError = true;
		}
	};

	if (m_SaveFileName.empty())
	{
		onError("No save file has been selected.");
		return;
	}

	auto saveFilePath = GetSavePath(m_SaveFileName.c_str());
	if (!Core::FileExists(saveFilePath))
	{
		onError(std::string("Save file '") + m_SaveFileName + "' does not exist.");
		return;
	}

	auto bytes = Core::ReadAllBytes(saveFilePath);
	auto newClientGameState = std::make_unique<ClientGameState>();
	newClientGameState->DeserializeForLoad(bytes);

	auto levelName = newClientGameState->GetGameCreationData().LevelName;
	if (!HasLevel(levelName.c_str()))
	{
		onError(std::string("Level '") + levelName + "' does not exist.");
		return;
	}
	
	m_ClientGameState = std::move(newClientGameState);

	InitializeOnLoading(context);

	m_InputFreezeTime = std::chrono::steady_clock::now();
}

void InGame::HandleLoadError()
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

void InGame::SaveGame()
{
	auto fileName = m_SaveGameGUIControl->GetFileName();

	Core::ByteVector bytes;
	m_ClientGameState->SerializeForSave(bytes);
	Core::WriteAllBytes(GetSavePath(fileName.c_str()), bytes);

	m_SaveTime = std::chrono::steady_clock::now();
}
