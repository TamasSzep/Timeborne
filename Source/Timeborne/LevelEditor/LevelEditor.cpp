// Timeborne/LevelEditor/LevelEditor.cpp

#include <Timeborne/LevelEditor/LevelEditor.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/GUI/TimeborneGUI.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorModel.h>
#include <Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorView.h>
#include <Timeborne/LevelEditor/GameObjects/ManageGameObjectsTool.h>
#include <Timeborne/LevelEditor/GameObjects/NewGameObjectsTool.h>
#include <Timeborne/LevelEditor/Terrain/TerrainCopyTool.h>
#include <Timeborne/LevelEditor/Terrain/TerrainLevelEditorView.h>
#include <Timeborne/LevelEditor/Terrain/TerrainSpadeTool.h>
#include <Timeborne/Misc/ScreenResolution.h>
#include <Timeborne/Logger.h>
#include <Timeborne/MainApplication.h>

#include <Core/System/Filesystem.h>
#include <Core/System/SimpleIO.h>
#include <EngineBuildingBlocks/Graphics/Camera/FreeCamera.h>
#include <EngineBuildingBlocks/Graphics/Lighting/Lighting1.h>
#include <EngineBuildingBlocks/Input/DefaultInputBinder.h>
#include <DirectX11Render/Utilities/RenderPassCB.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Input;
using namespace EngineBuildingBlocks::Math;
using namespace DirectXRender;
using namespace DirectX11Render;

constexpr unsigned c_MaxCountLights = 16;

LevelEditor::LevelEditor()
	: m_LevelLoadFilePath(std::make_unique<Nuklear_TextBox>(c_MaxLevelNameSize))
	, m_LevelSaveFilePath(std::make_unique<Nuklear_TextBox>(c_MaxLevelNameSize))
	, m_TerrainLevelEditorView(std::make_unique<TerrainLevelEditorView>())
	, m_GameObjectLevelEditorModel(std::make_unique<GameObjectLevelEditorModel>())
	, m_GameObjectLevelEditorView(std::make_unique<GameObjectLevelEditorView>(*m_GameObjectLevelEditorModel))
	, m_NewLevelName(std::make_unique<Nuklear_TextBox>(c_MaxLevelNameSize))
{
	m_Components.push_back(m_TerrainLevelEditorView.get());
	m_Components.push_back(m_GameObjectLevelEditorModel.get());
	m_Components.push_back(m_GameObjectLevelEditorView.get());

	m_LevelLoadFilePath->SetText(c_DefaultLevelName);
	m_LevelSaveFilePath->SetText(c_DefaultLevelName);
	m_NewLevelName->SetText("new level");

	Reset();
}

LevelEditor::~LevelEditor()
{
}

void LevelEditor::Reset()
{
	m_NextScreen = ApplicationScreens::LevelEditor;
}

void LevelEditor::Enter(const ComponentRenderContext& context)
{
	Reset();
}

void LevelEditor::Exit()
{
}

template <typename T, typename... TArgs>
void AddTool(std::vector<std::unique_ptr<LevelEditorComponent>>& tools, unsigned* pToolIndex, TArgs&&... args)
{
	*pToolIndex = (unsigned)tools.size();
	tools.push_back(std::make_unique<T>(std::forward<TArgs>(args)...));
}

