// Timeborne/LevelEditor/GameObjects/ManageGameObjectsTool.cpp

#include <Timeborne/LevelEditor/GameObjects/ManageGameObjectsTool.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/LevelEditor/Terrain/TerrainLevelEditorView.h>
#include <Timeborne/MainApplication.h>

using namespace EngineBuildingBlocks::Input;

ManageGameObjectsTool::ManageGameObjectsTool(GameObjectLevelEditorModel& model,
	TerrainLevelEditorView& terrainLevelEditorView)
	: m_GameObjectModel(model)
	, m_TerrainLevelEditorView(terrainLevelEditorView)
{
	m_Name = "game objects - manage objects";

	m_OnOffButton = std::make_unique<Nuklear_OnOffButton>([this](bool on) {
		m_Active = on;

		if (on)
		{
			Reset();
			m_Application->SetGUIActive(false);
			m_CursorPositionInScreen = m_Application->GetMouseHandler()->GetMouseCursorPosition();
		}
		else
		{
			ExitSelection();
		}
	});
}

ManageGameObjectsTool::~ManageGameObjectsTool()
{
}

void ManageGameObjectsTool::Reset()
{
	ResetSelectionState();

	m_DeleteActive = false;
	m_ActionEvents.clear();
}

void ManageGameObjectsTool::ResetSelectionState()
{
	m_SelectionState = SelectionState::SelectingStart;
	m_TerrainStart = glm::ivec2(-1);
	m_TerrainEnd = glm::ivec2(-1);
	m_HasTerrainIntersection = false;
	m_SelectedObjectIds.Clear();
}

void ManageGameObjectsTool::SetSelectedObjectAlpha(float alpha)
{
	auto countObjects = m_SelectedObjectIds.GetSize();
	for (unsigned i = 0; i < countObjects; i++)
	{
		m_GameObjectModel.SetObjectAlpha(m_SelectedObjectIds[i], alpha);
	}
}

void ManageGameObjectsTool::DeleteSelectedObjects()
{
	auto countObjects = m_SelectedObjectIds.GetSize();
	for (unsigned i = 0; i < countObjects; i++)
	{
		m_GameObjectModel.RemoveObject(m_SelectedObjectIds[i]);
	}

	ResetSelectionState();
}

void ManageGameObjectsTool::CreateSelection()
{
	constexpr float c_EditedObjectAlpha = 0.5f;

	glm::ivec2 tStart, tEnd;
	Terrain::GetFlippedLimits(m_TerrainStart, m_TerrainEnd, &tStart, &tEnd);
	m_GameObjectModel.GetObjectIdsForTerrain(tStart, tEnd, m_SelectedObjectIds);

	SetSelectedObjectAlpha(c_EditedObjectAlpha);
}

void ManageGameObjectsTool::ExitSelection()
{
	SetSelectedObjectAlpha(1.0f);
	ResetSelectionState();
}

void ManageGameObjectsTool::SetActive(bool active)
{
	m_OnOffButton->SetOn(active); // Calls the handler on change.
}

void ManageGameObjectsTool::DerivedInitializeMain(const ComponentInitContext& context)
{
	auto keyHandler = context.Application->GetKeyHandler();
	auto mouseHandler = context.Application->GetMouseHandler();

	m_DeleteECI = keyHandler->RegisterStateKeyEventListener("ManageGameObjectsTool.Delete", context.Application);
	keyHandler->BindEventToKey(m_DeleteECI, Keys::Delete);

	// Registering mouse events.
	m_ActionECI = mouseHandler->RegisterMouseButtonEventListener("ManageGameObjectsTool.Action", context.Application,
		true, true);
	m_MouseMoveEventECI = mouseHandler->RegisterMouseMoveEventListener("ManageGameObjectsTool.Move", context.Application);

	// Binding the mouse button event.
	mouseHandler->BindEventToButton(m_ActionECI, MouseButton::Right);
}

