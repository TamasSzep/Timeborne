// Timeborne/InGame/Model/GameObjects/GameObjectFightSubsystem.cpp

#include <Timeborne/InGame/Model/GameObjects/GameObjectFightSubsystem.h>

#include <Timeborne/GameCreation/GameCreationData.h>
#include <Timeborne/InGame/Controller/GameObjects/GameObjectCommand.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectMovementSubsystem.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/InGame/Model/TickContext.h>
#include <Timeborne/Math/Math.h>

GameObjectFightSubsystem::GameObjectFightSubsystem(const Level& level, const GameCreationData& gameCreationData,
	GameObjectData& gameObjectData, GameObjectMovementSubsystem& movementSubsystem)
	: m_Level(level)
	, m_GameCreationData(gameCreationData)
	, m_GameObjectData(gameObjectData)
	, m_MovementSubsystem(movementSubsystem)
{
	assert(gameObjectData.ClientModelGameState != nullptr);

	gameObjectData.ClientModelGameState->GetGameObjects().AddExistenceListenerOnce(*this);
	gameObjectData.ClientModelGameState->GetRoutes().AddListenerOnce(*this);
}

GameObjectFightSubsystem::~GameObjectFightSubsystem()
{
}

void GameObjectFightSubsystem::InitializeForNewObject(GameObjectFightData& fightData, GameObjectPrototype& prototype)
{
	fightData.HealthPoints = prototype.GetFight().MaxHealthPoints;
}

void GameObjectFightSubsystem::OnGameObjectAdded(const GameObject& object)
{
	if (object.FightIndex != Core::c_InvalidIndexU)
	{
		assert(m_GameObjectData.ClientModelGameState != nullptr);

		auto& fightData = m_GameObjectData.ClientModelGameState->GetFightList()[object.FightIndex];
		if (fightData.AttackState != AttackState::None)
		{
			m_FightingObjects.insert(object.Id);
		}
	}
}

void GameObjectFightSubsystem::OnGameObjectRemoved(GameObjectId objectId)
{
	m_FightingObjects.erase(objectId);
}

void GameObjectFightSubsystem::OnRouteAdded(GameObjectId objectId, const GameObjectRoute& route)
{
	// We handle the event, in which the game object was commanded to follow a non-attack route.

	if (!m_ReactOnRouteEvents) return;

	auto fIt = m_FightingObjects.find(objectId);
	if (fIt != m_FightingObjects.end())
	{
		assert(m_GameObjectData.ClientModelGameState != nullptr);

		auto& gameObjectList = m_GameObjectData.ClientModelGameState->GetGameObjects();
		auto& gameObjects = gameObjectList.Get();
		auto& fightList = m_GameObjectData.ClientModelGameState->GetFightList();

		auto gIt = gameObjects.find(objectId);
		assert(gIt != gameObjects.end());
		const auto& gameObject = gIt->second;

		auto& fightState = fightList[gameObject.FightIndex];
		fightState.AttackState = AttackState::None;
		fightState.AttackTarget = c_InvalidGameObjectId;

		m_FightingObjects.erase(objectId);

		gameObjectList.NotifyFightStateChanged(gameObject, fightState);
	}
}