void LevelEditor::DerivedInitializeMain(const ComponentInitContext& context)
{
	auto keyHandler = context.Application->GetKeyHandler();
	auto mouseHandler = context.Application->GetMouseHandler();

	m_CameraSceneNodeHandler = std::make_unique<SceneNodeHandler>();
	m_Camera = std::make_unique<FreeCamera>(m_CameraSceneNodeHandler.get(), keyHandler, mouseHandler);
	DefaultInputBinder::BindFreeCamera(keyHandler, mouseHandler, m_Camera.get());

	auto plusECI = keyHandler->RegisterStateKeyEventListener("LevelEditor.Plus", m_Application);
	auto minusECI = keyHandler->RegisterStateKeyEventListener("LevelEditor.Minus", m_Application);

	m_AutoGrabECI = keyHandler->RegisterStateKeyEventListener("LevelEditor.AutoGrab", m_Application);

	keyHandler->BindEventToKey(plusECI, Keys::KeyPadAdd);
	keyHandler->BindEventToKey(minusECI, Keys::KeyPadSubtract);
	keyHandler->BindEventToKey(m_AutoGrabECI, Keys::G);

	auto editDragECI = mouseHandler->RegisterMouseDragEventListener("LevelEditor.EditDrag", m_Application);
	mouseHandler->BindEventToButton(editDragECI, MouseButton::Right);

	LEC_InputData inputData { plusECI, minusECI, editDragECI };

	// Creating tools.
	AddTool<TerrainSpadeTool>(m_Tools, &m_TerrainSpadeToolIndex, *m_TerrainLevelEditorView);
	AddTool<TerrainCopyTool>(m_Tools, &m_TerrainCopyToolIndex, *m_TerrainLevelEditorView);
	AddTool<NewGameObjectsTool>(m_Tools, &m_NewGameObjectToolIndex, *m_GameObjectLevelEditorModel);
	AddTool<ManageGameObjectsTool>(m_Tools, &m_ManageGameObjectsToolIndex, *m_GameObjectLevelEditorModel,
		*m_TerrainLevelEditorView);

	auto InitializeComponent = [this, &context, &inputData](auto& component) {
		component.InitializeMain(context);
		component.SetLevelEditor(this);
		component.SetInput(inputData);
	};

	for (auto component : m_Components)
	{
		InitializeComponent(*component);
	}
	for (auto& tool : m_Tools)
	{
		InitializeComponent(*tool);
	}
}

void LevelEditor::InitializeRendering(const ComponentRenderContext& context)
{
	m_RenderPassCB = CreateRenderPassCB(context.Device);
	m_LightingCB.Initialize(context.Device, sizeof(Lighting_Dir_Spot_CBType1<c_MaxCountLights>), 1);

	// Initializing camera.
	{
		constexpr float fovY = glm::radians(60.0f);
		constexpr float nearPlaneDistance = 0.1f;
		constexpr float farPlaneDistance = 10000.0f;

		glm::vec3 cameraPosition(0.0f, 100.0f, 0.0f);
		glm::vec3 cameraDirection(1.0f, -0.8f, 1.0);

		float aspectRatio = context.ContentSize.x / (float)context.ContentSize.y;

		m_Camera->SetLocation(cameraPosition, cameraDirection);
		m_Camera->SetPerspectiveProjection(fovY, aspectRatio, nearPlaneDistance, farPlaneDistance, true);
	}

	SetLightingCBData(context);

	auto InitializeComponentRendering = [this, &context](auto& component) {
		component.SetCBs(&m_RenderPassCB, &m_LightingCB);
		component.InitializeRendering(context);
		component.SetCamera(m_Camera.get());
	};

	for (auto component : m_Components)
	{
		InitializeComponentRendering(*component);
	}
	for (auto& tool : m_Tools)
	{
		InitializeComponentRendering(*tool);
	}
}

void LevelEditor::DestroyMain()
{
	for (auto& tool : m_Tools)
	{
		tool->DestroyMain();
	}
}

void LevelEditor::DestroyRendering()
{
	for (auto component : m_Components)
	{
		component->DestroyRendering();
	}
	for (auto& tool : m_Tools)
	{
		tool->DestroyRendering();
	}
}

bool LevelEditor::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_EscapeECI)
	{
		if (m_ActiveToolIndex != Core::c_InvalidIndexU)
		{
			m_Tools[m_ActiveToolIndex]->SetActive(false);
			m_ActiveToolIndex = Core::c_InvalidIndexU;
		}
		else m_NextScreen = ApplicationScreens::MainMenu;
		return true;
	}
	else if (eci == m_AutoGrabECI)
	{
		DoAutoGrabTool();
		return true;
	}
	auto tool = GetActiveTool();
	if (tool != nullptr) return tool->HandleEvent(_event);
	return false;
}

void LevelEditor::PreUpdate(const ComponentPreUpdateContext& context)
{
	UpdateRenderPassCBData();

	for (auto component : m_Components)
	{
		component->PreUpdate(context);
	}

	auto tool = GetActiveTool();
	if (tool != nullptr) tool->PreUpdate(context);
}

