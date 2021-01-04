// Timeborne/InGame/Model/GameObjects/GameObjectId.h

#pragma once

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/Constants.h>

#include <cstdint>

// The game object id is an integer that is always incremented and NOT
// reused. Use Core::FastStdMap to store objects indexed with GameObjectId.
struct GameObjectId
{
	// New game objects are only allowed to be created on the server.
	// Having minor lags in object creation and deletion is considered to
	// be acceptable.
	uint32_t Id;

	constexpr GameObjectId() noexcept : Id(0) {}
	constexpr explicit GameObjectId(uint32_t id) noexcept : Id(id) {}

	static GameObjectId MakeId(uint32_t id);

	explicit operator uint32_t() const;
	bool operator==(const GameObjectId& other) const;
	bool operator!=(const GameObjectId& other) const;
	bool operator<(const GameObjectId& other) const;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};

constexpr GameObjectId c_InvalidGameObjectId
	= GameObjectId(Core::c_InvalidIndexU);
