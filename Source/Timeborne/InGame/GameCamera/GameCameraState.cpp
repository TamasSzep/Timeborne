// Timeborne/InGame/GameCamera/GameCameraState.cpp

#include <Timeborne/InGame/GameCamera/GameCameraState.h>

#include <Core/SimpleBinarySerialization.hpp>

GameCameraState::GameCameraState()
{
}

void GameCameraState::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, RotationIndex);
	Core::SerializeSB(bytes, StartAngle);
	Core::SerializeSB(bytes, TargetAngle);
	Core::SerializeSB(bytes, RotationAngle);
	Core::SerializeSB(bytes, Rotating);
	Core::SerializeSB(bytes, RotationAnimationTime);
	Core::SerializeSB(bytes, Core::ToPlaceHolder(SpeedTime));
	Core::SerializeSB(bytes, Core::ToPlaceHolder(LookAt));
	Core::SerializeSB(bytes, Zooming);
	Core::SerializeSB(bytes, ZoomValue);
	Core::SerializeSB(bytes, StartZoom);
	Core::SerializeSB(bytes, TargetZoom);
	Core::SerializeSB(bytes, ZoomAnimationTime);
}

void GameCameraState::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, RotationIndex);
	Core::DeserializeSB(bytes, StartAngle);
	Core::DeserializeSB(bytes, TargetAngle);
	Core::DeserializeSB(bytes, RotationAngle);
	Core::DeserializeSB(bytes, Rotating);
	Core::DeserializeSB(bytes, RotationAnimationTime);
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(SpeedTime));
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(LookAt));
	Core::DeserializeSB(bytes, Zooming);
	Core::DeserializeSB(bytes, ZoomValue);
	Core::DeserializeSB(bytes, StartZoom);
	Core::DeserializeSB(bytes, TargetZoom);
	Core::DeserializeSB(bytes, ZoomAnimationTime);
}