void LevelEditor::PostUpdate(const ComponentPostUpdateContext& context)
{
	m_Camera->Update(*context.SystemTime);

	auto& pathHandler = *context.PathHandler;

	if (m_IsLoadingLevel)
	{
		m_IsLoadingLevel = false;
		auto loadPath = m_LevelLoadFilePath->GetText();
		if (Core::FileExists(Level::GetPath(pathHandler, loadPath)))
		{
			m_Level = std::make_unique<Level>();
			m_Level->Load(pathHandler, loadPath, m_IsForcingLoadedLevelRecomputations);
			OnLevelLoaded(LevelEditorComponent::LevelDirtyFlags::All);
			LoadLevelMetadata(pathHandler, loadPath);
			Logger::Log([&](Logger::Stream& ss) { ss << "Level '" << loadPath << "' has been loaded in LevelEditor."; }, LogSeverity::Info);
		}
		else
		{
			Logger::Log([&](Logger::Stream& ss) { ss << "Level '" << loadPath << "' does not exist in LevelEditor."; }, LogSeverity::Warning,
				LogFlags::AddMessageBox);
		}
	}
	if (m_IsSavingLevel)
	{
		m_IsSavingLevel = false;

		if (m_Level != nullptr)
		{
			auto levelName = m_LevelSaveFilePath->GetText();

			m_Level->CreateTerrainTree(context.Application);
			m_Level->Save(pathHandler, levelName);
			OnLevelLoaded(LevelEditorComponent::LevelDirtyFlags::TerrainTree);
			SaveLevelMetadata(pathHandler, levelName);
			Logger::Log([&](Logger::Stream& ss) { ss << "Level '" << levelName << "' has been saved in LevelEditor."; }, LogSeverity::Info);
		}
		else
		{
			Logger::Log([&](Logger::Stream& ss) { ss << "Cannot save the level, because no level has been created or loaded."; }, LogSeverity::Warning,
				LogFlags::AddMessageBox);
		}
	}

	auto tool = GetActiveTool();
	if (tool != nullptr) tool->PostUpdate(context);
}

void LevelEditor::SetLightingCBData(const ComponentRenderContext& context)
{
	auto& lightingCBData = m_LightingCB.Access<Lighting_Dir_Spot_CBType1<c_MaxCountLights>>(0);
	CreateDefaultLighting1(lightingCBData);
	m_LightingCB.Update(context.DeviceContext);
}

void LevelEditor::UpdateRenderPassCBData()
{
	UpdateRenderPassCB(m_Camera.get(), &m_RenderPassCB);
}

void LevelEditor::RenderFullscreenClear(const ComponentRenderContext& context)
{
	glm::vec4 backgroundColor(0.0f, 0.0f, 0.0f, 1.0f);
	context.DeviceContext->ClearRenderTargetView(context.RTV, glm::value_ptr(backgroundColor));
}

void LevelEditor::RenderContent(const ComponentRenderContext& context)
{
	m_RenderPassCB.Update(context.DeviceContext);

	if (m_Level != nullptr)
	{
		for (auto component : m_Components)
		{
			component->RenderContent(context);
		}

		auto tool = GetActiveTool();
		if (tool != nullptr) tool->RenderContent(context);
	}
}

void LevelEditor::RenderGUI(const ComponentRenderContext& context)
{
	// Note that recently the Nuklear integration was fixed, which seems to solve the problem here
	// that was workarounded by the render order: [Main, ActiveTool, for: Components].
	// If any GUI regression is seen, this could be the root cause.

	CreateMainGUI(context);

	if (m_Level != nullptr)
	{
		for (auto component : m_Components)
		{
			component->RenderGUI(context);
		}
	}

	CreateActiveToolGUI(context);
}

inline std::string GetMetaDataPath(const PathHandler& pathHandler, const std::string& levelName)
{
	return Level::GetPath(pathHandler, levelName) + ".editdata";
}

void LevelEditor::LoadLevelMetadata(const PathHandler& pathHandler, const std::string& levelName)
{
	auto metaDataPath = GetMetaDataPath(pathHandler, levelName);
	if (Core::FileExists(metaDataPath))
	{
		Core::ByteVector metaBytes;
		Core::ReadAllBytes(GetMetaDataPath(pathHandler, levelName), metaBytes);
		const unsigned char* pBytes = metaBytes.GetArray();
		Core::DeserializeSB(pBytes, *m_Camera);
		bool isShowingTerrainGrid, isRenderingTerrainWithWireframe;
		Core::DeserializeSB(pBytes, isShowingTerrainGrid);
		Core::DeserializeSB(pBytes, isRenderingTerrainWithWireframe);
		m_TerrainLevelEditorView->SetShowingTerrainGrid(isShowingTerrainGrid);
		m_TerrainLevelEditorView->SetRenderingTerrainWithWireframe(isRenderingTerrainWithWireframe);
	}
}

