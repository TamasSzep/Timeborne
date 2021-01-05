// Timeborne/InGame/Model/GameObjects/GameObjectSubsystem.h

#pragma once

struct TickContext;
class Level;

class GameObjectSubsystem
{
protected:
	bool m_Loading = false;
public:
	GameObjectSubsystem();
	void SetLoading(bool loading);
	virtual ~GameObjectSubsystem();
	virtual void Tick(const TickContext& context)=0;
};