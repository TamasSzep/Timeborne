// Timeborne/InGame/Controller/GameObjects/GameObjectCommands.cpp

#include <Timeborne/InGame/Controller/GameObjects/GameObjectCommands.h>

#include <Timeborne/GameCreation/GameCreationData.h>
#include <Timeborne/InGame/Controller/CommandList.h>
#include <Timeborne/InGame/GameState/LocalGameState.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/InputHandling.h>
#include <Timeborne/MainApplication.h>

#include <EngineBuildingBlocks/Math/Intersection.h>
#include <EngineBuildingBlocks/EventHandling.h>

using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Input;
using namespace EngineBuildingBlocks::Math;

constexpr float c_MultipleSelectionThreshold = 0.02f;

const glm::ivec2 c_InvalidTerrainIntersection(-1);

GameObjectCommands::GameObjectCommands(const Level& level, const GameCreationData& gameCreationData,
	ServerGameState& gameState, LocalGameState& localGameState, CommandList& commandList,
	GameObjectVisibilityProvider& visibilityProvider,
	EngineBuildingBlocks::Graphics::Camera& camera,
	MainApplication& application)
	: GameObjectVisibilityListener(visibilityProvider)
	, m_Level(level)
	, m_GameCreationData(gameCreationData)
	, m_GameState(gameState)
	, m_LocalGameState(localGameState)
	, m_CommandList(commandList)
	, m_Camera(camera)
{
	InitializeInput(application);

	gameState.GetGameObjects().AddExistenceListenerOnce(*this);
}

GameObjectCommands::~GameObjectCommands()
{
	m_EventManager->UnregisterEventListenersForEvent(m_SelectECI);
	m_EventManager->UnregisterEventListenersForEvent(m_ActionECI);
}

void GameObjectCommands::InitializeInput(MainApplication& application)
{
	m_EventManager = application.GetEventManager();

	auto mouseHandler = application.GetMouseHandler();

	assert(mouseHandler != nullptr && m_EventManager != nullptr);

	// Registering mouse events.
	m_SelectECI = mouseHandler->RegisterMouseButtonEventListener("GameObjectCommands.Select", &application, true, true);
	m_ActionECI = mouseHandler->RegisterMouseButtonEventListener("GameObjectCommands.Action", &application, true, false);

	// Binding the mouse events.
	mouseHandler->BindEventToButton(m_SelectECI, MouseButton::Left);
	mouseHandler->BindEventToButton(m_ActionECI, MouseButton::Right);
}

bool GameObjectCommands::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	auto eci = _event->ClassId;
	bool handled = false;
	if (eci == m_SelectECI)
	{
		auto& mbEvent = ToMouseButtonEvent(_event);
		m_Inputs.PushBack({ mbEvent.Pressed ? InputType::SelectionActive : InputType::SelectionDone, mbEvent.Position });
		handled = true;
	}
	else if (eci == m_ActionECI)
	{
		auto& mbEvent = ToMouseButtonEvent(_event);
		m_Inputs.PushBack({ InputType::ActionActive, mbEvent.Position });
		handled = true;
	}
	return handled;
}

void GameObjectCommands::SetSourceObjects()
{
	auto& sourceObjectIds = m_LocalGameState.GetControllerGameState().SourceGameObjectIds;

	if (m_NewObjectIds != sourceObjectIds)
	{
		sourceObjectIds = m_NewObjectIds;
	}
}

void GameObjectCommands::SetLastCommand()
{
	auto commandId = m_CommandList.GetLastCommand().CommandId;
	auto& controllerGameState = m_LocalGameState.GetControllerGameState();
	controllerGameState.LastCommandId = commandId;
	controllerGameState.LastGameObjectCommand = m_CommandList.GetGameObjectCommand(commandId);
}

