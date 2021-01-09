// Timeborne/InGame/InGame.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

class ClientGameState;
struct ComponentInitContext;
struct ComponentPreUpdateContext;
struct ComponentPostUpdateContext;
struct ComponentRenderContext;
class CommandList;
class GameCamera;
struct GameCreationData;
class InGameController;
class InGameModel;
class InGameView;
class Level;

class InGame
{
private: // Level data.

	std::unique_ptr<Level> m_Level;

	void LoadLevel(const EngineBuildingBlocks::PathHandler& pathHandler);

private: // Components.

	std::unique_ptr<InGameView> m_View;
	std::unique_ptr<InGameModel> m_Model;
	std::unique_ptr<InGameController> m_Controller;

private:

	std::unique_ptr<CommandList> m_CommandList;

private: // Input.

	unsigned m_PauseECI;

public:

	InGame();
	~InGame();

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

	bool IsMultiplayerGame() const;
	bool IsPaused() const;
	void TriggerPauseSwitch();
	void SetControlsActive(bool active);

private:

	void Reset();

private: // Client-server state syncing.

	std::unique_ptr<ClientGameState> m_ClientGameState;

	void SyncWithServerData();

public: // Create.

	bool HasLevel(const char* levelName,
		const EngineBuildingBlocks::PathHandler& pathHandler) const;
	void CreateNewGame(const GameCreationData& data);
	void LoadGameFromLevel(const ComponentRenderContext& context);

public: // Load, save.

	bool IsSaveFileValid(const char* saveFileName,
		const EngineBuildingBlocks::PathHandler& pathHandler) const;
	void LoadGameFromSaveFile(const ComponentRenderContext& context,
		bool clearState,
		const char* saveFileName,
		std::string& loadError);
	void SaveGame(const char* saveFileName,
		const EngineBuildingBlocks::PathHandler& pathHandler) const;

private:

	std::string GetSavePath(const char* fileName,
		const EngineBuildingBlocks::PathHandler& pathHandler) const;

	void RemapPlayerIndices();

	void InitializeOnLoading(const ComponentRenderContext& context,
		bool loadingFromLevel);

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
};
