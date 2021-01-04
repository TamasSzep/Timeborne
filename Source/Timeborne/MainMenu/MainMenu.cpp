// Timeborne/MainMenu/MainMenu.cpp

#include <Timeborne/MainMenu/MainMenu.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/Misc/ScreenResolution.h>
#include <Timeborne/MainApplication.h>

inline nk_color ToNKColor(const glm::vec4& color) { return nk_rgba_fv(glm::value_ptr(color)); }
inline glm::vec4 ToFColor(nk_color color) { glm::vec4 res(glm::uninitialize); nk_color_fv(glm::value_ptr(res), color); return res; }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MainMenu::MainMenu()
{
	Reset();
}

MainMenu::~MainMenu()
{
}

void MainMenu::Reset()
{
	m_NextScreen = ApplicationScreens::MainMenu;
	m_HasConsole = false;
}

void MainMenu::Enter(const ComponentRenderContext& context)
{
	context.Application->SetAllowGUIActiveTracking(false);

	Reset();
}

void MainMenu::Exit()
{
}

void MainMenu::DerivedInitializeMain(const ComponentInitContext& context)
{
}

void MainMenu::InitializeRendering(const ComponentRenderContext& context)
{
}

void MainMenu::DestroyMain()
{
}

void MainMenu::DestroyRendering()
{
}

bool MainMenu::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_EscapeECI)
	{
		m_IsExiting = true;
		return true;
	}
	return false;
}

void MainMenu::PreUpdate(const ComponentPreUpdateContext& context)
{
}

void MainMenu::PostUpdate(const ComponentPostUpdateContext& context)
{
}

void MainMenu::RenderFullscreenClear(const ComponentRenderContext& context)
{
	auto backgroundColor = ToFColor(nk_rgb(28, 48, 62));
	context.DeviceContext->ClearRenderTargetView(context.RTV, glm::value_ptr(backgroundColor));
}

void MainMenu::RenderContent(const ComponentRenderContext& context)
{
}

void MainMenu::RenderGUI(const ComponentRenderContext& context)
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
