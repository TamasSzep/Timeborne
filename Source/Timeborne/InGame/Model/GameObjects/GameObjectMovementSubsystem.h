// Timeborne/InGame/Model/GameObjects/GameObjectMovementSubsystem.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/GameObjectSubsystem.h>
#include <Timeborne/InGame/Model/GameObjects/GameObject.h>

#include <Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/GroundObjectTerrainTreeNodeMapping.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectRoute.h>

#include <memory>

struct ComponentRenderContext;
struct GameObjectCommand;
struct GameObjectData;
class GameObjectPose;
struct HeightDependentDistanceParameters;
class Level;
class PathFinder;

class GameObjectMovementSubsystem : public GameObjectSubsystem
	, public GameObjectExistenceListener
{
	const Level& m_Level;
	GameObjectData& m_GameObjectData;

	std::unique_ptr<PathFinder> m_PathFinder;

	GroundObjectTerrainTreeNodeMapping m_ObjectToNodeMapping;

	void RemoveFromRoute(GameObjectId objectId, RouteRemoveReason reason);

private: // Temp in Tick(...).

	Core::SimpleTypeVectorU<GameObjectId> m_RoutesToRemove;

public:
	GameObjectMovementSubsystem(const Level& level, GameObjectData& gameObjectData);
	~GameObjectMovementSubsystem() override;

	float GetPathFindingHeight(const GameObjectPose& pose) const;

	bool CreateRoute(GameObjectId objectId, const glm::ivec2& targetField,
		const HeightDependentDistanceParameters* distanceParameters,
		const glm::dvec2& orientationTarget);
	void DeleteRoute(GameObjectId objectId);
	void ProcessCommand(const GameObjectCommand& command);

public: // GameObjectSubsystem IF.

	void Tick(const TickContext& context) override;

public: // GameObjectListener IF.

	void OnGameObjectAdded(const GameObject& object) override;
	void OnGameObjectRemoved(GameObjectId objectId) override;
};