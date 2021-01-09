// Timeborne/Screens/LevelEditorScreen.cpp

#include <Timeborne/Screens/LevelEditorScreen.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/GUI/TimeborneGUI.h>
#include <Timeborne/LevelEditor/LevelEditor.h>
#include <Timeborne/MainApplication.h>

#include <EngineBuildingBlocks/EventHandling.h>

using namespace EngineBuildingBlocks::Input;

LevelEditorScreen::LevelEditorScreen()
	: m_LevelEditor(std::make_unique<LevelEditor>())
	, m_LevelLoadFilePath(std::make_unique<Nuklear_TextBox>(c_MaxLevelNameSize))
	, m_LevelSaveFilePath(std::make_unique<Nuklear_TextBox>(c_MaxLevelNameSize))
	, m_NewLevelName(std::make_unique<Nuklear_TextBox>(c_MaxLevelNameSize))
{
	m_LevelLoadFilePath->SetText(c_DefaultLevelName);
	m_LevelSaveFilePath->SetText(c_DefaultLevelName);
	m_NewLevelName->SetText("new level");

	Reset();
}

LevelEditorScreen::~LevelEditorScreen()
{
}

void LevelEditorScreen::Reset()
{
	m_NextScreen = ApplicationScreens::LevelEditor;
}

void LevelEditorScreen::Exit()
{
}

void LevelEditorScreen::Enter(const ComponentRenderContext& context)
{
	Reset();
}

void LevelEditorScreen::DerivedInitializeMain(const ComponentInitContext& context)
{
	auto application = context.Application;
	assert(application != nullptr);

	auto keyHandler = application->GetKeyHandler();

	m_AutoGrabECI = keyHandler->RegisterStateKeyEventListener("LevelEditor.AutoGrab", application);
	keyHandler->BindEventToKey(m_AutoGrabECI, Keys::G);

	m_LevelEditor->InitializeMain(context);
}

void LevelEditorScreen::InitializeRendering(const ComponentRenderContext& context)
{
	m_LevelEditor->InitializeRendering(context);
}

void LevelEditorScreen::DestroyMain()
{
	m_LevelEditor->DestroyMain();
}

void LevelEditorScreen::DestroyRendering()
{
	m_LevelEditor->DestroyRendering();
}

bool LevelEditorScreen::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_EscapeECI)
	{
		if (!m_LevelEditor->TryEscapeActiveTool()) m_NextScreen = ApplicationScreens::MainMenu;
		return true;
	}
	if (eci == m_AutoGrabECI)
	{
		DoAutoGrabTool();
		return true;
	}
	return 	m_LevelEditor->HandleEvent(_event);
}

void LevelEditorScreen::PreUpdate(const ComponentPreUpdateContext& context)
{
	m_LevelEditor->PreUpdate(context);
}

void LevelEditorScreen::PostUpdate(const ComponentPostUpdateContext& context)
{
	if (m_IsLoadingLevel)
	{
		m_IsLoadingLevel = false;
		m_LevelEditor->LoadLevel(context, m_LevelLoadFilePath->GetText().c_str(),
			m_IsForcingLoadedLevelRecomputations);
	}
	if (m_IsSavingLevel)
	{
		m_IsSavingLevel = false;
		m_LevelEditor->SaveLevel(context, m_LevelSaveFilePath->GetText().c_str());
	}

	m_LevelEditor->PostUpdate(context);
}

void LevelEditorScreen::RenderFullscreenClear(const ComponentRenderContext& context)
{
	m_LevelEditor->RenderFullscreenClear(context);
}

void LevelEditorScreen::RenderContent(const ComponentRenderContext& context)
{
	m_LevelEditor->RenderContent(context);
}

void LevelEditorScreen::RenderGUI(const ComponentRenderContext& context)
{
	// Note that recently the Nuklear integration was fixed, which seems to solve the problem here
	// that was workarounded by the render order: [Main, ActiveTool, for: Components].
	// If any GUI regression is seen, this could be the root cause.

	CreateMainGUI(context);

	m_LevelEditor->RenderGUI(context);

	CreateActiveToolGUI(context);
}

