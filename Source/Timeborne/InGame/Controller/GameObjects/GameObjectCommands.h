// Timeborne/InGame/Controller/GameObjects/GameObjectCommands.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>
#include <Timeborne/InGame/Controller/GameObjects/GameObjectCommand.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <memory>

class CommandList;
struct ComponentPreUpdateContext;
struct GameCreationData;
class Level;
struct LocalGameState;
class MainApplication;
class ServerGameState;

class GameObjectCommands : public GameObjectVisibilityListener
{
	const Level& m_Level;
	const GameCreationData& m_GameCreationData;
	const ServerGameState& m_GameState;
	LocalGameState& m_LocalGameState;
	CommandList& m_CommandList;
	EngineBuildingBlocks::Graphics::Camera& m_Camera;

	Core::SimpleTypeVectorU<GameObjectVisibilityData> m_VisibleGameObjects;

	enum class State
	{
		SourceSelected,
		SelectingSource
	} m_State = State::SourceSelected;

	glm::vec2 m_StartInCS;
	glm::vec3 m_RayOrigin, m_RayDirection;

	Core::SimpleTypeVectorU<GameObjectId> m_NewObjectIds;

	GameObjectCommand m_NewCommand;

	void LoadState();

	void SetSourceObjects();
	void SetLastCommand();

	float IntersectObjectWithRay(const EngineBuildingBlocks::Math::AABoundingBox& box,
		const glm::vec3& rayOrigin, const glm::vec3& rayDirection);

	void SelectOne();
	void SelectMultiple(const glm::vec3& endRayOrigin,
		const glm::vec3& endRayDirection);

	bool IsSourceControllable() const;

	// @todo: implement rectangular lasso for multiple objects.

private: // Input.

	EngineBuildingBlocks::EventManager* m_EventManager = nullptr;

	unsigned m_SelectECI = Core::c_InvalidIndexU;
	unsigned m_ActionECI = Core::c_InvalidIndexU;

	enum class InputType { SelectionActive, SelectionDone, ActionActive };
	struct InputData
	{
		InputType Type;
		glm::vec2 Position;
	};
	Core::SimpleTypeVectorU<InputData> m_Inputs;

	void InitializeInput(MainApplication& application);

public:

	bool HandleEvent(const EngineBuildingBlocks::Event* _event);

public:

	GameObjectCommands(const Level& level, const GameCreationData& gameCreationData,
		const ServerGameState& gameState,
		LocalGameState& localGameState,
		CommandList& commandList,
		GameObjectVisibilityProvider& visibilityProvider,
		EngineBuildingBlocks::Graphics::Camera& camera,
		MainApplication& application, bool fromSaveFile);
	~GameObjectCommands() override;

	void PreUpdate(const ComponentPreUpdateContext& context);

public: // GameObjectVisibilityListener IF.

	void OnGameObjectVisibilityChanged(
		const Core::SimpleTypeVectorU<GameObjectVisibilityData>& visibleGameObjects) override;
};