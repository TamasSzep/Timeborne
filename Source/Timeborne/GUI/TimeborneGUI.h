// Timeborne/GUI/TimeborneGUI.h

#pragma once

#include <Timeborne/GUI/NuklearHelper.h>

////////////////////////////////////////////////////// Exit button //////////////////////////////////////////////////////

inline bool TimeborneGUI_CreateExitButton(nk_context* ctx)
{
	constexpr nk_color c_ExitButtonTextColor = { 255, 255, 255, 255 };
	auto exitButtonStyle = ctx->style.button;
	exitButtonStyle.text_active = c_ExitButtonTextColor;
	exitButtonStyle.text_normal = c_ExitButtonTextColor;
	exitButtonStyle.text_hover = c_ExitButtonTextColor;
	return nk_button_label_styled(ctx, &exitButtonStyle, "Exit");
}

////////////////////////////////////////////////////// Level input //////////////////////////////////////////////////////

constexpr unsigned c_MaxLevelNameSize = 32;
static const char* c_DefaultLevelName = "TestLevel";

inline void TimeborneGUI_CreateLevelInput(nk_context* ctx, Nuklear_TextBox& levelLoadTextBox)
{
	nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 2);
	nk_label(ctx, "File path:", NK_TEXT_LEFT);
	levelLoadTextBox.RenderGUI(ctx);
}
