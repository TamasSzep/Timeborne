// Timeborne/InGame/Model/GameObjects/Prototype/Units/TestCar.cpp

#include <Timeborne/InGame/Model/GameObjects/Prototype/Units/TestInfantryUnit.h>

#include <Timeborne/Math/Math.h>

TestInfantryUnit::TestInfantryUnit()
{
	m_Type = Type::Unit;

	m_Movement.SetDynamic();
	m_Movement.FlyHeight = 0.1f;
	m_Movement.Speed = 6.0f;
	m_Movement.RotationSpeed = DegreesToRadiaans(360.0f);
	m_Movement.GameLogicSize = glm::vec3(1.0f, 0.8f, 1.0f);
	m_Movement.PositionTerrainNodeMappingCircleRadius = 0.5f;

	m_Fight.MaxHealthPoints = 200;
	m_Fight.GroundAttack.Type = AttackType::Immediate;
	m_Fight.GroundAttack.AnimationDurationMs = 500;
	m_Fight.GroundAttack.ReattackDurationMs = 1000;
	m_Fight.GroundAttack.HitPoints = 50;
	m_Fight.GroundAttack.EnRouteAttacker = false;
	m_Fight.GroundAttack.ApproachDistance.BaseDistance = 8.0f;
	m_Fight.GroundAttack.ApproachDistance.HeightDistanceFactor = 0.25f;
	m_Fight.GroundAttack.ApproachDistance.HeightDistanceMin = -2.0f;
	m_Fight.GroundAttack.ApproachDistance.HeightDistanceMax = 2.0f;
	m_Fight.AirAttack = m_Fight.GroundAttack;

	m_RenderSize = glm::vec3(1.25f, 0.8f, 1.0f);

	m_ActionCost = 500;
	m_LevelEditorName = "Infantry";
}

TestInfantryUnit::~TestInfantryUnit()
{
}

void TestInfantryUnit::ExecuteAction()
{
}
