// Timeborne/InGame/Model/GameObjects/GameObjectFightSubsystem.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/GameObjectSubsystem.h>
#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectRoute.h>

#include <Timeborne/InGame/Model/GameObjects/GameObjectId.h>

struct AttackApproachPrototypeData;
struct AttackPrototypeData;
struct GameCreationData;
struct GameObject;
struct GameObjectCommand;
struct GameObjectData;
struct GameObjectFightData;
class GameObjectList;
class GameObjectMovementSubsystem;
class GameObjectPose;
class GameObjectPrototype;

class GameObjectFightSubsystem : public GameObjectSubsystem
	, public GameObjectExistenceListener
	, public GameObjectRouteListener
{
	const Level& m_Level;
	const GameCreationData& m_GameCreationData;
	GameObjectData& m_GameObjectData;
	GameObjectMovementSubsystem& m_MovementSubsystem;

	Core::FastStdSet<GameObjectId> m_FightingObjects;
	Core::SimpleTypeVectorU<GameObjectId> m_FightingObjectsToRemove;
	Core::SimpleTypeVectorU<GameObjectId> m_ChangedFightStates;

	bool m_ReactOnRouteEvents = true;

	float GetMaxDistance(const GameObject& sourceObject,
		const AttackPrototypeData& sourceAttackPData,
		const GameObjectPose& targetPose);
	bool IsCloseEnoughForAttack(const GameObject& sourceObject,
		const AttackPrototypeData& sourceAttackPData,
		const GameObjectPose& targetPose);

	static bool IsTurnedAheadForAttack(const GameObject& sourceObject,
		const AttackPrototypeData& sourceAttackPData,
		const GameObjectPose& targetPose);

	bool HasRoute(GameObjectId sourceObjectId) const;
	void DeleteRoute(GameObjectId sourceObjectId);
	bool CreateApproachRoute(const GameObject& sourceObject,
		const AttackPrototypeData& sourceAttackPData,
		const GameObjectPose& targetPose);

	bool TurnAhead(GameObjectList& gameObjectList,
		GameObjectId objectId, const GameObjectPose& targetPose,
		double& restAnimTime);
	void Attack(const AttackPrototypeData& sourceAttackPData,
		const GameObjectFightData& targetFightData,
		double& restAnimTime);

public:
	GameObjectFightSubsystem(const Level& level,
		const GameCreationData& gameCreationData,
		GameObjectData& gameObjectData,
		GameObjectMovementSubsystem& movementSubsystem);
	~GameObjectFightSubsystem() override;

	void InitializeForNewObject(GameObjectFightData& fightData,
		GameObjectPrototype& prototype);

	void ProcessCommand(const GameObjectCommand& command);

public: // GameObjectSubsystem IF.

	void Tick(const TickContext& context) override;

public: // GameObjectListener IF.

	void OnGameObjectAdded(const GameObject& object) override;
	void OnGameObjectRemoved(GameObjectId objectId) override;

public: // GameObjectRouteListener IF.

	void OnRouteAdded(GameObjectId objectId,
		const GameObjectRoute& route) override;
	void OnRouteRemoved(GameObjectId objectId,
		RouteRemoveReason reason) {}
};