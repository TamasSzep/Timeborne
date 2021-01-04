// Timeborne/InGame/GameCamera/GameCamera.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>
#include <Timeborne/InGame/GameCamera/GameCameraState.h>

#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>
#include <EngineBuildingBlocks/EventHandling.h>

class GameCamera
	: public EngineBuildingBlocks::Graphics::Camera
	, public EngineBuildingBlocks::IEventListener
{
	GameCameraState& m_State;

	void InitializeState();
	void InitializeInput(EngineBuildingBlocks::Input::KeyHandler* keyHandler,
		EngineBuildingBlocks::Input::MouseHandler* mouseHandler);

private: // Rotation.

	void SetDirection();
	void Rotate(bool right);
	void UpdateRotation(double dt);
	static float GetAngleForRotationIndex(int index);

private: // Position.

	glm::vec2 m_KeyDownTime = glm::vec2(0.0f);

	glm::vec2 GetDirectionXZ();

	void SetPosition();
	float GetSpeed(float& speedTime, float keyDownTime, float dt);
	void UpdateTranslation(double dt);

private: // Projection.

	const float m_AspectRatio;

	void Zoom(int scrolls);
	void SetProjection();
	void UpdateProjection(double dt);

public:

	float GetZoomFactor() const;
	float GetZoomToDefaultFactor() const;

public:

	GameCamera(GameCameraState& state,
		EngineBuildingBlocks::SceneNodeHandler* sceneNodeHandler,
		EngineBuildingBlocks::EventManager* eventManager,
		EngineBuildingBlocks::Input::KeyHandler* keyHandler,
		EngineBuildingBlocks::Input::MouseHandler* mouseHandler,
		const glm::uvec2& contentSize, bool fromSaveFile);
	~GameCamera() override;

	void Update(double dt);

private: // Input.

	EngineBuildingBlocks::EventManager* mEventManager;

	bool m_Active = true;
	unsigned m_KeyEventName_MoveUp_ECI;
	unsigned m_KeyEventName_MoveDown_ECI;
	unsigned m_KeyEventName_MoveLeft_ECI;
	unsigned m_KeyEventName_MoveRight_ECI;
	unsigned m_KeyEventName_RotateLeft_ECI;
	unsigned m_KeyEventName_RotateRight_ECI;
	unsigned m_KeyEventName_Zoom_ECI;

public:

	void SetActive(bool active);

	bool HandleEvent(const EngineBuildingBlocks::Event* _event) override;
}; 