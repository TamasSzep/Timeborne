// Timeborne/Screens/OptionsScreen.cpp

#include <Timeborne/Screens/OptionsScreen.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/GUI/TimeborneGUI.h>
#include <Timeborne/Misc/ScreenResolution.h>
#include <Timeborne/MainApplication.h>

#include <Core/DataStructures/Properties.h>

#include <filesystem>

uint32_t c_MaxTextboxStringLength = 64U;

OptionsScreen::OptionsScreen(const EngineBuildingBlocks::PathHandler& pathHandler)
	: m_PathHandler(pathHandler)
{
	Reset();
}

OptionsScreen::~OptionsScreen()
{
}

const Core::Properties& OptionsScreen::GetOptions()
{
	if (m_Options == nullptr)
	{
		LoadOptions();
	}
	return *m_Options;
}

void OptionsScreen::Reset()
{
	m_NextScreen = ApplicationScreens::Options;
	m_HasConsole = false;

	LoadOptions();
}

void OptionsScreen::Enter(const ComponentRenderContext& context)
{
	context.Application->SetAllowGUIActiveTracking(false);

	Reset();
}

void OptionsScreen::Exit()
{
}

void OptionsScreen::DerivedInitializeMain(const ComponentInitContext& context)
{
}

void OptionsScreen::InitializeRendering(const ComponentRenderContext& context)
{
}

void OptionsScreen::DestroyMain()
{
}

void OptionsScreen::DestroyRendering()
{
}

bool OptionsScreen::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_EscapeECI)
	{
		m_NextScreen = ApplicationScreens::MainMenu;
		return true;
	}
	return false;
}

void OptionsScreen::PreUpdate(const ComponentPreUpdateContext& context)
{
}

void OptionsScreen::PostUpdate(const ComponentPostUpdateContext& context)
{
}

void OptionsScreen::RenderFullscreenClear(const ComponentRenderContext& context)
{
	auto backgroundColor = ToFColor(nk_rgb(28, 48, 62));
	context.DeviceContext->ClearRenderTargetView(context.RTV, glm::value_ptr(backgroundColor));
}

void OptionsScreen::RenderContent(const ComponentRenderContext& context)
{
}

void OptionsScreen::LoadOptions()
{
	m_Options = std::make_unique<Core::Properties>();

	const auto optionsPath = m_PathHandler.GetPathFromResourcesDirectory("Options.xml");
	if (std::filesystem::exists(optionsPath))
	{
		m_Options->LoadFromXml(optionsPath.c_str());
	}

	// Filling default options.
	m_OptionsDirty = false;

	auto loadTextboxedOption = [this](TextboxedOption& option, const char* label, const char* propertyName,
		const char* defaultValue) {
		std::string str;
		if (!m_Options->TryGetPropertyValue(propertyName, str))
		{
			str = defaultValue;
			m_Options->AddProperty(propertyName, defaultValue);
			m_OptionsDirty = true;
		}
		option.Label = label;
		option.PropertyName = propertyName;
		option.LastEditTime = decltype(option.LastEditTime)();
		option.Textbox = std::make_unique<Nuklear_TextBox>(c_MaxTextboxStringLength, true);
		option.Textbox->SetText(str.c_str());
	};

	loadTextboxedOption(m_PlayerNameOption, "Player name", c_OptionsPropertyName_PlayerName, "Player");

	if (m_OptionsDirty)
	{
		m_OptionsDirty = false;
		SaveOptions();
	}
}

void OptionsScreen::SaveOptions() const
{
	m_Options->SaveToXml(m_PathHandler.GetPathFromResourcesDirectory("Options.xml").c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void OptionsScreen::RenderGUI(const ComponentRenderContext& context)
{
	m_GUIRenderTime = std::chrono::steady_clock::now();

	auto ctx = (nk_context*)context.NuklearContext;

	auto mwSize = glm::vec2(context.WindowSize.x, context.WindowSize.y);

	if (Nuklear_BeginWindow(ctx, "Options", glm::vec2(0, 0), mwSize))
	{
		auto prevSelectedGUITab = mSelectedGUITab;
		Nuklear_CreateTabs(ctx, &mSelectedGUITab, { "Player" }, c_ButtonSize.y, (int)MainTabs::COUNT + 1);
		if (TimeborneGUI_CreateExitButton(ctx)) m_NextScreen = ApplicationScreens::MainMenu;

		switch (mSelectedGUITab)
		{
			case MainTabs::Player:
			{
				CreatePlayerOptionsGUI(context);
				break;
			}
		}
	}
	nk_end(ctx);

	if (m_OptionsDirty)
	{
		m_OptionsDirty = false;
		SaveOptions();
	}
}

void OptionsScreen::AddOption(const ComponentRenderContext& context, TextboxedOption& option)
{
	auto ctx = (nk_context*)context.NuklearContext;

	nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 2);

	char buffer[64];
	sprintf_s(buffer, "%s:", option.Label.c_str());
	if (m_GUIRenderTime - option.LastEditTime < std::chrono::milliseconds(1500))
	{
		nk_label_colored(ctx, buffer, NK_TEXT_LEFT, ToNKColor({ 0.0f, 1.0f, 0.0f, 1.0f }));
	}
	else
	{
		nk_label(ctx, buffer, NK_TEXT_LEFT);
	}

	if (option.Textbox->RenderGUI(ctx))
	{
		const auto propertyName = option.PropertyName.c_str();
		const auto oldValue = m_Options->GetPropertyValueStr(propertyName);
		const auto newValue = option.Textbox->GetText();
		if (newValue != oldValue)
		{
			m_Options->SetProperty(propertyName, newValue.c_str());
			m_OptionsDirty = true;
			option.LastEditTime = m_GUIRenderTime;
		}
	}
}

void OptionsScreen::CreatePlayerOptionsGUI(const ComponentRenderContext& context)
{
	AddOption(context, m_PlayerNameOption);
}
