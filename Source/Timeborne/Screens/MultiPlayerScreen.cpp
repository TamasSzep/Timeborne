// Timeborne/Screens/MultiPlayerScreen.cpp

#include <Timeborne/Screens/MultiPlayerScreen.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/GUI/TimeborneGUI.h>
#include <Timeborne/MainApplication.h>

MultiPlayerScreen::MultiPlayerScreen()
{
	Reset();
}

MultiPlayerScreen::~MultiPlayerScreen()
{
}

void MultiPlayerScreen::Reset()
{
	m_NextScreen = ApplicationScreens::MultiPlayer;
	m_HasConsole = false;
	m_NewGameData = {};
}

void MultiPlayerScreen::Enter(const ComponentRenderContext& context)
{
	context.Application->SetAllowGUIActiveTracking(false);

	Reset();

	LanConnection::CheckEndianness();

	EnterMainTab(MainTabs::HostOnLAN);
}

void MultiPlayerScreen::Exit()
{
	if (m_NextScreen != ApplicationScreens::InGame)
	{
		m_LanConnection.Reset();
	}
}

void MultiPlayerScreen::DerivedInitializeMain(const ComponentInitContext& initContext)
{
}

void MultiPlayerScreen::InitializeRendering(const ComponentRenderContext& context)
{
}

void MultiPlayerScreen::DestroyMain()
{
}

void MultiPlayerScreen::DestroyRendering()
{
}

bool MultiPlayerScreen::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_EscapeECI)
	{
		m_NextScreen = ApplicationScreens::MainMenu;
		return true;
	}
	return false;
}

void MultiPlayerScreen::PreUpdate(const ComponentPreUpdateContext& context)
{
}

void MultiPlayerScreen::PostUpdate(const ComponentPostUpdateContext& context)
{
}

void MultiPlayerScreen::RenderFullscreenClear(const ComponentRenderContext& context)
{
	glm::vec4 backgroundColor(0.0f, 0.3f, 1.0f, 1.0f);
	context.DeviceContext->ClearRenderTargetView(context.RTV, glm::value_ptr(backgroundColor));
}

void MultiPlayerScreen::RenderContent(const ComponentRenderContext& context)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MultiPlayerScreen::RenderGUI(const ComponentRenderContext& context)
{
	assert(m_Application->GetPathHandler() != nullptr);

	auto ctx = (nk_context*)context.NuklearContext;

	auto mwSize = glm::vec2(context.WindowSize.x, context.WindowSize.y);

	if (Nuklear_BeginWindow(ctx, "Multiplayer", glm::vec2(0.0f, 0.0f), mwSize))
	{
		auto newSelectedGUITab = m_SelectedGUITab;
		Nuklear_CreateTabs(ctx, &newSelectedGUITab, { "Host on LAN", "Connect ON LAN" }, c_ButtonSize.y, (int)MainTabs::COUNT + 1);
		if (TimeborneGUI_CreateExitButton(ctx)) m_NextScreen = ApplicationScreens::MainMenu;

		if (newSelectedGUITab != m_SelectedGUITab)
		{
			EnterMainTab(newSelectedGUITab);
		}

		switch (m_SelectedGUITab)
		{
			case MainTabs::HostOnLAN:
			{
				CreateHostOnLANGUI(context);
				break;
			}
			case MainTabs::ConnectOnLAN:
			{
				CreateConnectOnLANGUI(context);
				break;
			}
		}
	}
	nk_end(ctx);
}

void MultiPlayerScreen::EnterMainTab(MainTabs tab)
{
	m_LanConnection.Reset();

	switch (tab)
	{
		case MainTabs::HostOnLAN: m_LanConnection.GetServer().Start(); break;
		case MainTabs::ConnectOnLAN: m_LanConnection.GetClient().ListServers(); break;
	}

	m_SelectedGUITab = tab;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MultiPlayerScreen::CreateHostOnLANGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MultiPlayerScreen::CreateConnectOnLANGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;
}
