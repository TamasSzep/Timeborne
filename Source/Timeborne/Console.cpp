// Timeborne/Console.cpp

#include <Timeborne/Console.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/CommandLine.h>

constexpr float c_FontHeight = 10;
constexpr float c_FontHeightMargin = 4;
constexpr float c_NonTextAreaHeight = 50;
constexpr float c_CommandSubmitWindowSize = 54;
constexpr int c_CommandBufferSize = 256;
constexpr int c_MaxCountMessages = 300;

Console::Console(CommandLine& commandLine)
	: m_CommandLine(commandLine)
	, m_CommandTextBox(std::make_unique<Nuklear_TextBox>(c_CommandBufferSize, true))
{
}

Console::~Console()
{
}

void Console::DerivedInitializeMain(const ComponentInitContext& context)
{
	Logger::GetInstance()->SetListener(this, LogSeverity::Debug);
}

void Console::DestroyMain()
{
	Logger::GetInstance()->RemoveListener(this);
}

void Console::OnLog(const char* message, LogSeverity severity, LogFlags flags)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	CreateLogHeader(m_SS, severity, flags);
	m_SS << message;
	m_LogStrings.push_back(m_SS.str());
	if (m_LogStrings.size() > c_MaxCountMessages)
	{
		m_LogStrings.pop_front();
	}
	m_SS.str("");
	m_SS.clear();
}

void Console::SubmitCommand()
{
	// @todo: implement command storing functionality?

	auto command = m_CommandTextBox->GetText();
	m_CommandTextBox->Clear();
	Logger::Log([&](Logger::Stream& stream) { stream << "> " << command; }, LogSeverity::Info, LogFlags::OmitSeverity);
	m_CommandLine.executeCommand(command.c_str());	
}

void Console::RenderGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	// Start and size of the total consolse and command area.
	auto mwStart = glm::vec2(context.WindowSize.x - context.ContentSize.x, context.ContentSize.y);
	auto mwSize = glm::vec2(context.ContentSize.x, context.WindowSize.y - context.ContentSize.y);

	float consoleHeight = mwSize.y - c_CommandSubmitWindowSize;
	float commandStartY = mwStart.y + consoleHeight;

	if (Nuklear_BeginWindow(ctx, "Console", glm::vec2(mwStart.x, mwStart.y), glm::vec2(mwSize.x, consoleHeight)))
	{
		nk_layout_row_dynamic(ctx, c_FontHeight, 1);
		{
			std::lock_guard<std::mutex> lock(m_Mutex);

			// Scrolling to the end of the console is currently not solved in Nuklear. Therefore we simply limit the amount of messages displayed.
			float textAreaSize = context.WindowSize.y - context.ContentSize.y - c_NonTextAreaHeight - c_CommandSubmitWindowSize;
			int fontSizeWithMargin = (int)c_FontHeight + (int)c_FontHeightMargin;
			int countShownMessages = (int)textAreaSize / fontSizeWithMargin;
			int countMessages = (int)m_LogStrings.size();
			for (int i = std::max(countMessages - countShownMessages, 0); i < countMessages; i++)
			{
				nk_label(ctx, m_LogStrings[i].c_str(), NK_TEXT_LEFT);
			}
		}
	}
	nk_end(ctx);
	if (Nuklear_BeginWindow(ctx, nullptr, glm::vec2(mwStart.x, commandStartY), glm::vec2(mwSize.x, c_CommandSubmitWindowSize)))
	{
		float ratios[] = { mwSize.x - c_ButtonSize.x - c_RowMargin, c_ButtonSize.x };
		nk_layout_row(ctx, NK_STATIC, c_ButtonSize.y, 2, ratios);
		bool isSubmitting1 = m_CommandTextBox->RenderGUI(ctx);
		bool isSubmitting2 = (nk_button_label(ctx, "Submit") != 0);
		if (isSubmitting1 || isSubmitting2)
		{
			SubmitCommand();
		}
	}
	nk_end(ctx);
}