// Timeborne/InGame/GameState/LocalGameState.cpp

#include <Timeborne/InGame/GameState/LocalGameState.h>

#include <Core/SimpleBinarySerialization.hpp>

LocalGameState::LocalGameState(ServerGameState& modelGameState)
	: m_Statistics(modelGameState)
{
}

LocalGameState::~LocalGameState()
{
}

void LocalGameState::SetGameCreationData(const GameCreationData& data)
{
	m_Statistics.SetGameCreationData(data);
}

const GameCameraState& LocalGameState::GetGameCameraState() const
{
	return m_GameCameraState;
}

GameCameraState& LocalGameState::GetGameCameraState()
{
	return m_GameCameraState;
}

const ControllerGameState& LocalGameState::GetControllerGameState() const
{
	return m_ControllerGameState;
}

ControllerGameState& LocalGameState::GetControllerGameState()
{
	return m_ControllerGameState;
}

const InGameStatistics& LocalGameState::GetStatistics() const
{
	return m_Statistics;
}

void LocalGameState::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, m_GameCameraState);
	Core::SerializeSB(bytes, m_ControllerGameState);
	Core::SerializeSB(bytes, m_Statistics);
}

void LocalGameState::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, m_GameCameraState);
	Core::DeserializeSB(bytes, m_ControllerGameState);
	Core::DeserializeSB(bytes, m_Statistics);
}
