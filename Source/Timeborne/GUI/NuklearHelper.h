// Timeborne/GUI/NuklearHelper.h

#pragma once

#include <Timeborne/GUI/GUIConstants.h>
#include <Timeborne/GUI/NuklearInclude.h>

#include <EngineBuildingBlocks/Math/GLM.h>

#include <functional>
#include <vector>

inline nk_color ToNKColor(const glm::vec4& color)
{
	return nk_rgba_fv(glm::value_ptr(color));
}

inline glm::vec4 ToFColor(nk_color color)
{
	glm::vec4 res(glm::uninitialize);
	nk_color_fv(glm::value_ptr(res), color);
	return res;
}

template <typename TEnum>
void Nuklear_CreateTabs(nk_context* ctx, TEnum* pIndex, const std::initializer_list<const char*>& enumNames, float height, int rowSize = -1)
{
	constexpr nk_color c_TabButtonTextColor = { 255, 0, 0, 255 };

	if (rowSize == -1) rowSize = (int)TEnum::COUNT;
	nk_layout_row_dynamic(ctx, height, rowSize);
	int i = 0;
	for (auto enumName : enumNames)
	{
		auto buttonStyle = ctx->style.button;
		buttonStyle.text_active = c_TabButtonTextColor;
		buttonStyle.text_normal = c_TabButtonTextColor;
		buttonStyle.text_hover = c_TabButtonTextColor;
		if ((*pIndex == (TEnum)i)) buttonStyle.border_color = { 255, 255, 255, 255 };

		if (nk_button_label_styled(ctx, &buttonStyle, enumName)) *pIndex = (TEnum)i;
		i++;
	}

	// It's generally not trivial to draw lines. Workarounds, e.g. a very thin progress bar, seems to have a bad effect on input areas.
}

inline void Nuklear_Slider(nk_context* ctx, const char* label, int minValue, int maxValue, int* pValue)
{
	char buffer[32];
	nk_layout_row_dynamic(ctx, c_ButtonSize.y, 3);
	nk_label(ctx, label, NK_TEXT_LEFT);
	nk_slider_int(ctx, minValue, pValue, maxValue, 1);
	snprintf(buffer, 32, "%d", *pValue);
	nk_label(ctx, buffer, NK_TEXT_LEFT);
}

inline bool Nuklear_CreateCheckbox(nk_context* ctx, bool currentValue, const char* text)
{
	int value = (currentValue ? 0 : 1);
	nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
	nk_checkbox_label(ctx, text, &value);
	return value == 0;
}

inline bool Nuklear_BeginWindow(nk_context* ctx, const char* title, const glm::vec2& start, const glm::vec2& size)
{
	// For some reason Nuklear sizes windows 2 pixels bigger than the specified size. This bug is addressed here.
	auto fixedStart = start + 1.0f;
	auto fixedSize = size - 2.0f;

	auto sTitle = title ? title : "";
	auto titleFlag = title ? NK_WINDOW_TITLE : 0;
	return nk_begin(ctx, sTitle, nk_rect(fixedStart.x, fixedStart.y, fixedSize.x, fixedSize.y), NK_WINDOW_BORDER | titleFlag);
}

class Nuklear_TextBox
{
	bool m_IsCommitting;
	std::vector<char> m_Buffer;
public:
	Nuklear_TextBox(unsigned bufferSize, bool isCommitting = false)
		: m_IsCommitting(isCommitting)
	{
		m_Buffer.resize(bufferSize, '\0');
	}

	std::string GetText() const
	{
		return m_Buffer.data();
	}

	void SetText(const char* text)
	{
		snprintf(m_Buffer.data(), (int)m_Buffer.size(), text);
	}

	void Clear()
	{
		m_Buffer[0] = '\0';
	}

	bool RenderGUI(nk_context* ctx)
	{
		auto editFlags = m_IsCommitting ? NK_EDIT_SIG_ENTER : 0;
		auto flags = nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD | editFlags, m_Buffer.data(), (int)m_Buffer.size(), nullptr);
		return (flags & NK_EDIT_COMMITED) != 0;
	}
};

class Nuklear_OnOffButton
{
	bool m_On = false;
	std::function<void(bool)> m_Handler;

public:

	explicit Nuklear_OnOffButton(const std::function<void(bool)>& handler = [](bool on) {})
		: m_Handler(handler)
	{
	}

	void SetOn(bool on)
	{
		if (on != m_On)
		{
			m_On = on;
			m_Handler(on);
		}
	}

	void RenderGUI(nk_context* ctx)
	{
		auto buttonColor = m_On ? nk_color{ 0, 128, 0, 255 } : nk_color{ 128, 0, 0, 255 };
		auto buttonStyle = ctx->style.button;
		auto setButtonModeToColor = [&buttonColor](nk_style_item& mode) {
			mode.type = NK_STYLE_ITEM_COLOR;
			mode.data.color = buttonColor;
		};
		setButtonModeToColor(buttonStyle.normal);
		setButtonModeToColor(buttonStyle.hover);
		setButtonModeToColor(buttonStyle.active);
		if (nk_button_label_styled(ctx, &buttonStyle, m_On ? "ON" : "OFF"))
		{
			m_On = !m_On;
			m_Handler(m_On);
		}
	}
};