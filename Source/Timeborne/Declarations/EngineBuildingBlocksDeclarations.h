// Timeborne/Declarations/EngineBuildingBlocksDeclarations.h

#pragma once

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		// Camera.
		class Camera;
		class FreeCamera;

		// Primitives.
		class ModelLoader;
	}

	namespace Input
	{
		class KeyHandler;
		class MouseHandler;
	}

	namespace Math
	{
		struct AABoundingBox;
		struct Plane;
	}

	struct Event;
	class EventManager;
	class IEventListener;
	class PathHandler;
	class ResourceDatabase;
	class SceneNodeHandler;
	class SystemTime;
}