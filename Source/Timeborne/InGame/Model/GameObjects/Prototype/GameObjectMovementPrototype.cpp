// Timeborne/InGame/Model/GameObjects/Prototype/GameObjectMovementPrototype.cpp

#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectMovementPrototype.h>

bool GameObjectMovementPrototype::IsDynamic() const
{
	return HasFlag(Flags, MovementFlags::Dynamic);
}

void GameObjectMovementPrototype::SetDynamic()
{
	Flags |= MovementFlags::Dynamic;
}
