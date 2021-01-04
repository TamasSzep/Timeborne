// Timeborne/InGame/Model/GameObjects/GameObjectConstants.h

#pragma once

#include <cassert>

// Sizes are defined in field-size.

constexpr float c_MaxGameObjectXZSize = 10.0;
constexpr float c_MaxGameObjectHeight = 5.0;
constexpr float c_MaxGameObjectBoundingBoxYFromSurface = 15.0;

// The cull size is defined in count fields.

constexpr int c_GameObjectCullNodeSize = 16;

static_assert(c_MaxGameObjectXZSize < c_GameObjectCullNodeSize,
	"The game object node size must be greater than the maximum game object size along X and Z "
	"to ensure that 1 game object belongs to maximum 4 nodes.");