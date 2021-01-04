// Timeborne/InGame/GameCamera/GameCamera.cpp

#include <Timeborne/InGame/GameCamera/GameCamera.h>

#include <Core/MathHelper.h>
#include <EngineBuildingBlocks/Input/KeyHandler.h>
#include <EngineBuildingBlocks/Input/MouseHandler.h>
#include <EngineBuildingBlocks/ErrorHandling.h>
#include <EngineBuildingBlocks/SystemTime.h>

#include <cmath>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Input;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Rotation.
constexpr float c_RotationAnimationDuration = 0.2f;

// Position.
constexpr float c_MaxSpeed = 5.0f;
constexpr float c_AccellerationTime = 0.1f;
constexpr float c_PositionHeight = 100.0f;

const glm::vec2 c_DefaultLookAt = glm::vec2(10.0f, 10.0f);

// Projection.
constexpr float c_NearPlaneDistance = 0.1f;
constexpr float c_FarPlaneDistance = 10000.0f;

constexpr float c_ZoomFactorMultiplier = 1.0f;
constexpr uint32_t c_CountZoomLevels = 8U;
constexpr uint32_t c_StartZoomLevel = 4U;
constexpr float c_ZoomAnimationDuration = 0.1f;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GameCamera::GameCamera(GameCameraState& state, SceneNodeHandler* sceneNodeHandler, EngineBuildingBlocks::EventManager* eventManager,
	EngineBuildingBlocks::Input::KeyHandler* keyHandler, EngineBuildingBlocks::Input::MouseHandler* mouseHandler,
	const glm::uvec2& contentSize, bool fromSaveFile)
	: Camera(sceneNodeHandler)
	, m_State(state)
	, mEventManager(eventManager)
	, m_AspectRatio(contentSize.x / (float)contentSize.y)
{
	if (!fromSaveFile)
	{
		InitializeState();
	}
	assert(m_State.ZoomValue >= 0.0f);

	SetDirection();
	SetPosition();
	SetProjection();

	InitializeInput(keyHandler, mouseHandler);
}

void GameCamera::InitializeState()
{
	m_State.LookAt = c_DefaultLookAt;

	m_State.RotationAngle = GetAngleForRotationIndex(m_State.RotationIndex);

	m_State.StartZoom = m_State.TargetZoom = m_State.ZoomValue = (float)c_StartZoomLevel;
}

void GameCamera::InitializeInput(EngineBuildingBlocks::Input::KeyHandler* keyHandler,
	EngineBuildingBlocks::Input::MouseHandler* mouseHandler)
{
	// Registering state key events.
	auto stateKeyECIs = keyHandler->RegisterStateKeyEventListener(std::vector<std::string>{ "InGame.RotateLeft", "InGame.RotateRight" }, this);
	m_KeyEventName_RotateLeft_ECI = stateKeyECIs[0];
	m_KeyEventName_RotateRight_ECI = stateKeyECIs[1];

	// Registering timed key events.
	auto timedKeyECIs = keyHandler->RegisterTimedKeyEventListener(
		{ "InGame.MoveUp", "InGame.MoveDown", "InGame.MoveLeft", "InGame.MoveRight" }, this);
	m_KeyEventName_MoveUp_ECI = timedKeyECIs[0];
	m_KeyEventName_MoveDown_ECI = timedKeyECIs[1];
	m_KeyEventName_MoveLeft_ECI = timedKeyECIs[2];
	m_KeyEventName_MoveRight_ECI = timedKeyECIs[3];

	// Registering mouse events.
	m_KeyEventName_Zoom_ECI = mouseHandler->RegisterMouseScrollEventListener("InGame.Scroll", this);

	// Binding the key events.
	keyHandler->BindEventToKey(m_KeyEventName_RotateLeft_ECI, Keys::Q);
	keyHandler->BindEventToKey(m_KeyEventName_RotateRight_ECI, Keys::E);
	keyHandler->BindEventToKey(m_KeyEventName_MoveUp_ECI, Keys::W);
	keyHandler->BindEventToKey(m_KeyEventName_MoveDown_ECI, Keys::S);
	keyHandler->BindEventToKey(m_KeyEventName_MoveLeft_ECI, Keys::A);
	keyHandler->BindEventToKey(m_KeyEventName_MoveRight_ECI, Keys::D);
}

GameCamera::~GameCamera()
{
	mEventManager->UnregisterEventListener(this);
}

void GameCamera::SetActive(bool active)
{
	m_Active = active;
}

bool GameCamera::HandleEvent(const Event* _event)
{
	if (!m_Active) return false;

	bool handled = true;
	auto eci = _event->ClassId;

	if (eci == m_KeyEventName_RotateLeft_ECI) Rotate(false);
	else if (eci == m_KeyEventName_RotateRight_ECI) Rotate(true);
	else if (eci == m_KeyEventName_MoveUp_ECI)
	{
		float dt = static_cast<float>(ToKeyEvent(_event).TotalDownTime);
		m_KeyDownTime.x += dt;
	}
	else if (eci == m_KeyEventName_MoveDown_ECI)
	{
		float dt = static_cast<float>(ToKeyEvent(_event).TotalDownTime);
		m_KeyDownTime.x -= dt;
	}
	else if (eci == m_KeyEventName_MoveLeft_ECI)
	{
		float dt = static_cast<float>(ToKeyEvent(_event).TotalDownTime);
		m_KeyDownTime.y -= dt;
	}
	else if (eci == m_KeyEventName_MoveRight_ECI)
	{
		float dt = static_cast<float>(ToKeyEvent(_event).TotalDownTime);
		m_KeyDownTime.y += dt;
	}
	else if (eci == m_KeyEventName_Zoom_ECI)
	{
		int scrolls = ToMouseScrollEvent(_event).Scrolls;
		Zoom(scrolls);
	}
	else handled = false;

	return handled;
}