void LevelEditor::SaveLevelMetadata(const PathHandler& pathHandler, const std::string& levelName) const
{
	Core::ByteVector metaBytes;
	Core::SerializeSB(metaBytes, *m_Camera);
	Core::SerializeSB(metaBytes, m_TerrainLevelEditorView->IsShowingTerrainGrid());
	Core::SerializeSB(metaBytes, m_TerrainLevelEditorView->IsRenderingTerrainWithWireframe());
	Core::WriteAllBytes(GetMetaDataPath(pathHandler, levelName), metaBytes);
}

void LevelEditor::OnLevelLoaded(LevelEditorComponent::LevelDirtyFlags dirtyFlags)
{
	assert(m_Level != nullptr);

	for (auto component : m_Components)
	{
		component->SetLevel(m_Level.get());
		component->OnLevelLoaded(dirtyFlags);
	}
	for (auto& tool : m_Tools)
	{
		tool->SetLevel(m_Level.get());
		tool->OnLevelLoaded(dirtyFlags);
	}
}

const char* LevelEditor::GetCurrentToolName()
{
	auto tool = GetActiveTool();
	if (tool == nullptr) return "none";
	return tool->GetName();
}

void LevelEditor::UpdateActiveToolInfoString()
{
	auto tool = GetActiveTool();
	if (tool == nullptr) m_ActiveToolInfo = "";
	else m_ActiveToolInfo = tool->GetInfoString();
}

void LevelEditor::HandleToolActivityChange(unsigned toolIndex)
{
	auto tool = m_Tools[toolIndex].get();
	bool active = tool->IsActive();

	if (active && (m_ActiveToolIndex != toolIndex))
	{
		if (m_ActiveToolIndex != Core::c_InvalidIndexU) m_Tools[m_ActiveToolIndex]->SetActive(false);
		m_ActiveToolIndex = toolIndex;
	}
	else if (!active && (m_ActiveToolIndex == toolIndex))
	{
		m_ActiveToolIndex = Core::c_InvalidIndexU;
	}
}

void LevelEditor::DoAutoGrabTool()
{
	auto setToolActive = [this](unsigned toolIndex, bool active) {
		m_Tools[toolIndex]->SetActive(active);
		HandleToolActivityChange(toolIndex);
	};
	auto trySetToolGroupActive = [&](auto selectedTab, auto referenceTab, std::initializer_list<unsigned> toolIndices) {
		if (selectedTab == referenceTab)
		{
			assert(toolIndices.begin() != toolIndices.end());
			auto iIt = std::find(toolIndices.begin(), toolIndices.end(), m_ActiveToolIndex);
			if (iIt == toolIndices.end())
			{
				setToolActive(*toolIndices.begin(), true);
			}
			else if (iIt == std::prev(toolIndices.end()))
			{
				setToolActive(*iIt, false);
			}
			else
			{
				setToolActive(*std::next(iIt), true);
			}
		}
	};

	if (m_SelectedGUITab == MainTabs::Terrain)
	{
		trySetToolGroupActive(m_SelectedTerrainTab, TerrainTabs::HeightTools,
			{ m_TerrainSpadeToolIndex, m_TerrainCopyToolIndex });
	}
	else if (m_SelectedGUITab == MainTabs::GameObjects)
	{
		trySetToolGroupActive(m_SelectedGameObjectsTab, GameObjectTabs::NewObject, { m_NewGameObjectToolIndex });
		trySetToolGroupActive(m_SelectedGameObjectsTab, GameObjectTabs::ManageObjects, { m_ManageGameObjectsToolIndex });
	}

	// Setting the current tool inactive and adjusting m_ActiveToolIndex is handled in RenderToolGUI(...).
}

