// Timeborne/LevelEditor/LevelEditor.cpp

#include <Timeborne/LevelEditor/LevelEditor.h>

#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorModel.h>
#include <Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorView.h>
#include <Timeborne/LevelEditor/GameObjects/ManageGameObjectsTool.h>
#include <Timeborne/LevelEditor/GameObjects/NewGameObjectsTool.h>
#include <Timeborne/LevelEditor/Terrain/TerrainCopyTool.h>
#include <Timeborne/LevelEditor/Terrain/TerrainLevelEditorView.h>
#include <Timeborne/LevelEditor/Terrain/TerrainSpadeTool.h>
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
	: m_TerrainLevelEditorView(std::make_unique<TerrainLevelEditorView>())
	, m_GameObjectLevelEditorModel(std::make_unique<GameObjectLevelEditorModel>())
	, m_GameObjectLevelEditorView(std::make_unique<GameObjectLevelEditorView>(*m_GameObjectLevelEditorModel))
{
	m_Components.push_back(m_TerrainLevelEditorView.get());
	m_Components.push_back(m_GameObjectLevelEditorModel.get());
	m_Components.push_back(m_GameObjectLevelEditorView.get());
}

LevelEditor::~LevelEditor()
{
}

template <typename T, typename... TArgs>
void AddTool(std::vector<std::unique_ptr<LevelEditorComponent>>& tools,
	LevelEditor::ToolType toolType, TArgs&&... args)
{
	assert((uint32_t)toolType == (uint32_t)tools.size());
	tools.push_back(std::make_unique<T>(std::forward<TArgs>(args)...));
}

void LevelEditor::InitializeMain(const ComponentInitContext& context)
{
	auto application = context.Application;
	assert(application != nullptr);

	auto keyHandler = application->GetKeyHandler();
	auto mouseHandler = application->GetMouseHandler();

	m_CameraSceneNodeHandler = std::make_unique<SceneNodeHandler>();
	m_Camera = std::make_unique<FreeCamera>(m_CameraSceneNodeHandler.get(), keyHandler, mouseHandler);
	DefaultInputBinder::BindFreeCamera(keyHandler, mouseHandler, m_Camera.get());

	auto plusECI = keyHandler->RegisterStateKeyEventListener("LevelEditor.Plus", application);
	auto minusECI = keyHandler->RegisterStateKeyEventListener("LevelEditor.Minus", application);

	keyHandler->BindEventToKey(plusECI, Keys::KeyPadAdd);
	keyHandler->BindEventToKey(minusECI, Keys::KeyPadSubtract);

	auto editDragECI = mouseHandler->RegisterMouseDragEventListener("LevelEditor.EditDrag", application);
	mouseHandler->BindEventToButton(editDragECI, MouseButton::Right);

	LEC_InputData inputData { plusECI, minusECI, editDragECI };

	// Creating tools.
	AddTool<TerrainSpadeTool>(m_Tools, ToolType::TerrainSpade, *m_TerrainLevelEditorView);
	AddTool<TerrainCopyTool>(m_Tools, ToolType::TerrainCopy, *m_TerrainLevelEditorView);
	AddTool<NewGameObjectsTool>(m_Tools, ToolType::NewGameObject, *m_GameObjectLevelEditorModel);
	AddTool<ManageGameObjectsTool>(m_Tools, ToolType::ManageGameObjects, *m_GameObjectLevelEditorModel,
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
	auto tool = GetActiveTool();
	if (tool != nullptr) return tool->HandleEvent(_event);
	return false;
}

bool LevelEditor::TryEscapeActiveTool()
{
	if (m_ActiveToolIndex != Core::c_InvalidIndexU)
	{
		m_Tools[m_ActiveToolIndex]->SetActive(false);
		m_ActiveToolIndex = Core::c_InvalidIndexU;
		return true;
	}
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
	if (m_Level != nullptr)
	{
		for (auto component : m_Components)
		{
			component->RenderGUI(context);
		}
	}
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

const char* LevelEditor::GetActiveToolInfoString()
{
	auto tool = GetActiveTool();
	return (tool != nullptr) ? tool->GetInfoString() : "";
}

LevelEditor::ToolType LevelEditor::GetActiveToolType() const
{
	return (ToolType)m_ActiveToolIndex;
}

void LevelEditor::SetToolActive(ToolType toolType, bool active)
{
	auto toolIndex = (uint32_t)toolType;
	m_Tools[toolIndex]->SetActive(active);
	HandleToolActivityChange(toolIndex);

	// Setting the current tool inactive and adjusting m_ActiveToolIndex is handled in RenderToolGUI(...).
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

void LevelEditor::RenderToolGUI(const ComponentRenderContext& context, ToolType toolType)
{
	auto toolIndex = (uint32_t)toolType;

	m_Tools[toolIndex]->RenderGUI(context);

	HandleToolActivityChange(toolIndex);
}

bool LevelEditor::IsShowingTerrainGrid() const
{
	return m_TerrainLevelEditorView->IsShowingTerrainGrid();
}

void LevelEditor::SetShowingTerrainGrid(bool value)
{
	m_TerrainLevelEditorView->SetShowingTerrainGrid(value);
}

bool LevelEditor::IsRenderingTerrainWithWireframe() const
{
	return m_TerrainLevelEditorView->IsRenderingTerrainWithWireframe();
}

void LevelEditor::SetRenderingTerrainWithWireframe(bool value)
{
	m_TerrainLevelEditorView->SetRenderingTerrainWithWireframe(value);
}

void LevelEditor::CreateNewLevel(MainApplication* application,
	const char* levelName, const glm::uvec2& size)
{
	LevelSetupData levelSetupData;
	levelSetupData.Name = levelName;
	levelSetupData.Size = size;
	m_Level = std::make_unique<Level>(levelSetupData);
	m_Level->CreateTerrainTree(application);
	OnLevelLoaded(LevelEditorComponent::LevelDirtyFlags::All);
	Logger::Log([&](Logger::Stream& ss) { ss << "Level '" << levelSetupData.Name << "' of size "
		<< levelSetupData.Size.x << "x" << levelSetupData.Size.y << "' has been created."; }, LogSeverity::Info);
}

void LevelEditor::LoadLevel(const ComponentPostUpdateContext& context, const std::string& levelName,
	bool isForcingLoadedLevelRecomputations)
{
	assert(context.PathHandler != nullptr);
	auto& pathHandler = *context.PathHandler;

	if (Core::FileExists(Level::GetPath(pathHandler, levelName)))
	{
		m_Level = std::make_unique<Level>();
		m_Level->Load(pathHandler, levelName, isForcingLoadedLevelRecomputations);
		OnLevelLoaded(LevelEditorComponent::LevelDirtyFlags::All);
		LoadLevelMetadata(pathHandler, levelName);
		Logger::Log([&](Logger::Stream& ss) { ss << "Level '" << levelName << "' has been loaded in LevelEditor."; }, LogSeverity::Info);
	}
	else
	{
		Logger::Log([&](Logger::Stream& ss) { ss << "Level '" << levelName << "' does not exist in LevelEditor."; }, LogSeverity::Warning,
			LogFlags::AddMessageBox);
	}
}

void LevelEditor::SaveLevel(const ComponentPostUpdateContext& context, const std::string& levelName)
{
	assert(context.PathHandler != nullptr);
	auto& pathHandler = *context.PathHandler;

	if (m_Level != nullptr)
	{
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