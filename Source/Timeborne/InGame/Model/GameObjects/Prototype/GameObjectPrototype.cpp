// Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.cpp

#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>

#include <Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/GroundObjectTerrainTreeNodeMapping.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectConstants.h>

GameObjectPrototype::GameObjectPrototype()
{
}

GameObjectPrototype::~GameObjectPrototype()
{
}

GameObjectPrototype::Type GameObjectPrototype::GetType() const
{
	return m_Type;
}

const GameObjectMovementPrototype& GameObjectPrototype::GetMovement() const
{
	return m_Movement;
}

const GameObjectFightPrototype& GameObjectPrototype::GetFight() const
{
	return m_Fight;
}

const glm::vec3& GameObjectPrototype::GetRenderSize() const
{
	return m_RenderSize;
}

const char* GameObjectPrototype::GetLevelEditorName() const
{
	return m_LevelEditorName.c_str();
}

void GameObjectPrototype::_Check() const
{
	assert(m_RenderSize.x <= c_MaxGameObjectXZSize);
	assert(m_RenderSize.z <= c_MaxGameObjectXZSize);
	assert(m_RenderSize.y <= c_MaxGameObjectHeight);
	assert(GroundObjectTerrainTreeNodeMapping::MatchesSamplingCriteria(*this));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Timeborne/InGame/Model/GameObjects/Prototype/Units/TestInfantryUnit.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/Units/TestCar.h>

GameObjectPrototype::Collection GameObjectPrototype::s_Prototypes;
GameObjectPrototype::TypeInitializer GameObjectPrototype::s_TypeInitializer;

template <typename T>
void AddType(GameObjectPrototype::Collection& types, GameObjectTypeIndex typeIndex)
{
	auto index = (uint32_t)types.size();
	types.push_back(std::make_unique<T>());
	auto& prototype = *types.back();
	prototype._SetTypeIndex(typeIndex);
	prototype._Check();
	assert(index == (uint32_t)typeIndex);
}

GameObjectPrototype::TypeInitializer::TypeInitializer()
{
	auto& types = GameObjectPrototype::s_Prototypes;
	AddType<TestInfantryUnit>(types, GameObjectTypeIndex::TestInfantryUnit);
	AddType<TestCar>(types, GameObjectTypeIndex::TestCar);
}

const GameObjectPrototype::Collection& GameObjectPrototype::GetPrototypes()
{
	return s_Prototypes;
}

GameObjectTypeIndex GameObjectPrototype::GetTypeIndex() const
{
	return m_TypeIndex;
}

void GameObjectPrototype::_SetTypeIndex(GameObjectTypeIndex typeIndex)
{
	m_TypeIndex = typeIndex;
}