void GameObjectFightSubsystem::Tick(const TickContext& context)
{
	assert(m_GameObjectData.ClientModelGameState != nullptr);

	m_ReactOnRouteEvents = false;

	auto& prototypes = GameObjectPrototype::GetPrototypes();
	auto& gameObjectList = m_GameObjectData.ClientModelGameState->GetGameObjects();
	auto& gameObjects = gameObjectList.Get();
	auto& fightList = m_GameObjectData.ClientModelGameState->GetFightList();

	m_FightingObjectsToRemove.ClearAndReserveWithGrow((uint32_t)m_FightingObjects.size());
	m_ChangedFightStates.Clear();

	double animTime = (double)context.updateIntervalInMillis * 1e-3;

	for (auto sourceId : m_FightingObjects)
	{
		auto sgIt = gameObjects.find(sourceId);

		// The SOURCE might be destroyed by another game object or another mechanism.
		// VERY IMPORTANT: alone because of this check the game object indices are NOT allowed to be reused.
		if (sgIt == gameObjects.end())
		{
			m_FightingObjectsToRemove.UnsafePushBack(sourceId);
			continue;
		}

		const auto& sourceObject = sgIt->second;
		auto& sourceFightData = fightList[sourceObject.FightIndex];

		assert(sourceFightData.AttackState != AttackState::None && sourceFightData.AttackTarget != c_InvalidGameObjectId);

		auto changeAttackState = [this, &sourceFightData, sourceId](AttackState state) {
			assert(sourceFightData.AttackState != state);
			sourceFightData.AttackState = state;
			if (state == AttackState::None)
			{
				sourceFightData.AttackTarget = c_InvalidGameObjectId;
				m_FightingObjectsToRemove.UnsafePushBack(sourceId);
			}
			m_ChangedFightStates.PushBack(sourceId);
		};

		auto tgIt = gameObjects.find(sourceFightData.AttackTarget);

		// The TARGET might be destroyed by another game object or another mechanism.
		// VERY IMPORTANT: alone because of this check the game object indices are NOT allowed to be reused.
		if (tgIt == gameObjects.end())
		{
			changeAttackState(AttackState::None);
			continue;
		}

		// @todo: currently only considering ground objects here.

		const auto& sourcePrototype = *prototypes[(uint32_t)sourceObject.Data.TypeIndex];
		const auto& sourceFightPrototype = sourcePrototype.GetFight();
		const auto& sourceAttackPrototype = sourceFightPrototype.GroundAttack;

		const auto& targetObject = tgIt->second;

		bool hasRoute = HasRoute(sourceId);

		// Note that the movement subsystem's rest time is not taken into account here, the object gets
		// its full fight time, when the target is approached.

		// Turning is object game object specific and as such, it's handled differently for different object types:
		//
		// - en-route-attackers: e.g. tanks, the orientation wouldn't change, just the tower is rotated.
		// - non-en-route-attackers: e.g. infantry, the orientation will be changed in the movement subsystem
		//   to make sure no collisions occur.
		//
		// Thus the fight subsystem will NEVER change the object's pose directly, neither the position nor the
		// orientation.

		auto& targetFightData = fightList[targetObject.FightIndex];

		double restAnimTime = animTime;
		while (restAnimTime > 0.0)
		{
			if (sourceFightData.AttackState == AttackState::Approach)
			{
				if (hasRoute)
				{
					if (sourceAttackPrototype.EnRouteAttacker)
					{
						// @todo: handle secondary targets.
						break;
					}
					else
					{
						break;
					}
				}
				else
				{
					// Approaching is finished, proceeding to the next state.
					changeAttackState(sourceAttackPrototype.EnRouteAttacker
						? AttackState::Turning : AttackState::Attack);
				}
			}
			else if (sourceFightData.AttackState == AttackState::Turning)
			{
				bool turnReady = TurnAhead(gameObjectList, sourceId, targetObject.Data.Pose, restAnimTime);
				if (turnReady)
				{
					changeAttackState(AttackState::Attack);
				}
			}
			else
			{
				assert(sourceFightData.AttackState == AttackState::Attack);

				if (IsCloseEnoughForAttack(sourceObject, sourceAttackPrototype, targetObject.Data.Pose))
				{
					if (IsTurnedAheadForAttack(sourceObject, sourceAttackPrototype, targetObject.Data.Pose))
					{
						auto hpBefore = targetFightData.HealthPoints;
						Attack(sourceAttackPrototype, targetFightData, restAnimTime);
						auto hpAfter = targetFightData.HealthPoints;

						if (hpAfter == 0)
						{
							changeAttackState(AttackState::None);
							gameObjectList.Remove(sourceFightData.AttackTarget);
							break;
						}
						else if (hpAfter != hpBefore)
						{
							m_ChangedFightStates.PushBack(sourceFightData.AttackTarget);
						}
					}
					else if (sourceAttackPrototype.EnRouteAttacker)
					{
						changeAttackState(AttackState::Turning);
					}
					else
					{
						bool rotationExist = CreateApproachRoute(sourceObject, sourceAttackPrototype,
							targetObject.Data.Pose);
						assert(rotationExist);
						changeAttackState(AttackState::Approach);
						break;
					}
				}
				else
				{
					// @todo: when following is not intended, this must be changed.

					bool routeExists = CreateApproachRoute(sourceObject, sourceAttackPrototype,
						targetObject.Data.Pose);
					if (routeExists)
					{
						changeAttackState(AttackState::Approach);
						break;
					}
					else
					{
						changeAttackState(AttackState::None);
					}
				}
			}
		}
	}

	m_ChangedFightStates.SortAndRemoveDuplicates();
	unsigned countChangedFightStates = m_ChangedFightStates.GetSize();
	for (uint32_t i = 0; i < countChangedFightStates; i++)
	{
		auto gIt = gameObjects.find(m_ChangedFightStates[i]);
		if (gIt != gameObjects.end())
		{
			auto& gameObject = gIt->second;
			gameObjectList.NotifyFightStateChanged(gameObject, fightList[gameObject.FightIndex]);
		}
	}

	unsigned countFightingObjectsToRemove = m_FightingObjectsToRemove.GetSize();
	for (uint32_t i = 0; i < countFightingObjectsToRemove; i++)
	{
		m_FightingObjects.erase(m_FightingObjectsToRemove[i]);
	}

	m_ReactOnRouteEvents = true;
}

