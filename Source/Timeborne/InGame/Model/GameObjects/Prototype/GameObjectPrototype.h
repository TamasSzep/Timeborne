// Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectTypeIndex.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectFightPrototype.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectMovementPrototype.h>

#include <Core/Constants.h>
#include <EngineBuildingBlocks/Math/GLM.h>

#include <memory>
#include <vector>

class GameObjectPrototype
{
public:

	using Collection = std::vector<std::unique_ptr<GameObjectPrototype>>;

private:  // Static game object interace.

	class TypeInitializer
	{
	public:
		TypeInitializer();
	};

	static Collection s_Prototypes;
	static TypeInitializer s_TypeInitializer;

	GameObjectTypeIndex m_TypeIndex = (GameObjectTypeIndex)Core::c_InvalidIndexU;

public:

	GameObjectTypeIndex GetTypeIndex() const;
	void _SetTypeIndex(GameObjectTypeIndex typeIndex);

	void _Check() const;

	static const Collection& GetPrototypes();

public:

	enum class Type
	{
		Neutral,
		Unit,
		Building,
		Resource
	};

protected:

	Type m_Type = Type::Neutral;

	GameObjectMovementPrototype m_Movement;
	GameObjectFightPrototype m_Fight;

	// Render size in units.
	glm::vec3 m_RenderSize = glm::vec3(0.0f);

public:

	GameObjectPrototype();
	virtual ~GameObjectPrototype();
	virtual void ExecuteAction() = 0;

	Type GetType() const;
	const GameObjectMovementPrototype& GetMovement() const;
	const GameObjectFightPrototype& GetFight() const;
	const glm::vec3& GetRenderSize() const;

protected: // Command costs.

	unsigned m_ActionCost = 0;

public:

	unsigned GetActionCost() const { return m_ActionCost; }

protected: // Level editor-related functionality.

	std::string m_LevelEditorName;

public:

	const char* GetLevelEditorName() const;
};