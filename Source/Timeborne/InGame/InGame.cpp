// Timeborne/InGame/InGame.cpp

#include <Timeborne/InGame/InGame.h>

#include <Timeborne/InGame/Controller/CommandList.h>
#include <Timeborne/InGame/Controller/InGameController.h>
#include <Timeborne/InGame/GameCamera/GameCamera.h>
#include <Timeborne/InGame/GameState/ClientGameState.h>
#include <Timeborne/InGame/Model/InGameModel.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/InGame/Model/TickContext.h>
#include <Timeborne/InGame/View/InGameView.h>
#include <Timeborne/Logger.h>
#include <Timeborne/MainApplication.h>

#include <Core/System/Filesystem.h>
#include <Core/System/SimpleIO.h>
#include <EngineBuildingBlocks/Input/DefaultInputBinder.h>
#include <EngineBuildingBlocks/SceneNode.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Input;
using namespace EngineBuildingBlocks::Math;

InGame::InGame()
{
	Reset();
}

InGame::~InGame()
{
}

bool InGame::IsMultiplayerGame() const
{
	return m_ClientGameState->GetGameCreationData().Players.IsMultiplayerGame();
}

bool InGame::IsPaused() const
{
	return m_Paused;
}

bool InGame::IsGameEnded() const
{
	// @todo: we will be able to use player data.
	
	// The game is ended if there is only zero or one alliance with game objects.
	assert(m_ClientGameState != nullptr);
	uint32_t allianceIndex = Core::c_InvalidIndexU;
	const auto& gcPlayerData = m_ClientGameState->GetGameCreationData().Players;
	for (auto& gameObjectData : m_ClientGameState->GetClientModelGameState().GetGameObjects().Get())
	{
		auto playerIndex = gameObjectData.second.Data.PlayerIndex;
		if (playerIndex == Core::c_InvalidIndexU) continue;

		auto cAllianceIndex = gcPlayerData[playerIndex].AllianceIndex;
		if (allianceIndex == Core::c_InvalidIndexU)
		{
			allianceIndex = cAllianceIndex;
		}
		else if (cAllianceIndex != allianceIndex)
		{
			return false;
		}
	}
	return true;
}

void InGame::TriggerPauseSwitch()
{
	// Not setting 'm_Paused' directly, generating the input to set it in order to reuse the same pausing mechanism.

	m_CountPauseSwitches++;
}

void InGame::SetControlsActive(bool active)
{
	m_Camera->SetActive(active);
}

const InGameStatistics& InGame::GetStatistics() const
{
	assert(m_ClientGameState != nullptr);
	return m_ClientGameState->GetLocalGameState().GetStatistics();
}

void InGame::Reset()
{
	m_Model.reset();
	m_Controller.reset();

	m_Level.reset();
	m_Camera.reset();
	m_CommandList.reset();

	ResetGameUpdate();
}

bool InGame::HasLevel(const char* levelName, const EngineBuildingBlocks::PathHandler& pathHandler) const
{
	return Core::FileExists(Level::GetPath(pathHandler, levelName));
}

void InGame::LoadLevel(const EngineBuildingBlocks::PathHandler& pathHandler)
{
	auto levelName = m_ClientGameState->GetGameCreationData().LevelName;
	bool levelExists = HasLevel(levelName.c_str(), pathHandler);
	if (levelExists)
	{
		m_Level = std::make_unique<Level>();
		m_Level->Load(pathHandler, levelName, false);

		Logger::Log([&](Logger::Stream& ss) { ss << "Level '" << levelName << "' has been loaded in InGame."; },
			LogSeverity::Info);
	}

	assert(levelExists);
}

void InGame::CreateCamera(const ComponentRenderContext& context, bool isLoadingFromSaveFile)
{
	auto application = context.Application;

	auto eventManager = application->GetEventManager();
	auto keyHandler = application->GetKeyHandler();
	auto mouseHandler = application->GetMouseHandler();

	m_CameraSceneNodeHandler = std::make_unique<EngineBuildingBlocks::SceneNodeHandler>();
	m_Camera = std::make_unique<GameCamera>(m_ClientGameState->GetLocalGameState().GetGameCameraState(),
		m_CameraSceneNodeHandler.get(), eventManager,
		keyHandler, mouseHandler, context.ContentSize, isLoadingFromSaveFile);
}

