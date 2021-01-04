// Timeborne/InGame/Model/GameObjects/Prototype/Units/TestInfantryUnit.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>

class TestInfantryUnit : public GameObjectPrototype
{
public:
	TestInfantryUnit();
	~TestInfantryUnit() override;
	void ExecuteAction() override;
};