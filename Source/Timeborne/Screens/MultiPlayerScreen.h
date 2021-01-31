// Timeborne/Screens/MultiPlayerScreen.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

#include <Timeborne/GameCreation/GameCreationData.h>
#include <Timeborne/Misc/LanConnection.h>

#include <memory>

class Level;
class Nuklear_TextBox;

class MultiPlayerScreen : public ApplicationScreen
{
public:

	MultiPlayerScreen();

private:

	void Reset();

public: // ApplicationComponent IF.

	~MultiPlayerScreen() override;

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

private: // Connection.

	LanConnection m_LanConnection;

private: // GUI.

	enum class MainTabs { HostOnLAN, ConnectOnLAN, COUNT };
	MainTabs m_SelectedGUITab = MainTabs::HostOnLAN;

	void EnterMainTab(MainTabs tab);

	GameCreationData m_NewGameData;

private: // Host on LAN.

	void CreateHostOnLANGUI(const ComponentRenderContext& context);

private: // Connect on LAN.

	void CreateConnectOnLANGUI(const ComponentRenderContext& context);
};