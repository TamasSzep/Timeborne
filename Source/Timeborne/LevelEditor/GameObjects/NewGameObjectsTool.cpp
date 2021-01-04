// Timeborne/LevelEditor/GameObjects/NewGameObjectsTool.cpp

#include <Timeborne/LevelEditor/GameObjects/NewGameObjectsTool.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/MainApplication.h>

using namespace EngineBuildingBlocks::Input;

NewGameObjectsTool::NewGameObjectsTool(GameObjectLevelEditorModel& model)
	: m_GameObjectModel(model)
{
	model.AddListener(this);

	m_Name = "game objects - new object";

	for (auto& type : GameObjectPrototype::GetPrototypes()) m_TypeNames.PushBack(type->GetLevelEditorName());

	m_OnOffButton = std::make_unique<Nuklear_OnOffButton>([this](bool on) {
		m_Active = on;

		if (on)
		{
			m_Application->SetGUIActive(false);
			m_NewGameObjectPositionInScreen = m_Application->GetMouseHandler()->GetMouseCursorPosition();
		}
		else
		{
			RemoveNewObject();
		}
	});
}

NewGameObjectsTool::~NewGameObjectsTool()
{
}

void NewGameObjectsTool::OnObjectRemoved(GameObjectId objectId)
{
	if (objectId == m_NewObjectId)
	{
		m_NewObjectId = c_InvalidGameObjectId;
	}
}

void NewGameObjectsTool::SetActive(bool active)
{
	m_OnOffButton->SetOn(active); // Calls the handler on change.
}

void NewGameObjectsTool::DerivedInitializeMain(const ComponentInitContext& context)
{
	auto mouseHandler = context.Application->GetMouseHandler();

	// Registering mouse events.
	m_ActionECI = mouseHandler->RegisterMouseButtonEventListener("NewGameObjectsTool.Action", context.Application,
		true, false);
	m_MouseMoveEventECI = mouseHandler->RegisterMouseMoveEventListener("NewGameObjectsTool.Move", context.Application);

	// Binding the mouse button event.
	mouseHandler->BindEventToButton(m_ActionECI, MouseButton::Right);
}

bool NewGameObjectsTool::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	bool handled = false;
	if (eci == m_ActionECI)
	{
		m_CreateNewObject = true;
		handled = true;
	}
	else if (eci == m_MouseMoveEventECI)
	{
		m_NewGameObjectPositionInScreen = ToMouseMoveEvent(_event).Position;
		handled = true;
	}
	return false;
}

GameObjectPose NewGameObjectsTool::GetGameObjectPose(GameObjectTypeIndex typeIndex,
	const glm::ivec2& fieldIndex, float yaw)
{
	assert(m_Level != nullptr);
	auto& terrain = m_Level->GetTerrain();

	auto& prototype = *GameObjectPrototype::GetPrototypes()[(uint32_t)typeIndex];

	GameObjectPose pose;
	pose.SetPosition(terrain, GameObjectPose::GetMiddle2dFromTerrainFieldIndex(fieldIndex),
		prototype.GetMovement().FlyHeight);
	pose.SetOrientationFromTerrain(terrain, yaw);
	return pose;
}

void NewGameObjectsTool::RemoveNewObject()
{
	if (m_NewObjectId != c_InvalidGameObjectId)
	{
		m_GameObjectModel.RemoveObject(m_NewObjectId);
		assert(m_NewObjectId == c_InvalidGameObjectId); // Set in OnObjectRemoved(...).
	}
}

void NewGameObjectsTool::PreUpdate(const ComponentPreUpdateContext& context)
{
	constexpr float c_NonLevelObjectAlpha = 0.5f;

	glm::ivec2 fieldIndex(-1);

	// Updating the position.
	TerrainBlockIntersection intersection{};
	if (m_Level != nullptr)
	{
		intersection = GetTerrainBlockIntersection(m_Camera, context.WindowSize, context.ContentSize,
			m_Level->GetTerrain(), &m_Level->GetFieldHeightQuadtree(), glm::ivec2(1),
			m_NewGameObjectPositionInScreen);
	}

	if (intersection.HasIntersection)
	{
		if (m_ObjectAppearanceDirty)
		{
			RemoveNewObject();
			m_ObjectAppearanceDirty = false;
		}

		fieldIndex = intersection.Start;
		float yaw = m_YawIndex * glm::pi<float>() * 0.25f;

		auto pose = GetGameObjectPose(m_TypeIndex, fieldIndex, yaw);

		if (m_NewObjectId == c_InvalidGameObjectId)
		{
			GameObjectLevelData objectData;
			objectData.PlayerIndex = m_PlayerIndex;
			objectData.TypeIndex = m_TypeIndex;
			objectData.Pose = pose;

			m_NewObjectId = m_GameObjectModel.AddNonLevelObject(objectData, c_NonLevelObjectAlpha);
		}
		else
		{
			m_GameObjectModel.SetNonLevelObjectPose(m_NewObjectId, pose);
		}
	}
	else
	{
		RemoveNewObject();
	}

	// Updating info string.
	if(fieldIndex != m_FieldIndex)
	{
		char buffer[1024];
		snprintf(buffer, 1024, "(%d, %d)", fieldIndex.x, fieldIndex.y);
		m_InfoString = buffer;
		m_FieldIndex = fieldIndex;
	}

	// Creating objects.
	if (m_CreateNewObject)
	{
		m_CreateNewObject = false;

		if (m_NewObjectId != c_InvalidGameObjectId && m_Level != nullptr)
		{
			GameObjectLevelData newGameObject;
			newGameObject.PlayerIndex = m_PlayerIndex;
			newGameObject.TypeIndex = m_TypeIndex;
			newGameObject.Pose = m_GameObjectModel.GetObjectPose(m_NewObjectId);

			m_GameObjectModel.TryAddLevelObject(newGameObject);
		}
	}
}

void NewGameObjectsTool::RenderContent(const ComponentRenderContext& context)
{
}

void NewGameObjectsTool::RenderGUI(const ComponentRenderContext& context)
{
	int playerIndex = m_PlayerIndex;
	int typeIndex = (int)m_TypeIndex;

	auto ctx = (nk_context*)context.NuklearContext;

	Nuklear_Slider(ctx, "Player index:", 0, 7, &playerIndex);

	nk_layout_row_dynamic(ctx, c_ButtonSize.y, 3);
	nk_label(ctx, "Type:", NK_TEXT_LEFT);
	nk_combobox(ctx, m_TypeNames.GetArray(), (int)m_TypeNames.GetSize(), &typeIndex, (int)c_ButtonSize.y,
		nk_vec2(300, 500));
	nk_label(ctx, "", NK_TEXT_LEFT); // Dummy label to align controls.

	nk_layout_row_dynamic(ctx, c_ButtonSize.y, 3);
	nk_label(ctx, "Direction:", NK_TEXT_LEFT);
	nk_slider_int(ctx, 0, &m_YawIndex, 7, 1);
	const char* directionStrs[] = { "East (0)", "North-east (45)", "North (90)", "North-west (135)",
		"West (180)", "South-west (225)", "South (270)", "South-east (315)" };
	nk_label(ctx, directionStrs[m_YawIndex], NK_TEXT_LEFT);

	nk_layout_row_dynamic(ctx, c_ButtonSize.y, 1);
	m_OnOffButton->RenderGUI(ctx);

	if (playerIndex != m_PlayerIndex || typeIndex != (int)m_TypeIndex)
	{
		m_PlayerIndex = playerIndex;
		m_TypeIndex = (GameObjectTypeIndex)typeIndex;
		m_ObjectAppearanceDirty = true;
	}
}