bool ManageGameObjectsTool::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	bool handled = false;
	if (eci == m_ActionECI)
	{
		m_ActionEvents.push_back(ToMouseButtonEvent(_event).Pressed);
		handled = true;
	}
	else if (eci == m_MouseMoveEventECI)
	{
		m_CursorPositionInScreen = ToMouseMoveEvent(_event).Position;
		handled = true;
	}
	else if (eci == m_DeleteECI)
	{
		m_DeleteActive = true;
		handled = true;
	}
	return false;
}

void ManageGameObjectsTool::PreUpdate(const ComponentPreUpdateContext& context)
{
	assert(m_Level != nullptr);

	// Intersecting terrain if necessary.
	TerrainBlockIntersection intersection{};
	if (m_SelectionState == SelectionState::SelectingStart || m_SelectionState == SelectionState::SelectingEnd)
	{
		intersection = GetTerrainBlockIntersection(m_Camera, context.WindowSize, context.ContentSize,
			m_Level->GetTerrain(), &m_Level->GetFieldHeightQuadtree(), glm::ivec2(1),
			m_CursorPositionInScreen);
	}
	bool hasIntersection = intersection.HasIntersection;

	// Handling input.
	for (bool pressed : m_ActionEvents)
	{
		if (m_SelectionState == SelectionState::SelectingStart && pressed && hasIntersection)
		{
			m_SelectionState = SelectionState::SelectingEnd;
			m_TerrainStart = intersection.Start;
			m_TerrainEnd = intersection.Start;
		}
		else if (m_SelectionState == SelectionState::SelectingEnd && !pressed)
		{
			if (hasIntersection)
			{
				m_SelectionState = SelectionState::Selected;
				m_TerrainEnd = intersection.Start;

				CreateSelection();
			}
			else
			{
				ResetSelectionState();
			}
		}
		else if (m_SelectionState == SelectionState::Selected && pressed)
		{
			ExitSelection();
		}
	}
	m_ActionEvents.clear();
	if (m_DeleteActive)
	{
		m_DeleteActive = false;
		if (m_SelectionState == SelectionState::Selected)
		{
			DeleteSelectedObjects();
		}
	}

	// Updating the state.
	if (m_SelectionState == SelectionState::SelectingStart)
	{
		m_HasTerrainIntersection = hasIntersection;
		m_TerrainStart = intersection.Start;
		m_TerrainEnd = intersection.Start;
	}
	else if (m_SelectionState == SelectionState::SelectingEnd && hasIntersection)
	{
		m_TerrainEnd = intersection.Start;
	}

	// Updating info string.
	{
		char buffer[1024];
		snprintf(buffer, 1024, "start: (%d, %d), end: (%d, %d)", m_TerrainStart.x, m_TerrainStart.y,
			m_TerrainEnd.x, m_TerrainEnd.y);
		m_InfoString = buffer;
	}
}

void ManageGameObjectsTool::RenderContent(const ComponentRenderContext& context)
{
	glm::ivec2 tStart, tEnd;
	Terrain::GetFlippedLimits(m_TerrainStart, m_TerrainEnd, &tStart, &tEnd);

	if (m_HasTerrainIntersection)
		m_TerrainLevelEditorView.RenderFieldMarkers(context, tStart, tEnd, Core::c_InvalidIndexU, glm::vec4(1.0f, 0.5f, 0.0f, 0.5f));
}

void ManageGameObjectsTool::RenderGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
	m_OnOffButton->RenderGUI(ctx);

	nk_layout_row_static(ctx, c_ButtonSize.y, (int)c_ButtonSize.x, 1);
	bool deletable = (m_SelectionState == SelectionState::Selected) && !m_SelectedObjectIds.IsEmpty();
	if (deletable)
	{
		bool deleteButtonClicked = (nk_button_label(ctx, "Delete selection") != 0);
		if (deleteButtonClicked)
		{
			m_DeleteActive = true;
		}
	}
}
