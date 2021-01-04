// Timeborne/InGame/Model/GameObjects/GameObjectWorkSubsystem.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/GameObjectSubsystem.h>

struct GameObjectCommand;

class GameObjectWorkSubsystem : public GameObjectSubsystem
{
public:
	GameObjectWorkSubsystem(const Level& level);
	~GameObjectWorkSubsystem() override;
	void Tick(const TickContext& context) override;

	void ProcessCommand(const GameObjectCommand& command);
};