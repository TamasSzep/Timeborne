// Timeborne/GUI/LoadSaveGUIControl.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>
#include <Timeborne/GUI/NuklearHelper.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <memory>
#include <string>
#include <vector>

namespace _Detail
{
	class LoadSaveGuiControlBase
	{
	protected:
		std::vector<std::string> m_SaveFiles;
		Core::SimpleTypeVectorU<const char*> m_SaveFileStrs;
		int m_SelectedSaveFile = 0;

		void ResetBase();

		void DetermineSaveFiles(const EngineBuildingBlocks::PathHandler& pathHandler);
		void UpdateStringPtrVector();

	public:
		LoadSaveGuiControlBase();
		~LoadSaveGuiControlBase();
	};
}

class SaveGameGUIControl : public _Detail::LoadSaveGuiControlBase
{
	std::string m_PreviousSaveGameStr;
	Nuklear_TextBox m_SaveGameTextbox;

public:

	enum class Action { NoAction, Save, Exit };

	SaveGameGUIControl();
	~SaveGameGUIControl();

	void Reset();
	Action Render(nk_context* ctx);

	void OnScreenEnter(const EngineBuildingBlocks::PathHandler& pathHandler);

	std::string GetFileName() const;
};

class LoadGameGUIControl : public _Detail::LoadSaveGuiControlBase
{
	bool m_IsDummySaveFile = false;
	bool m_HasExitButton;
	std::string m_LoadText;
	int m_ItemWidth;

public:

	enum class Action { NoAction, Load, Exit };

	LoadGameGUIControl(bool hasExitButton, const char* loadText = nullptr,
		int itemWidth = 0);
	~LoadGameGUIControl();

	void Reset();
	Action Render(nk_context* ctx);

	void OnScreenEnter(const EngineBuildingBlocks::PathHandler& pathHandler);

	const char* GetFileName() const;
};