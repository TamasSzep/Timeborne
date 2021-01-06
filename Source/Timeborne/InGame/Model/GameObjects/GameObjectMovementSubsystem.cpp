// Timeborne/InGame/Model/GameObjects/GameObjectMovementSubsystem.cpp

#include <Timeborne/InGame/Model/GameObjects/GameObjectMovementSubsystem.h>

#include <Timeborne/InGame/Controller/GameObjects/GameObjectCommand.h>
#include <Timeborne/InGame/Model/GameObjects/PathFinding/PathFinder.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectPose.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/InGame/Model/TickContext.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>

GameObjectMovementSubsystem::GameObjectMovementSubsystem(const Level& level, GameObjectData& gameObjectData)
	: m_Level(level)
	, m_GameObjectData(gameObjectData)
	, m_PathFinder(std::make_unique<PathFinder>(level, gameObjectData))
	, m_ObjectToNodeMapping(*level.GetTerrainTree())
{
	assert(gameObjectData.ClientModelGameState != nullptr && level.GetTerrainTree() != nullptr);

	gameObjectData.ClientModelGameState->GetGameObjects().AddExistenceListenerOnce(*this);
}

GameObjectMovementSubsystem::~GameObjectMovementSubsystem()
{
}

float GameObjectMovementSubsystem::GetPathFindingHeight(const GameObjectPose& pose) const
{
	return m_PathFinder->GetPathFindingHeight(pose);
}

bool GameObjectMovementSubsystem::CreateRoute(GameObjectId objectId, const glm::ivec2& targetField,
	const HeightDependentDistanceParameters* distanceParameters, const glm::dvec2& orientationTarget)
{
	assert(m_GameObjectData.ClientModelGameState != nullptr);
	const auto& gameObjectsMap = m_GameObjectData.ClientModelGameState->GetGameObjects().Get();

	auto gIt = gameObjectsMap.find(objectId);
	if (gIt == gameObjectsMap.end()) return false;

	auto& routes = m_GameObjectData.ClientModelGameState->GetRoutes();
	auto& prototypes = GameObjectPrototype::GetPrototypes();

	const auto& gameObject = gIt->second;
	auto& objectPrototype = *prototypes[(uint32_t)gameObject.Data.TypeIndex];

	if (!objectPrototype.GetMovement().IsDynamic()) return false;

	RemoveFromRoute(objectId, RouteRemoveReason::Aborted);

	auto& route = routes.BeginAdd(objectId);

	
	bool pathValid = m_PathFinder->FindPath(objectId, targetField, distanceParameters, route.Path);
	if (pathValid)
	{
		route.OrientationTarget = orientationTarget;
		route.NextFieldIndex = 1U; // Skipping the current field.
		routes.FinishAdd(objectId);
	}
	else
	{
		routes.AbortAdd(objectId);
	}

	return pathValid;
}

void GameObjectMovementSubsystem::DeleteRoute(GameObjectId objectId)
{
	RemoveFromRoute(objectId, RouteRemoveReason::Aborted);
}

void GameObjectMovementSubsystem::ProcessCommand(const GameObjectCommand& command)
{
	assert(command.Type == GameObjectCommand::Type::ObjectToTerrain);

	auto countSources = command.SourceIds.GetSize();
	for (unsigned i = 0; i < countSources; i++)
	{
		CreateRoute(command.SourceIds[i], command.TargetField, nullptr,
			GameObjectRoute::c_InvalidOrientationTarget);
	}
}

void GameObjectMovementSubsystem::RemoveFromRoute(GameObjectId objectId, RouteRemoveReason reason)
{
	assert(m_GameObjectData.ClientModelGameState != nullptr);
	auto& routes = m_GameObjectData.ClientModelGameState->GetRoutes();

	auto route = routes.GetRoutes().Get(objectId);
	if (route != nullptr)
	{
		// Currently a route belongs to a single object.
		assert(route->Path.ObjectId == objectId);
		routes.Remove(objectId, reason);
	}
}

void GameObjectMovementSubsystem::OnGameObjectAdded(const GameObject& object)
{
	m_ObjectToNodeMapping.AddObject(object.Id, object.Data.TypeIndex, object.Data.Pose);
}

void GameObjectMovementSubsystem::OnGameObjectRemoved(GameObjectId objectId)
{
	RemoveFromRoute(objectId, RouteRemoveReason::ObjectRemoved);
	m_ObjectToNodeMapping.RemoveObject(objectId);
}