void GameObjectCommands::PreUpdate(const ComponentPreUpdateContext& context)
{
	const auto& sourceObjectIds = m_LocalGameState.GetControllerGameState().SourceGameObjectIds;

	unsigned countInputs = m_Inputs.GetSize();
	for (unsigned i = 0; i < countInputs; i++)
	{
		auto& input = m_Inputs[i];
		if (m_State == State::SourceSelected && input.Type == InputType::SelectionActive)
		{
			bool valid;
			glm::vec3 rayOrigin, rayDirection;
			GetContentCursorRayInWorldCs(context.WindowSize, context.ContentSize, m_Camera, false,
				rayOrigin, rayDirection, valid, input.Position);
			if (valid)
			{
				m_State = State::SelectingSource;
				m_RayOrigin = rayOrigin;
				m_RayDirection = rayDirection;
				m_StartInCS = GetCursorContentCSPosition(context.WindowSize, context.ContentSize, input.Position);
			}
		}
		else if (m_State == State::SelectingSource && input.Type == InputType::SelectionDone)
		{
			bool valid; // Always valid due to clamping.
			glm::vec3 rayOrigin, rayDirection;
			GetContentCursorRayInWorldCs(context.WindowSize, context.ContentSize, m_Camera, true,
				rayOrigin, rayDirection, valid, input.Position);

			auto endInCS = GetCursorContentCSPosition(context.WindowSize, context.ContentSize, input.Position);
			auto absDiffInCs = glm::abs(endInCS - m_StartInCS);
			if (std::max(absDiffInCs.x, absDiffInCs.y) >= c_MultipleSelectionThreshold * 2.0)
			{
				SelectMultiple(rayOrigin, rayDirection);
			}
			else
			{
				SelectOne();
			}
			SetSourceObjects();

			m_State = State::SourceSelected;
		}
		else if (m_State == State::SourceSelected && input.Type == InputType::ActionActive && IsSourceControllable())
		{
			bool valid;
			glm::vec3 rayOrigin, rayDirection;
			GetContentCursorRayInWorldCs(context.WindowSize, context.ContentSize, m_Camera, false,
				rayOrigin, rayDirection, valid, input.Position);
			if (valid)
			{
				m_RayOrigin = rayOrigin;
				m_RayDirection = rayDirection;

				bool hasNewCommand = false;
				auto startCommand = [this, &hasNewCommand, &sourceObjectIds]() {
					m_NewCommand.SourceIds = sourceObjectIds;
					hasNewCommand = true;
				};

				SelectOne();
				if (!m_NewObjectIds.IsEmpty())
				{
					startCommand();
					m_NewCommand.Type = GameObjectCommand::Type::ObjectToObject;
					m_NewCommand.TargetField = { -1, -1 };
					m_NewCommand.TargetId = m_NewObjectIds[0];
				}
				else
				{
					auto tIntersection = GetTerrainBlockIntersection(&m_Camera, context.WindowSize,
						context.ContentSize, m_Level.GetTerrain(), &m_Level.GetFieldHeightQuadtree(), glm::ivec2(1),
						input.Position);
					if (tIntersection.HasIntersection)
					{
						startCommand();
						m_NewCommand.Type = GameObjectCommand::Type::ObjectToTerrain;
						m_NewCommand.TargetField = tIntersection.Start;
						m_NewCommand.TargetId = c_InvalidGameObjectId;
					}
				}

				if (hasNewCommand)
				{
					m_CommandList.AddCommand(m_NewCommand);
					SetLastCommand();
				}
			}
		}
	}
	m_Inputs.Clear();
}

float GameObjectCommands::IntersectObjectWithRay(const EngineBuildingBlocks::Math::AABoundingBox& box,
	const glm::vec3& rayOrigin, const glm::vec3& rayDirection)
{
	return GetRayAABBIntersection_PositiveT(box.Minimum, box.Maximum, rayOrigin, rayDirection);
}

void GameObjectCommands::SelectOne()
{
	m_NewObjectIds.Clear();

	unsigned countVisibleObjects = m_VisibleGameObjects.GetSize();
	float minT = c_InvalidIntersectionT;
	auto closestObjectIndex = c_InvalidGameObjectId;
	for (unsigned i = 0; i < countVisibleObjects; i++)
	{
		const auto& objectData = m_VisibleGameObjects[i];
		float t = IntersectObjectWithRay(objectData.Box, m_RayOrigin, m_RayDirection);
		if (t < minT)
		{
			minT = t;
			closestObjectIndex = objectData.Id;
		}
	}
	if (minT < c_InvalidIntersectionT)
	{
		m_NewObjectIds.PushBack(closestObjectIndex);
	}
}

void GameObjectCommands::SelectMultiple(const glm::vec3& endRayOrigin, const glm::vec3& endRayDirection)
{
	// @todo: implement multiple selection. Make sure that only the OWN objects can be lasso selected.
	m_NewObjectIds.Clear();
}

bool GameObjectCommands::IsSourceControllable() const
{
	const auto& sourceObjectIds = m_LocalGameState.GetControllerGameState().SourceGameObjectIds;

	if (sourceObjectIds.IsEmpty()) return false;

	const auto& gameObjects = m_GameState.GetGameObjects().Get();
	unsigned localPlayerIndex = m_GameCreationData.LocalPlayerIndex;

	// Only controllable if ALL selected objects are controllable.
	unsigned countSelectedObjects = sourceObjectIds.GetSize();
	for (uint32_t i = 0; i < countSelectedObjects; i++)
	{
		auto gIt = gameObjects.find(sourceObjectIds[i]);
		assert(gIt != gameObjects.end());

		if (gIt->second.Data.PlayerIndex != localPlayerIndex) return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GameObjectCommands::OnGameObjectRemoved(GameObjectId objectId)
{
	m_LocalGameState.GetControllerGameState().SourceGameObjectIds.RemoveFirst(objectId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GameObjectCommands::OnGameObjectVisibilityChanged(
	const Core::SimpleTypeVectorU<GameObjectVisibilityData>& visibleGameObjects)
{
	m_VisibleGameObjects = visibleGameObjects;
}
