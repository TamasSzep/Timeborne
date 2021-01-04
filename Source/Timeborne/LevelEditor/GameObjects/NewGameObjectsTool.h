// Timeborne/LevelEditor/GameObjects/NewGameObjectsTool.h

#pragma once

#include <Timeborne/LevelEditor/LevelEditorComponent.h>
#include <Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorModel.h>

#include <Timeborne/InGame/Model/GameObjects/GameObjectPose.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectTypeIndex.h>

class Nuklear_OnOffButton;

class NewGameObjectsTool : public LevelEditorComponent
                         , public IGameObjectLevelEditorModelListener
{
	GameObjectLevelEditorModel& m_GameObjectModel;

	GameObjectId m_NewObjectId = c_InvalidGameObjectId;

private: // GUI.

	int m_PlayerIndex = 0;
	GameObjectTypeIndex m_TypeIndex = (GameObjectTypeIndex)0;
	bool m_ObjectAppearanceDirty = false;

	int m_YawIndex = 0;
	Core::SimpleTypeVectorU<const char*> m_TypeNames;

	glm::ivec2 m_FieldIndex = glm::ivec2(-1);

	std::unique_ptr<Nuklear_OnOffButton> m_OnOffButton;

	GameObjectPose GetGameObjectPose(GameObjectTypeIndex typeIndex,
		const glm::ivec2& fieldIndex, float yaw);

	void RemoveNewObject();

private: // Input.

	unsigned m_ActionECI = Core::c_InvalidIndexU;
	unsigned m_MouseMoveEventECI = Core::c_InvalidIndexU;

	bool m_CreateNewObject = false;
	glm::vec2 m_NewGameObjectPositionInScreen;

public:

	explicit NewGameObjectsTool(GameObjectLevelEditorModel& model);
	~NewGameObjectsTool() override;

public: // LevelEditorComponent IF.

	void SetActive(bool active) override;

	bool HandleEvent(const EngineBuildingBlocks::Event* _event);
	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;

protected:

	void DerivedInitializeMain(const ComponentInitContext& context) override;

public: // IGameObjectLevelEditorModelListener IF.

	void OnObjectAdded(GameObjectId objectId,
		const GameObjectLevelEditorData& objectData) override {}
	void OnObjectRemoved(GameObjectId objectId) override;
	void OnObjectPoseChanged(GameObjectId objectId,
		const GameObjectPose& pose) override {}
};