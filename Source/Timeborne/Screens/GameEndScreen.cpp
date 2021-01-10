// Timeborne/Screens/GameEndScreen.cpp

#include <Timeborne/Screens/GameEndScreen.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/GUI/TimeborneGUI.h>
#include <Timeborne/Misc/ScreenResolution.h>
#include <Timeborne/MainApplication.h>

GameEndScreen::GameEndScreen()
{
	Reset();
}

GameEndScreen::~GameEndScreen()
{
}

void GameEndScreen::Reset()
{
	m_NextScreen = ApplicationScreens::GameEnd;
	m_HasConsole = false;
}

void GameEndScreen::Enter(const ComponentRenderContext& context)
{
	context.Application->SetAllowGUIActiveTracking(false);

	Reset();
}

void GameEndScreen::Exit()
{
}

void GameEndScreen::DerivedInitializeMain(const ComponentInitContext& context)
{
}

void GameEndScreen::InitializeRendering(const ComponentRenderContext& context)
{
}

void GameEndScreen::DestroyMain()
{
}

void GameEndScreen::DestroyRendering()
{
}

bool GameEndScreen::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_EscapeECI)
	{
		m_NextScreen = ApplicationScreens::MainMenu;
		return true;
	}
	return false;
}

void GameEndScreen::PreUpdate(const ComponentPreUpdateContext& context)
{
}

void GameEndScreen::PostUpdate(const ComponentPostUpdateContext& context)
{
}

void GameEndScreen::RenderFullscreenClear(const ComponentRenderContext& context)
{
	auto backgroundColor = ToFColor(nk_rgb(28, 48, 62));
	context.DeviceContext->ClearRenderTargetView(context.RTV, glm::value_ptr(backgroundColor));
}

void GameEndScreen::RenderContent(const ComponentRenderContext& context)
{
}

void GameEndScreen::RenderGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	auto mwSize = glm::vec2(context.WindowSize.x, context.WindowSize.y);

	if (Nuklear_BeginWindow(ctx, "Game ended", glm::vec2(0.0f, 0.0f), glm::vec2(mwSize.x, mwSize.y)))
	{
		nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 1);
		if (TimeborneGUI_CreateExitButton(ctx)) m_NextScreen = ApplicationScreens::MainMenu;
	}
	nk_end(ctx);
}
