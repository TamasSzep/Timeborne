// Timeborne/Screens/MainMenuScreen.cpp

#include <Timeborne/Screens/MainMenuScreen.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/Misc/ScreenResolution.h>
#include <Timeborne/MainApplication.h>

MainMenuScreen::MainMenuScreen()
{
	Reset();
}

MainMenuScreen::~MainMenuScreen()
{
}

void MainMenuScreen::Reset()
{
	m_NextScreen = ApplicationScreens::MainMenu;
	m_HasConsole = false;
}

void MainMenuScreen::Enter(const ComponentRenderContext& context)
{
	context.Application->SetAllowGUIActiveTracking(false);

	Reset();
}

void MainMenuScreen::Exit()
{
}

void MainMenuScreen::DerivedInitializeMain(const ComponentInitContext& context)
{
}

void MainMenuScreen::InitializeRendering(const ComponentRenderContext& context)
{
}

void MainMenuScreen::DestroyMain()
{
}

void MainMenuScreen::DestroyRendering()
{
}

bool MainMenuScreen::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_EscapeECI)
	{
		m_IsExiting = true;
		return true;
	}
	return false;
}

void MainMenuScreen::PreUpdate(const ComponentPreUpdateContext& context)
{
}

void MainMenuScreen::PostUpdate(const ComponentPostUpdateContext& context)
{
}

void MainMenuScreen::RenderFullscreenClear(const ComponentRenderContext& context)
{
	auto backgroundColor = ToFColor(nk_rgb(28, 48, 62));
	context.DeviceContext->ClearRenderTargetView(context.RTV, glm::value_ptr(backgroundColor));
}

void MainMenuScreen::RenderContent(const ComponentRenderContext& context)
{
}

void MainMenuScreen::RenderGUI(const ComponentRenderContext& context)
{
	if (m_IsExiting)
	{
		// Requesting exit only AFTER the exit state was rendered once. Therefore this check must precede
		// the setting later in this function.
		m_Application->RequestExiting();
	}

	auto ctx = (nk_context*)context.NuklearContext;

	auto mwStart = RatioToPixels(glm::vec2(0.4f, 0.4f), context.WindowSize);
	auto mwSize = RatioToPixels(glm::vec2(0.2f, 0.2f), context.WindowSize);

	constexpr float c_ButtonHeight = 30;
	constexpr float c_ExitHeight = 150;

	auto height = m_IsExiting ? c_ExitHeight : mwSize.y;

	if (Nuklear_BeginWindow(ctx, "Menu", glm::vec2(mwStart.x, mwStart.y), glm::vec2(mwSize.x, height)))
	{
		if (m_IsExiting)
		{
			nk_layout_row_dynamic(ctx, c_ButtonHeight, 1); // Empty row.
			nk_layout_row_dynamic(ctx, c_ButtonHeight, 1);
			nk_label(ctx, "Exiting...", NK_TEXT_CENTERED);
		}
		else
		{
			nk_layout_row_dynamic(ctx, c_ButtonHeight, 1);
			if (nk_button_label(ctx, "Level editor")) m_NextScreen = ApplicationScreens::LevelEditor;
			nk_layout_row_dynamic(ctx, c_ButtonHeight, 1);
			if (nk_button_label(ctx, "Single player")) m_NextScreen = ApplicationScreens::SinglePlayer;
			nk_layout_row_dynamic(ctx, c_ButtonHeight, 1);
			if (nk_button_label(ctx, "Options")) {}
			nk_layout_row_dynamic(ctx, c_ButtonHeight, 1);
			if (nk_button_label(ctx, "Exit")) m_IsExiting = true;
		}
	}
	nk_end(ctx);
}
