// Timeborne/GUI/LoadSaveGUIControl.cpp

#include <Timeborne/GUI/LoadSaveGUIControl.h>

#include <EngineBuildingBlocks/PathHandler.h>

#include <filesystem>

_Detail::LoadSaveGuiControlBase::LoadSaveGuiControlBase()
{
}

_Detail::LoadSaveGuiControlBase::~LoadSaveGuiControlBase()
{
}

void _Detail::LoadSaveGuiControlBase::ResetBase()
{
	m_SaveFiles.clear();
	m_SaveFileStrs.Clear();
}

void _Detail::LoadSaveGuiControlBase::DetermineSaveFiles(const EngineBuildingBlocks::PathHandler& pathHandler)
{
	m_SaveFiles.clear();
	auto saveFileFolder = pathHandler.GetPathFromResourcesDirectory("SavedGames/");
	for (auto& filePath : std::filesystem::directory_iterator(saveFileFolder))
	{
		m_SaveFiles.push_back(filePath.path().filename().replace_extension().string());
	}
}

void _Detail::LoadSaveGuiControlBase::UpdateStringPtrVector()
{
	auto countSavedFiles = (unsigned)m_SaveFiles.size();
	m_SaveFileStrs.Resize(countSavedFiles);
	for (unsigned i = 0; i < countSavedFiles; i++)
	{
		m_SaveFileStrs[i] = m_SaveFiles[i].c_str();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr unsigned c_MaxSaveFileNameLength = 64;

SaveGameGUIControl::SaveGameGUIControl()
	: m_SaveGameTextbox(c_MaxSaveFileNameLength, true)
{
}

SaveGameGUIControl::~SaveGameGUIControl()
{
}

void SaveGameGUIControl::Reset()
{
	ResetBase();

	m_PreviousSaveGameStr.clear();
	m_SaveGameTextbox.Clear();
}

void SaveGameGUIControl::OnScreenEnter(const EngineBuildingBlocks::PathHandler& pathHandler)
{
	DetermineSaveFiles(pathHandler);

	bool noSaveFile = m_SaveFiles.empty();

	m_SaveFiles.push_back("<New file>");

	// Setting the pointers must be done AFTER filling the string vector.
	UpdateStringPtrVector();

	m_SaveGameTextbox.SetText(noSaveFile ? "TestSave" : m_SaveFileStrs[0]);
	m_PreviousSaveGameStr = m_SaveGameTextbox.GetText();
}

SaveGameGUIControl::Action SaveGameGUIControl::Render(nk_context* ctx)
{
	const int c_ComboBoxWidth = 300;
	const int c_ComboBoxHeight = 300;

	auto result = Action::NoAction;

	nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
	nk_label(ctx, "Saved game:", NK_TEXT_LEFT);

	// Synchronized textbox and combobox.
	{
		nk_layout_row_dynamic(ctx, c_ButtonSize.y, 2);

		auto textboxCommitted = m_SaveGameTextbox.RenderGUI(ctx);
		if (m_SaveGameTextbox.GetText() != m_PreviousSaveGameStr)
		{
			m_PreviousSaveGameStr = m_SaveGameTextbox.GetText();
			auto it = std::find(m_SaveFiles.begin(), m_SaveFiles.end(), m_PreviousSaveGameStr);
			m_SelectedSaveFile = (it != m_SaveFiles.end())
				? (int)std::distance(m_SaveFiles.begin(), it)
				: (int)m_SaveFiles.size() - 1;
		}

		auto prevSaveFile = m_SelectedSaveFile;
		nk_combobox(ctx, m_SaveFileStrs.GetArray(), m_SaveFileStrs.GetSize(), &m_SelectedSaveFile,
			(int)c_ButtonSize.y, { (float)c_ComboBoxWidth, (float)c_ComboBoxHeight });
		if (m_SelectedSaveFile != prevSaveFile)
		{
			m_SaveGameTextbox.SetText(m_SaveFileStrs[m_SelectedSaveFile]);
		}

		if (textboxCommitted) result = Action::Save;
	}

	nk_layout_row_dynamic(ctx, c_ButtonSize.y, 2);
	if (nk_button_label(ctx, "Save")) result = Action::Save;
	if (nk_button_label(ctx, "Exit")) result = Action::Exit;

	return result;
}

std::string SaveGameGUIControl::GetFileName() const
{
	return m_SaveGameTextbox.GetText().c_str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LoadGameGUIControl::LoadGameGUIControl(bool hasExitButton, const char* loadText, int itemWidth)
	: m_HasExitButton(hasExitButton)
	, m_LoadText(loadText != nullptr ? loadText : "Load")
	, m_ItemWidth(itemWidth)
{
}

LoadGameGUIControl::~LoadGameGUIControl()
{
}

void LoadGameGUIControl::Reset()
{
	ResetBase();

	m_IsDummySaveFile = false;
}

void LoadGameGUIControl::OnScreenEnter(const EngineBuildingBlocks::PathHandler& pathHandler)
{
	DetermineSaveFiles(pathHandler);

	m_IsDummySaveFile = m_SaveFiles.empty();

	if (m_IsDummySaveFile) m_SaveFiles.push_back("<No save files>");

	// Setting the pointers must be done AFTER filling the string vector.
	UpdateStringPtrVector();
}

LoadGameGUIControl::Action LoadGameGUIControl::Render(nk_context* ctx)
{
	const int c_ComboBoxWidth = 300;
	const int c_ComboBoxHeight = 300;

	auto result = Action::NoAction;

	if (m_ItemWidth > 0)
	{
		nk_layout_row_static(ctx, c_ButtonSize.y, m_ItemWidth, 2);
	}
	else
	{
		nk_layout_row_dynamic(ctx, c_ButtonSize.y, 2);
	}
	nk_label(ctx, "Saved game:", NK_TEXT_LEFT);
	nk_combobox(ctx, m_SaveFileStrs.GetArray(), m_SaveFileStrs.GetSize(), &m_SelectedSaveFile,
		(int)c_ButtonSize.y, { (float)c_ComboBoxWidth, (float)c_ComboBoxHeight });

	int countButtons = m_HasExitButton ? 2 : 1;
	if (m_ItemWidth > 0)
	{
		nk_layout_row_static(ctx, c_ButtonSize.y, m_ItemWidth, countButtons);
	}
	else
	{
		nk_layout_row_dynamic(ctx, c_ButtonSize.y, countButtons);
	}
	if (nk_button_label(ctx, m_LoadText.c_str())) result = Action::Load;
	if (m_HasExitButton && nk_button_label(ctx, "Exit")) result = Action::Exit;

	return result;
}

const char* LoadGameGUIControl::GetFileName() const
{
	return !m_IsDummySaveFile && m_SelectedSaveFile < (int)m_SaveFileStrs.GetSize()
		? m_SaveFileStrs[m_SelectedSaveFile]
		: nullptr;
}
