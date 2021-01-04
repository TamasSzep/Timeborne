// Timeborne/Console.h

#pragma once

#include <Timeborne/ApplicationComponent.h>
#include <Timeborne/Logger.h>

#include <deque>
#include <memory>
#include <mutex>
#include <sstream>

class CommandLine;
class Nuklear_TextBox;

class Console : public ApplicationComponent, public ILoggerListener
{
	CommandLine& m_CommandLine;
	std::deque<std::string> m_LogStrings;
	std::stringstream m_SS;
	mutable std::mutex m_Mutex;

private: // GUI.

	std::unique_ptr<Nuklear_TextBox> m_CommandTextBox;

private: // Commands.

	void SubmitCommand();

protected:

	void DerivedInitializeMain(const ComponentInitContext& context) override;

public:

	explicit Console(CommandLine& commandLine);
	~Console() override;

	void OnLog(const char* message, LogSeverity severity, LogFlags flags) override;

	void InitializeRendering(const ComponentRenderContext& context) override {}
	void DestroyMain() override;
	void DestroyRendering() override {}
	bool HandleEvent(const EngineBuildingBlocks::Event* _event) override { return false; }
	void PreUpdate(const ComponentPreUpdateContext& context) override {}
	void PostUpdate(const ComponentPostUpdateContext& context) override {}
	void RenderFullscreenClear(const ComponentRenderContext& context) override {}
	void RenderContent(const ComponentRenderContext& context) override{}
	void RenderGUI(const ComponentRenderContext& context) override;
};