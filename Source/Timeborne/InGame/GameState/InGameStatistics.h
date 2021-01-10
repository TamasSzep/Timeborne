// Timeborne/InGame/GameState/InGameStatistics.h

#pragma once

#include <Timeborne/InGame/Model/GameObjects/GameObject.h>

#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <cstdint>
#include <vector>

struct GameCreationData;
class ServerGameState;

class InGameStatistics : public GameObjectFightListener
{
public:

	struct PlayerData
	{
		// Mined resources.
		// ...

		uint32_t ProducedUnits = 0;
		uint32_t ProducedBuildings = 0;

		// First index: of which player.
		Core::IndexVectorU DestroyedUnits;
		Core::IndexVectorU DestroyedBuildings;

		void SerializeSB(Core::ByteVector& bytes) const;
		void DeserializeSB(const unsigned char*& bytes);
	};

private:

	std::vector<PlayerData> m_PlayerData;

	mutable Core::IndexVectorU m_TempIndexVector;

public:

	explicit InGameStatistics(ServerGameState& modelGameState);
	~InGameStatistics() override;

	void SetGameCreationData(const GameCreationData& data);

	const std::vector<PlayerData>& GetPlayerData() const;

	const Core::IndexVectorU& GetLostUnits(uint32_t playerIndex) const;
	const Core::IndexVectorU& GetLostBuildings(uint32_t playerIndex) const;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);

public: // GameObjectFightListener IF.

	void OnGameObjectDestroyed(const GameObject& sourceObject, const GameObject& targetObject) override;
	void OnGameObjectFightStateChanged(const GameObject& object, const GameObjectFightData& fightData) override {}
};
