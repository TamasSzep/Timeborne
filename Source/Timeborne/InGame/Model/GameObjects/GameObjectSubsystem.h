// Timeborne/InGame/Model/GameObjects/GameObjectSubsystem.h

#pragma once

struct TickContext;
class Level;

class GameObjectSubsystem
{
public:
	GameObjectSubsystem();
	virtual ~GameObjectSubsystem();
	virtual void Tick(const TickContext& context)=0;
};