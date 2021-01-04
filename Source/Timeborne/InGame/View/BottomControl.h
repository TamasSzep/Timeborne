// Timeborne/InGame/View/BottomControl.h

#pragma once

#include <Timeborne/InGame/View/InGameViewComponent.h>

#include <Timeborne/InGame/Model/GameObjects/GameObjectId.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/Constants.h>
#include <Core/SingleElementPoolAllocator.hpp>

#include <memory>

struct GameObjectFightData;

class BottomControl : public InGameViewComponent
{
	Core::SimpleTypeVectorU<GameObjectId> m_SourceGameObjectIds;
	std::unique_ptr<Core::FastStdMap<GameObjectId, GameObjectFightData>> m_FightMap;
	
	unsigned m_LastCommandId = Core::c_InvalidIndexU;

	void UpdateSourceGameObjects();
	void UpdateLastCommand();

private: // GUI.

	std::stringstream m_Stream;
	std::string m_SourceObjectStr;
	std::string m_LastCommandStr;

	void UpdateFightMap();
	void UpdateSourceObjectString();
	void UpdateLastCommandString();

public:

	BottomControl();
	~BottomControl() override;

public: // InGameViewComponent IF.

	void OnLoading(const ComponentRenderContext& context) override;
	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;
};