void GameObjectFightSubsystem::ProcessCommand(const GameObjectCommand& command)
{
	assert(command.Type == GameObjectCommand::Type::ObjectToObject);
	assert(m_GameObjectData.ClientModelGameState != nullptr);

	m_ReactOnRouteEvents = false;

	auto& prototypes = GameObjectPrototype::GetPrototypes();
	auto& gameObjectList = m_GameObjectData.ClientModelGameState->GetGameObjects();
	auto& gameObjects = gameObjectList.Get();
	auto& fightList = m_GameObjectData.ClientModelGameState->GetFightList();

	auto tgIt = gameObjects.find(command.TargetId);
	assert(tgIt != gameObjects.end());
	auto& targetObject = tgIt->second;

	if (targetObject.FightIndex == Core::c_InvalidIndexU)
	{
		// Cannot attack objects outside of the fight subsystem.
		return;
	}

	// @todo: implement approaching smarter.

	auto countSources = command.SourceIds.GetSize();
	for (uint32_t i = 0; i < countSources; i++)
	{
		auto sourceId = command.SourceIds[i];

		auto sgIt = gameObjects.find(sourceId);
		assert(sgIt != gameObjects.end());
		auto& sourceObject = sgIt->second;

		if (sourceObject.FightIndex == Core::c_InvalidIndexU ||
			m_GameCreationData.Players.AreAllied(sourceObject.Data.PlayerIndex, targetObject.Data.PlayerIndex))
		{
			// Cannot attack with objects outside of the fight subsystem.
			// Cannot attack allies, including the myself.
			continue;
		}

		auto& sourceFightData = fightList[sourceObject.FightIndex];

		if (sourceFightData.AttackTarget == targetObject.Id)
		{
			// Already attacking the same object.
			assert(sourceFightData.AttackState != AttackState::None);
			continue;
		}

		// @todo: currently only considering ground objects here.

		const auto& sourcePrototype = *prototypes[(uint32_t)sourceObject.Data.TypeIndex];
		const auto& sourceFightPrototype = sourcePrototype.GetFight();
		const auto& sourceAttackPrototype = sourceFightPrototype.GroundAttack;

		if (sourceAttackPrototype.Type == AttackType::None)
		{
			continue;
		}

		DeleteRoute(sourceId);

		bool attackStopped = false;
		if (IsCloseEnoughForAttack(sourceObject, sourceAttackPrototype, targetObject.Data.Pose))
		{
			if (IsTurnedAheadForAttack(sourceObject, sourceAttackPrototype, targetObject.Data.Pose))
			{
				sourceFightData.AttackState = AttackState::Attack;
			}
			else if (sourceAttackPrototype.EnRouteAttacker)
			{
				sourceFightData.AttackState = AttackState::Turning;
			}
			else
			{
				bool rotationExist = CreateApproachRoute(sourceObject, sourceAttackPrototype, 
					targetObject.Data.Pose);
				sourceFightData.AttackState = AttackState::Approach;
				assert(rotationExist);
			}
		}
		else
		{
			bool routeExists = CreateApproachRoute(sourceObject, sourceAttackPrototype, targetObject.Data.Pose);
			attackStopped = !routeExists;

			if (attackStopped)
			{
				sourceFightData.AttackState = AttackState::None;
				sourceFightData.AttackTarget = c_InvalidGameObjectId;
				m_FightingObjects.erase(sourceObject.Id);
			}
			else
			{
				sourceFightData.AttackState = AttackState::Approach;
			}
		}

		if (!attackStopped)
		{
			sourceFightData.AttackTarget = targetObject.Id;
			m_FightingObjects.insert(sourceObject.Id);
		}

		gameObjectList.NotifyFightStateChanged(sourceObject, sourceFightData);
	}

	m_ReactOnRouteEvents = true;
}