void GameCamera::Rotate(bool right)
{
	if (m_State.Rotating) return;
	m_State.Rotating = true;
	m_State.RotationAnimationTime = 0.0f;

	auto prevIndex = m_State.RotationIndex;
	m_State.RotationIndex += (right ? 1 : -1);
	if (m_State.RotationIndex == -1) m_State.RotationIndex = 3;
	if (m_State.RotationIndex == 4) m_State.RotationIndex = 0;

	m_State.StartAngle = GetAngleForRotationIndex(prevIndex);
	m_State.TargetAngle = GetAngleForRotationIndex(m_State.RotationIndex);

	// Handling special cases.
	if (prevIndex == 0 && m_State.RotationIndex == 3) m_State.TargetAngle = GetAngleForRotationIndex(-1);
	else if (prevIndex == 3 && m_State.RotationIndex == 0) m_State.TargetAngle = GetAngleForRotationIndex(4);
}

glm::vec2 GameCamera::GetDirectionXZ()
{
	return { std::cos(m_State.RotationAngle), -std::sin(m_State.RotationAngle) };
}

// SetPosition and SetDirection hard-code the 45 degrees camera angle to the ground.

void GameCamera::SetPosition()
{
	Camera::SetPosition(glm::vec3(m_State.LookAt.x, 0.0f, m_State.LookAt.y)
		+ -GetDirection() * c_PositionHeight * std::sqrt(2.0f));
}

void GameCamera::SetDirection()
{
	auto direction = GetDirectionXZ();
	Camera::SetDirection(glm::vec3(direction.x, -1.0f, direction.y));
}

void GameCamera::UpdateTranslation(double dt)
{
	auto speed = glm::vec2(
		GetSpeed(m_State.SpeedTime.x, m_KeyDownTime.x, (float)dt),
		GetSpeed(m_State.SpeedTime.y, m_KeyDownTime.y, (float)dt));

	m_KeyDownTime = glm::vec2(0.0f);

	auto direction = GetDirectionXZ();
	auto right = glm::vec2(-direction.y, direction.x);

	m_State.LookAt += (speed.x * direction + speed.y * right) * GetZoomFactor() * (float)dt;

	// The position has to be set even if it's not animated, because of a possible change in the orientation.
	SetPosition();
}

float GameCamera::GetSpeed(float& speedTime, float keyDownTime, float dt)
{
	if (keyDownTime == 0.0f) // Either no key pressing or both keys pressed for the exact same (probably whole) time.
	{
		if (speedTime >= 0.0) speedTime = std::max(speedTime - dt, 0.0f);
		else speedTime = std::min(speedTime + dt, 0.0f);
	}
	else
	{
		speedTime = glm::clamp(speedTime + keyDownTime, -c_AccellerationTime, c_AccellerationTime);
	}

	return c_MaxSpeed * speedTime / c_AccellerationTime;
}

void GameCamera::UpdateRotation(double dt)
{
	if (!m_State.Rotating) return;

	m_State.RotationAnimationTime += (float)dt;
	auto factor = glm::smoothstep(0.0f, c_RotationAnimationDuration, m_State.RotationAnimationTime);
	m_State.RotationAngle = glm::lerp(m_State.StartAngle, m_State.TargetAngle, factor);

	SetDirection();

	if (m_State.RotationAnimationTime >= c_RotationAnimationDuration) m_State.Rotating = false;
}

void GameCamera::Update(double dt)
{
	UpdateRotation(dt);
	UpdateTranslation(dt);
	UpdateProjection(dt);
}

float GameCamera::GetAngleForRotationIndex(int index)
{
	constexpr float halfPi = 3.141592653584f * 0.5f;
	return halfPi * (index + 0.5f);
};

void GameCamera::Zoom(int scrolls)
{
	if (m_State.Zooming || scrolls == 0) return;
	
	float targetZoom = glm::clamp(m_State.ZoomValue - scrolls, 0.0f, (float)c_CountZoomLevels - 1U);

	if (targetZoom == m_State.ZoomValue) return;
	
	m_State.Zooming = true;
	m_State.ZoomAnimationTime = 0.0f;

	m_State.StartZoom = m_State.ZoomValue;
	m_State.TargetZoom = targetZoom;
}

float GameCamera::GetZoomFactor() const
{
	return c_ZoomFactorMultiplier * std::pow(2.0f, m_State.ZoomValue);
}

float GameCamera::GetZoomToDefaultFactor() const
{
	return std::pow(2.0f, m_State.ZoomValue - (float)c_StartZoomLevel);
}

void GameCamera::SetProjection()
{
	float halfWidth = GetZoomFactor();

	float right = halfWidth;
	float left = -right;
	float top = halfWidth / m_AspectRatio;
	float bottom = -top;

	Camera::SetOrthographicProjection(left, right, bottom, top, c_NearPlaneDistance, c_FarPlaneDistance, true);
}

void GameCamera::UpdateProjection(double dt)
{
	if (!m_State.Zooming) return;

	m_State.ZoomAnimationTime += (float)dt;
	auto factor = glm::smoothstep(0.0f, c_ZoomAnimationDuration, m_State.ZoomAnimationTime);
	m_State.ZoomValue = glm::lerp(m_State.StartZoom, m_State.TargetZoom, factor);

	SetProjection();

	if (m_State.ZoomAnimationTime >= c_ZoomAnimationDuration) m_State.Zooming = false;
}
