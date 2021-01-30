// Timeborne/Screens/OptionsScreen.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

#include <Timeborne/Declarations/CoreDeclarations.h>
#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <chrono>
#include <memory>

constexpr const char* c_OptionsPropertyName_PlayerName = "Player.PlayerName";

class Nuklear_TextBox;

class OptionsScreen : public ApplicationScreen
{
	const EngineBuildingBlocks::PathHandler& m_PathHandler;

protected:

	void DerivedInitializeMain(const ComponentInitContext& context);

public:

	explicit OptionsScreen(const EngineBuildingBlocks::PathHandler& pathHandler);
	~OptionsScreen() override;

	void InitializeRendering(const ComponentRenderContext& context) override;
	void DestroyMain() override;
	void DestroyRendering() override;
	bool HandleEvent(const EngineBuildingBlocks::Event* _event) override;
	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void PostUpdate(const ComponentPostUpdateContext& context) override;
	void RenderFullscreenClear(const ComponentRenderContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;

	void Enter(const ComponentRenderContext& context) override;
	void Exit() override;

public: // For other screens.

	const Core::Properties& GetOptions();

private:
	void Reset();

private: // GUI.

	enum class MainTabs { Player, COUNT };
	MainTabs mSelectedGUITab = MainTabs::Player;

	struct TextboxedOption
	{
		std::string Label;
		std::string PropertyName;
		std::chrono::steady_clock::time_point LastEditTime;
		std::unique_ptr<Nuklear_TextBox> Textbox;
	};

	std::chrono::steady_clock::time_point m_GUIRenderTime;

	TextboxedOption m_PlayerNameOption;

	void AddOption(const ComponentRenderContext& context, TextboxedOption& option);
	void CreatePlayerOptionsGUI(const ComponentRenderContext& context);

private: // Options.

	std::unique_ptr<Core::Properties> m_Options;
	bool m_OptionsDirty = false;

	void LoadOptions();
	void SaveOptions() const;
};