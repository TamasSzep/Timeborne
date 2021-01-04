// Timeborne/InGame/Model/GameObjects/Prototype/Units/TestCar.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>

class TestCar : public GameObjectPrototype
{
public:
	TestCar();
	~TestCar() override;
	void ExecuteAction() override;
};