void InGame::InitializeOnLoading(const ComponentRenderContext& context, bool loadingFromLevel)
{
	assert(context.Application != nullptr && context.Application->GetPathHandler() != nullptr);
	auto& pathHandler = *context.Application->GetPathHandler();

	bool isLoadingFromSaveFile = !loadingFromLevel;
	bool isRemappingPlayerIndices = loadingFromLevel;

	Reset();

	LoadLevel(pathHandler);

	if (isRemappingPlayerIndices)
	{
		RemapPlayerIndices();
	}

	CreateCamera(context, isLoadingFromSaveFile);

	// Creating the command list.
	m_CommandList = std::make_unique<CommandList>();

	assert(m_Level != nullptr && m_ClientGameState != nullptr && m_Camera != nullptr && m_CommandList != nullptr);

	// The view must be loaded before the model, because the loading clears the view and sets up the connections,
	// so when the level is loaded in the model, the view can listen the model's events.
	m_View->Load(*m_Level, *m_ClientGameState, *m_Camera, context);

	// Creating the model.
	m_Model = std::make_unique<InGameModel>(*m_Level, *m_ClientGameState, *m_CommandList, isLoadingFromSaveFile);

	// Creating the controller.
	m_Controller = std::make_unique<InGameController>(*m_Level, *m_ClientGameState, *m_CommandList,
		m_View->GetGameObjectVisibilityProvider(), *m_Camera, *context.Application);
}

void InGame::InitializeMain(const ComponentInitContext& context)
{
	auto application = context.Application;
	auto keyHandler = application->GetKeyHandler();

	// Registering state key events.
	m_PauseECI = keyHandler->RegisterStateKeyEventListener("InGame.Pause", application);

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
	if (_event->ClassId == m_PauseECI)
	{
		m_CountPauseSwitches++;
		return true;
	}

	if (m_Paused) return false;

	return m_Controller->HandleEvent(_event);
}

void InGame::Tick()
{
	assert(m_ClientGameState != nullptr);

	auto& modelGameState = m_ClientGameState->GetClientModelGameState();
	modelGameState.IncreaseTickCount();
	uint32_t tickCount = modelGameState.GetTickCount();

	TickContext context;
	context.UpdateIntervalInMillis = c_UpdateIntervalInMillis;
	context.TickCount = tickCount;

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

void InGame::PostUpdate(const ComponentPostUpdateContext& context)
{
	m_View->PostUpdate(context);
}

void InGame::RenderFullscreenClear(const ComponentRenderContext& context)
{
	m_View->RenderFullscreenClear(context);
}

void InGame::RenderContent(const ComponentRenderContext& context)
{
	m_View->RenderContent(context);
}

void InGame::RenderGUI(const ComponentRenderContext& context)
{
	m_View->RenderGUI(context);
}

void InGame::SyncWithServerData()
{
	m_ClientGameState->Sync();
}

std::string InGame::GetSavePath(const char* fileName, const EngineBuildingBlocks::PathHandler& pathHandler) const
{
	return pathHandler.GetPathFromResourcesDirectory(std::string("SavedGames/") + fileName + ".bin");
}

void InGame::CreateNewGame(const GameCreationData& data)
{
	m_ClientGameState = std::make_unique<ClientGameState>();
	m_ClientGameState->SetGameCreationData(data);
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

bool InGame::IsSaveFileValid(const char* saveFileName,
	const EngineBuildingBlocks::PathHandler& pathHandler) const
{
	if (saveFileName == nullptr) return false;

	auto saveFilePath = GetSavePath(saveFileName, pathHandler);
	return Core::FileExists(saveFilePath);
}

void InGame::LoadGameFromLevel(const ComponentRenderContext& context)
{
	InitializeOnLoading(context, true);
}

void InGame::LoadGameFromSaveFile(const ComponentRenderContext& context, bool clearState, const char* saveFileName,
	std::string& loadError)
{
	if (clearState)
	{
		m_ClientGameState.reset();
	}

	assert(context.Application != nullptr && context.Application->GetPathHandler() != nullptr);
	auto& pathHandler = *context.Application->GetPathHandler();

	auto saveFilePath = GetSavePath(saveFileName, pathHandler);
	if (!Core::FileExists(saveFilePath))
	{
		loadError = std::string("Save file '") + saveFileName + "' does not exist.";
		return;
	}

	auto bytes = Core::ReadAllBytes(saveFilePath);
	auto newClientGameState = std::make_unique<ClientGameState>();
	newClientGameState->DeserializeForLoad(bytes);

	auto levelName = newClientGameState->GetGameCreationData().LevelName;
	if (!HasLevel(levelName.c_str(), pathHandler))
	{
		loadError = std::string("Level '") + levelName + "' does not exist.";
		return;
	}
	
	m_ClientGameState = std::move(newClientGameState);

	InitializeOnLoading(context, false);
}

void InGame::SaveGame(const char* saveFileName, const EngineBuildingBlocks::PathHandler& pathHandler) const
{
	Core::ByteVector bytes;
	m_ClientGameState->SerializeForSave(bytes);
	Core::WriteAllBytes(GetSavePath(saveFileName, pathHandler), bytes);
}