void LevelEditor::OnTerrainHeightsDirty(const glm::ivec2& changeStart, const glm::ivec2& changeEnd)
{
	assert(m_Level != nullptr);

	m_Level->GetTerrain().UpdateSurfaceData(changeStart, changeEnd);
	m_Level->GetFieldHeightQuadtree().Update(changeStart, changeEnd);

	for (auto component : m_Components)
	{
		component->OnTerrainHeightsDirty(changeStart, changeEnd);
	}
	for (auto& tool : m_Tools)
	{
		tool->OnTerrainHeightsDirty(changeStart, changeEnd);
	}
}

LevelEditorComponent* LevelEditor::GetActiveTool()
{
	if (m_ActiveToolIndex == Core::c_InvalidIndexU) return nullptr;
	return m_Tools[(int)m_ActiveToolIndex].get();
}

void LevelEditor::RenderToolGUI(const ComponentRenderContext& context, unsigned toolIndex)
{
	m_Tools[toolIndex]->RenderGUI(context);

	HandleToolActivityChange(toolIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////// GUI /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LevelEditor::CreateMainGUI(const ComponentRenderContext& context)
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
						RenderToolGUI(context, m_TerrainSpadeToolIndex);
						RenderToolGUI(context, m_TerrainCopyToolIndex);
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
						RenderToolGUI(context, m_NewGameObjectToolIndex);
						break;
					}
					case GameObjectTabs::ManageObjects:
					{
						RenderToolGUI(context, m_ManageGameObjectsToolIndex);
						break;
					}
				}

				break;
			}
		}
	}
	nk_end(ctx);
}

void LevelEditor::CreateFileGUI(const ComponentRenderContext& context)
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

void LevelEditor::CreateRenderGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	bool isShowingTerrainGrid = Nuklear_CreateCheckbox(ctx, m_TerrainLevelEditorView->IsShowingTerrainGrid(),
		"Show terrain grid");
	m_TerrainLevelEditorView->SetShowingTerrainGrid(isShowingTerrainGrid);

	bool isRenderingTerrainWithWireframe = Nuklear_CreateCheckbox(ctx,
		m_TerrainLevelEditorView->IsRenderingTerrainWithWireframe(), "Terrain wireframe");
	m_TerrainLevelEditorView->SetRenderingTerrainWithWireframe(isRenderingTerrainWithWireframe);
}

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

void LevelEditor::CreateNewLevelGUI(const ComponentRenderContext& context)
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
		LevelSetupData levelSetupData;
		levelSetupData.Name = m_NewLevelName->GetText();
		levelSetupData.Size = glm::uvec2(1 << m_NewLevelSizeExp.x, 1 << m_NewLevelSizeExp.y);
		m_Level = std::make_unique<Level>(levelSetupData);
		m_Level->CreateTerrainTree(context.Application);
		OnLevelLoaded(LevelEditorComponent::LevelDirtyFlags::All);
		Logger::Log([&](Logger::Stream& ss) { ss << "Level '" << levelSetupData.Name << "' of size "
			<< levelSetupData.Size.x << "x" << levelSetupData.Size.y << "' has been created."; }, LogSeverity::Info);
	}
}

void LevelEditor::CreateActiveToolGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	auto longLineWidth = 500;

	auto mwStart = glm::vec2(0, c_LevelEditor_MainGUIWindowHeight);
	auto mwSize = glm::vec2(context.WindowSize.x - context.ContentSize.x, c_LevelEditor_ToolGUIWindowHeight);
	auto labelSize = glm::vec2(100, 20);

	if (Nuklear_BeginWindow(ctx, "Tool info", glm::vec2(mwStart.x, mwStart.y), glm::vec2(mwSize.x, mwSize.y)))
	{
		char buffer[64];
		snprintf(buffer, 64, "Current tool: %s", GetCurrentToolName());
		nk_layout_row_static(ctx, c_ButtonSize.y, longLineWidth, 1);
		nk_label(ctx, buffer, NK_TEXT_LEFT);

		UpdateActiveToolInfoString();
		if (!m_ActiveToolInfo.empty())
		{
			char infoBuffer[1024];
			snprintf(infoBuffer, 1024, "Current tool info: %s", m_ActiveToolInfo.c_str());
			nk_layout_row_static(ctx, c_ButtonSize.y, longLineWidth, 1);
			nk_label(ctx, infoBuffer, NK_TEXT_LEFT);
		}
	}
	nk_end(ctx);
}