float GameObjectFightSubsystem::GetMaxDistance(const GameObject& sourceObject,
	const AttackPrototypeData& sourceAttackPData,
	const GameObjectPose& targetPose)
{
	const auto& sourceApproachPData = sourceAttackPData.Approach;

	// We could use here a less surface-dependent height.
	auto getHeight = [this](const GameObjectPose& pose){
		auto position = pose.GetPosition2d();
		auto fieldIndex = pose.GetTerrainFieldIndex();
		auto offset = position - glm::dvec2(fieldIndex);
		return m_Level.GetTerrain().GetHeight(fieldIndex, (float)offset.x, (float)offset.y);
	};

	float heightDiff = getHeight(sourceObject.Data.Pose) - getHeight(targetPose);

	return sourceApproachPData.MaxDistance *
		glm::clamp(1.0f + heightDiff * sourceApproachPData.HeightDistanceFactor,
			sourceApproachPData.HeightDistanceMin,
			sourceApproachPData.HeightDistanceMax);
}

bool GameObjectFightSubsystem::IsCloseEnoughForAttack(const GameObject& sourceObject,
	const AttackPrototypeData& sourceAttackPData, const GameObjectPose& targetPose)
{
	float distance = (float)sourceObject.Data.Pose.GetDistance2d(targetPose);
	float maxDistance = GetMaxDistance(sourceObject, sourceAttackPData, targetPose);
	return (distance <= maxDistance);
}

bool GameObjectFightSubsystem::IsTurnedAheadForAttack(const GameObject& sourceObject,
	const AttackPrototypeData& sourceAttackPData,
	const GameObjectPose& targetPose)
{
	bool result;
	if (sourceAttackPData.EnRouteAttacker)
	{
		// @todo

		// No angle hysteresis is needed here, since it's running in the same update loop as the attack.

		result = true;
	}
	else
	{
		constexpr float c_AngleHysteresis = DegreesToRadiaans(1.0f);

		const auto& sourcePose = sourceObject.Data.Pose;
		auto targetYaw = sourcePose.GetTargetYaw(targetPose.GetPosition2d());
		float angleDiff = GameObjectPose::WrapWithRepeat(targetYaw - sourcePose.GetYaw());
		result = std::abs(angleDiff) <= c_AngleHysteresis;
	}

	return result;
}

bool GameObjectFightSubsystem::HasRoute(GameObjectId sourceObjectId) const
{
	assert(m_GameObjectData.ClientModelGameState != nullptr);
	return m_GameObjectData.ClientModelGameState->GetRoutes().GetRoute(sourceObjectId) != nullptr;
}

void GameObjectFightSubsystem::DeleteRoute(GameObjectId sourceObjectId)
{
	m_MovementSubsystem.DeleteRoute(sourceObjectId);
}

bool GameObjectFightSubsystem::CreateApproachRoute(const GameObject& sourceObject,
	const AttackPrototypeData& sourceAttackPData,
	const GameObjectPose& targetPose)
{
	constexpr float c_DistanceHysteresis = 2.0f;

	float maxDistance = GetMaxDistance(sourceObject, sourceAttackPData, targetPose);
	float approachDistance = std::max(maxDistance - c_DistanceHysteresis, 0.0f);

	auto orientationTarget = sourceAttackPData.EnRouteAttacker ? GameObjectRoute::c_InvalidOrientationTarget
		: targetPose.GetPosition2d();

	// @todo: fix this bug. We only pass the approach distance which is computed based on the max distance
	// in the CURRENT state. However the max distance changes when the heights change. Therefore, it must be
	// reevaluted in an efficient way for each field in the path finding.

	return m_MovementSubsystem.CreateRoute(sourceObject.Id, targetPose.GetTerrainFieldIndex(),
		approachDistance, orientationTarget);
}

bool GameObjectFightSubsystem::TurnAhead(GameObjectList& gameObjectList,
	GameObjectId objectId, const GameObjectPose& targetPose, double& restAnimTime)
{
	// @todo
	return true;
}

void GameObjectFightSubsystem::Attack(const AttackPrototypeData& sourceAttackPData,
	const GameObjectFightData& targetFightData, double& restAnimTime)
{
	// @todo
	restAnimTime = 0.0;
}