void LevelEditorScreen::DoAutoGrabTool()
{
	auto trySetToolGroupActive = [this](auto selectedTab, auto referenceTab,
		std::initializer_list<LevelEditor::ToolType> toolTypes) {
		if (selectedTab == referenceTab)
		{
			assert(toolTypes.begin() != toolTypes.end());
			auto iIt = std::find(toolTypes.begin(), toolTypes.end(), m_LevelEditor->GetActiveToolType());
			if (iIt == toolTypes.end())
			{
				m_LevelEditor->SetToolActive(*toolTypes.begin(), true);
			}
			else if (iIt == std::prev(toolTypes.end()))
			{
				m_LevelEditor->SetToolActive(*iIt, false);
			}
			else
			{
				m_LevelEditor->SetToolActive(*std::next(iIt), true);
			}
		}
	};

	if (m_SelectedGUITab == MainTabs::Terrain)
	{
		trySetToolGroupActive(m_SelectedTerrainTab, TerrainTabs::HeightTools,
			{ LevelEditor::ToolType::TerrainSpade, LevelEditor::ToolType::TerrainCopy });
	}
	else if (m_SelectedGUITab == MainTabs::GameObjects)
	{
		trySetToolGroupActive(m_SelectedGameObjectsTab, GameObjectTabs::NewObject,
			{ LevelEditor::ToolType::NewGameObject });
		trySetToolGroupActive(m_SelectedGameObjectsTab, GameObjectTabs::ManageObjects,
			{ LevelEditor::ToolType::ManageGameObjects });
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////// GUI /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void CreateSizeExponentSlider(nk_context* ctx, const char* name, int* pValue)
{
	char buffer[32];
	const int minExp = 6;
	const int maxExp = 10;

	nk_label(ctx, name, NK_TEXT_LEFT);
	nk_slider_int(ctx, minExp, pValue, maxExp, 1);
	snprintf(buffer, 32, "%d", (1 << *pValue));
	nk_label(ctx, buffer, NK_TEXT_LEFT);
}

void LevelEditorScreen::CreateMainGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	auto mwSize = glm::vec2(context.WindowSize.x - context.ContentSize.x, c_LevelEditor_MainGUIWindowHeight);

	if (Nuklear_BeginWindow(ctx, "Level Editor", glm::vec2(0.0f, 0.0f), glm::vec2(mwSize.x, mwSize.y)))
	{
		Nuklear_CreateTabs(ctx, &m_SelectedGUITab, { "File", "Render", "Terrain", "Game objects" }, c_ButtonSize.y, (int)MainTabs::COUNT + 1);
		if (TimeborneGUI_CreateExitButton(ctx)) m_NextScreen = ApplicationScreens::MainMenu;

		switch (m_SelectedGUITab)
		{
		case MainTabs::File:
		{
			CreateFileGUI(context);
			break;
		}
		case MainTabs::Render:
		{
			CreateRenderGUI(context);
			break;
		}
		case MainTabs::Terrain:
		{
			Nuklear_CreateTabs(ctx, &m_SelectedTerrainTab, { "Height tools" }, c_ButtonSize.y);

			switch (m_SelectedTerrainTab)
			{
			case TerrainTabs::HeightTools:
			{
				m_LevelEditor->RenderToolGUI(context, LevelEditor::ToolType::TerrainSpade);
				m_LevelEditor->RenderToolGUI(context, LevelEditor::ToolType::TerrainCopy);
				break;
			}
			}

			break;
		}
		case MainTabs::GameObjects:
		{
			Nuklear_CreateTabs(ctx, &m_SelectedGameObjectsTab, { "New object", "Manage objects" },
				c_ButtonSize.y);

			switch (m_SelectedGameObjectsTab)
			{
			case GameObjectTabs::NewObject:
			{
				m_LevelEditor->RenderToolGUI(context, LevelEditor::ToolType::NewGameObject);
				break;
			}
			case GameObjectTabs::ManageObjects:
			{
				m_LevelEditor->RenderToolGUI(context, LevelEditor::ToolType::ManageGameObjects);
				break;
			}
			}

			break;
		}
		}
	}
	nk_end(ctx);
}

void LevelEditorScreen::CreateFileGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	Nuklear_CreateTabs(ctx, &m_SelectedFileTab, { "New level", "Load level", "Save level" }, c_ButtonSize.y);

	switch (m_SelectedFileTab)
	{
	case FileTabs::NewLevel:
	{
		CreateNewLevelGUI(context);
		break;
	}
	case FileTabs::LoadLevel:
	{
		TimeborneGUI_CreateLevelInput(ctx, *m_LevelLoadFilePath);

		nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 1);
		int isForcingLoadedLevelRecomputations = (m_IsForcingLoadedLevelRecomputations ? 0 : 1);
		nk_checkbox_label(ctx, "Force level recomputations", &isForcingLoadedLevelRecomputations);
		m_IsForcingLoadedLevelRecomputations = (isForcingLoadedLevelRecomputations == 0);

		nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 1);
		m_IsLoadingLevel = (nk_button_label(ctx, "Load level") != 0);

		break;
	}
	case FileTabs::SaveLevel:
	{
		nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 2);
		nk_label(ctx, "File path:", NK_TEXT_LEFT);
		m_LevelSaveFilePath->RenderGUI(ctx);

		nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 1);
		m_IsSavingLevel = (nk_button_label(ctx, "Save level") != 0);

		break;
	}
	}
}