void GameObjectMovementSubsystem::Tick(const TickContext& context)
{
	assert(m_GameObjectData.ClientModelGameState != nullptr);
	const auto& gameObjectsMap = m_GameObjectData.ClientModelGameState->GetGameObjects().Get();
	auto& routes = m_GameObjectData.ClientModelGameState->GetRoutes();
	auto& routeContainer = routes.GetRoutes().GetElements();

	auto& terrain = m_Level.GetTerrain();

	auto& prototypes = GameObjectPrototype::GetPrototypes();

	double animTime = (double)context.UpdateIntervalInMillis * 1e-3;

	m_RoutesToRemove.ClearAndReserve(routeContainer.GetSize());

	auto pathEnd = routeContainer.GetEndConstIterator();
	for (auto pathIt = routeContainer.GetBeginConstIterator(); pathIt != pathEnd; ++pathIt)
	{
		auto objectId = pathIt->Key;
		auto& pathData = pathIt->Data;

		assert(objectId == pathData.Path.ObjectId);

		auto gIt = gameObjectsMap.find(objectId);
		assert(gIt != gameObjectsMap.end());

		auto& gameObject = gIt->second;
		auto typeIndex = gameObject.Data.TypeIndex;
		auto& prototype = *prototypes[(uint32_t)typeIndex];
		auto& movementPrototype = prototype.GetMovement();

		// Copying the pose.
		auto currentPose = gameObject.Data.Pose;
		auto startPose = currentPose;

		// Copying the next field index.
		auto currentNextFieldIndex = pathData.NextFieldIndex;
		auto startNextFieldIndex = currentNextFieldIndex;

		// Moving object along the path.
		double restAnimTime = animTime;
		bool moving = true;
		bool collisionDetected = false;
		while (restAnimTime > 0.0 && moving && !collisionDetected)
		{
			auto originalPose = currentPose;
			auto originalNextFieldIndex = currentNextFieldIndex;

			auto currentPosition2d = currentPose.GetPosition2d();
			float yaw = currentPose.GetYaw();

			auto rotate = [&yaw, &movementPrototype, &restAnimTime](float targetYaw) {
				float rotationSpeed = movementPrototype.RotationSpeed;
				float angleDiff = GameObjectPose::WrapWithRepeat(targetYaw - yaw);
				double totalTime = (double)(std::abs(angleDiff) / rotationSpeed);

				if (totalTime <= restAnimTime)
				{
					yaw = targetYaw;
					restAnimTime -= totalTime;
				}
				else
				{
					yaw += (angleDiff > 0.0f ? 1.0f : -1.0f) * rotationSpeed * (float)restAnimTime;
					restAnimTime = 0.0;
				}
			};

			if (currentNextFieldIndex >= pathData.Path.Fields.GetSize())
			{
				assert(pathData.IsOrienting());

				float targetYaw = currentPose.GetTargetYaw(pathData.OrientationTarget);
				rotate(targetYaw);
				if (yaw == targetYaw)
				{
					moving = false;
				}
			}
			else
			{
				auto targetFieldIndex = pathData.Path.Fields[currentNextFieldIndex].FieldIndex;
				auto targetPosition2d = GameObjectPose::GetMiddle2dFromTerrainFieldIndex(targetFieldIndex);
				float targetYaw = currentPose.GetTargetYaw(targetPosition2d);

				if (yaw != targetYaw) // Rotating.
				{
					rotate(targetYaw);
				}
				else // Translating.
				{
					double speed = (double)movementPrototype.Speed;
					double distance = currentPose.GetDistance2d(targetPosition2d);
					double totalTime = distance / speed;

					if (totalTime <= restAnimTime)
					{
						currentPosition2d = targetPosition2d;
						restAnimTime -= totalTime;
						if (++currentNextFieldIndex >= pathData.Path.Fields.GetSize() && !pathData.IsOrienting())
						{
							moving = false;
						}
					}
					else
					{
						double drivenDistance = restAnimTime * speed;
						currentPosition2d = currentPose.GetOffsetPosition2d(targetPosition2d, drivenDistance);
						restAnimTime = 0.0;
					}

					float flyHeight = movementPrototype.FlyHeight;
					currentPose.SetPosition(terrain, currentPosition2d, flyHeight);
				}
			}

			currentPose.SetOrientationFromTerrain(terrain, yaw);

			// Checking whether the nodes are not occupied by ANOTHER object.
			// Note that if multiple steps taking place we ensure that as many as possible of them are executed.
			// More fine-granular collision check can be implemented here.
			{
				m_ObjectToNodeMapping.SetObject(objectId, typeIndex, currentPose);

				if (m_ObjectToNodeMapping.IsObjectColliding(objectId, true))
				{
					// Reverting the change.
					currentPose = originalPose;
					currentNextFieldIndex = originalNextFieldIndex;
					moving = true;
					collisionDetected = true;
					m_ObjectToNodeMapping.SetObject(objectId, typeIndex, originalPose);
				}
			}
		}

		// Publising the pose change.
		if (currentPose != startPose)
		{
			m_GameObjectData.ClientModelGameState->GetGameObjects().SetPose(objectId, currentPose);
		}

		// Publishing the next field index change.
		if (currentNextFieldIndex != startNextFieldIndex)
		{
			routes.AccessRoute(objectId).NextFieldIndex = currentNextFieldIndex;
		}

		// Marking the route for remove. This must happen after checking collisions.
		if (!moving)
		{
			m_RoutesToRemove.UnsafePushBack(objectId);
		}
	}

	// Deleting the path if the object has reached the target point or it has stuck.
	unsigned countIndicesToRemove = m_RoutesToRemove.GetSize();
	for (unsigned i = 0; i < countIndicesToRemove; i++)
	{
		routes.Remove(m_RoutesToRemove[i], RouteRemoveReason::Ended);
	}
}
