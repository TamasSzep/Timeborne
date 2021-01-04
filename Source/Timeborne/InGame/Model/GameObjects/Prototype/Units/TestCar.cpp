// Timeborne/InGame/Model/GameObjects/Prototype/Units/TestCar.cpp

#include <Timeborne/InGame/Model/GameObjects/Prototype/Units/TestCar.h>

#include <Timeborne/Math/Math.h>

TestCar::TestCar()
{
	m_Type = Type::Unit;

	m_Movement.SetDynamic();
	m_Movement.Speed = 2.0f;
	m_Movement.RotationSpeed = DegreesToRadiaans(120.0f);
	m_Movement.GameLogicSize = glm::vec3(2.0f, 1.745f, 1.0f);

	m_Fight.MaxHealthPoints = 1000;

	m_RenderSize = glm::vec3(2.0f, 1.745f, 1.0f);

	m_ActionCost = 1000;
	m_LevelEditorName = "Car";
}

TestCar::~TestCar()
{
}

void TestCar::ExecuteAction()
{
}