void LevelEditorScreen::CreateRenderGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	bool isShowingTerrainGrid = Nuklear_CreateCheckbox(ctx, m_LevelEditor->IsShowingTerrainGrid(),
		"Show terrain grid");
	m_LevelEditor->SetShowingTerrainGrid(isShowingTerrainGrid);

	bool isRenderingTerrainWithWireframe = Nuklear_CreateCheckbox(ctx,
		m_LevelEditor->IsRenderingTerrainWithWireframe(), "Terrain wireframe");
	m_LevelEditor->SetRenderingTerrainWithWireframe(isRenderingTerrainWithWireframe);
}

void LevelEditorScreen::CreateNewLevelGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	auto mwStart = glm::vec2(0, c_LevelEditor_MainGUIWindowHeight);
	auto mwSize = glm::vec2(context.WindowSize.x - context.ContentSize.x, 200);
	auto colSize = glm::vec2(150, 30);

	nk_layout_row_static(ctx, colSize.y, (int)colSize.x, 2);
	nk_label(ctx, "Level name", NK_TEXT_LEFT);
	m_NewLevelName->RenderGUI(ctx);

	nk_layout_row_static(ctx, colSize.y, (int)colSize.x, 3);
	CreateSizeExponentSlider(ctx, "Width", &m_NewLevelSizeExp.x);
	CreateSizeExponentSlider(ctx, "Height", &m_NewLevelSizeExp.y);
	bool isCreatingNewLevel = (nk_button_label(ctx, "Create level") != 0);

	if (isCreatingNewLevel)
	{
		m_LevelEditor->CreateNewLevel(context.Application, m_NewLevelName->GetText().c_str(),
			glm::uvec2(1 << m_NewLevelSizeExp.x, 1 << m_NewLevelSizeExp.y));
	}
}

void LevelEditorScreen::CreateActiveToolGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	auto longLineWidth = 500;

	auto mwStart = glm::vec2(0, c_LevelEditor_MainGUIWindowHeight);
	auto mwSize = glm::vec2(context.WindowSize.x - context.ContentSize.x, c_LevelEditor_ToolGUIWindowHeight);
	auto labelSize = glm::vec2(100, 20);

	if (Nuklear_BeginWindow(ctx, "Tool info", glm::vec2(mwStart.x, mwStart.y), glm::vec2(mwSize.x, mwSize.y)))
	{
		char buffer[64];
		snprintf(buffer, 64, "Current tool: %s", m_LevelEditor->GetCurrentToolName());
		nk_layout_row_static(ctx, c_ButtonSize.y, longLineWidth, 1);
		nk_label(ctx, buffer, NK_TEXT_LEFT);

		auto activeToolInfoStr = m_LevelEditor->GetActiveToolInfoString();

		if (std::strlen(activeToolInfoStr) > 0)
		{
			char infoBuffer[1024];
			snprintf(infoBuffer, 1024, "Current tool info: %s", activeToolInfoStr);
			nk_layout_row_static(ctx, c_ButtonSize.y, longLineWidth, 1);
			nk_label(ctx, infoBuffer, NK_TEXT_LEFT);
		}
	}
	nk_end(ctx);
}
