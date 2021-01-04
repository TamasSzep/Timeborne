// Timeborne/InGame/View/GameObjects/GameObjectInGameView.cpp

#include <Timeborne/InGame/View/GameObjects/GameObjectInGameView.h>

#include <Timeborne/GameCreation/GameCreationData.h>
#include <Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/GameObjectTerrainTreeNodeMapping.h>
#include <Timeborne/InGame/Model/GameObjects/GameObjectConstants.h>
#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/Render/GameObjects/GameObjectRenderer.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>
#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>
#include <Timeborne/InGame/Model/Level.h>

GameObjectInGameView::GameObjectInGameView()
	: m_GameObjectRenderer(std::make_unique<GameObjectRenderer>())
{
}

GameObjectInGameView::~GameObjectInGameView()
{
}

void GameObjectInGameView::OnLoading(const ComponentRenderContext& context)
{
	assert(m_Level->GetTerrainTree() != nullptr);

	m_VisibleGameObjects.Clear();

	m_GameObjects.clear();
	m_StaticObjectIds.clear();
	m_DynamicObjectIds.clear();

	m_GameObjectRenderer->ClearObjects();

	m_ObjectNodeMapping
		= std::make_unique<GameObjectTerrainTreeNodeMapping>(*m_Level->GetTerrainTree(), c_GameObjectCullNodeSize);

	m_VisibleTerrainNodeIndices.Clear();
	m_VisibleObjectIds.Clear();
	m_VisibleObjectRendererIndices.Clear();

	m_GameState->GetGameObjects().AddExistenceListenerOnce(*this);
	m_GameState->GetGameObjects().AddPoseListenerOnce(*this);
}

void GameObjectInGameView::OnGameObjectAdded(const GameObject& object)
{
	assert(m_GameCreationData != nullptr);

	const auto& prototype = *GameObjectPrototype::GetPrototypes()[(uint32_t)object.Data.TypeIndex];
	const auto& pose = object.Data.Pose;

	auto& playerData = m_GameCreationData->Players[object.Data.PlayerIndex];

	GameObjectRenderer::ObjectData renderData;
	renderData.TypeIndex = object.Data.TypeIndex;
	renderData.Size = prototype.GetRenderSize();
	renderData.Position = pose.GetWorldPosition();
	renderData.Direction = pose.GetWorldDirection();
	renderData.Up = pose.GetWorldUp();
	renderData.ColorIndex = playerData.LevelEditorIndex;
	renderData.Alpha = 1.0;

	bool isDynamic = prototype.GetMovement().IsDynamic();
	auto& idContainer = isDynamic ? m_DynamicObjectIds : m_StaticObjectIds;
	idContainer.insert(object.Id);

	auto rendererIndex = m_GameObjectRenderer->AddObject(renderData);

	auto transformedBox = m_GameObjectRenderer->GetTransformedBox(rendererIndex);
	m_ObjectNodeMapping->AddObject(object.Id, transformedBox);

	m_GameObjects[object.Id] = { object, rendererIndex, isDynamic };
}

void GameObjectInGameView::OnGameObjectRemoved(GameObjectId objectId)
{
	auto oIt = m_GameObjects.find(objectId);
	assert(oIt != m_GameObjects.end());

	const auto& objectData = oIt->second;
	bool isDynamic = objectData.IsDynamic;
	unsigned rendererIndex = objectData.RendererIndex;

	auto& idContainer = isDynamic ? m_DynamicObjectIds : m_StaticObjectIds;
	idContainer.erase(objectId);

	m_GameObjectRenderer->RemoveObject(rendererIndex);
	m_ObjectNodeMapping->RemoveObject(objectId);

	m_GameObjects.erase(oIt);
}

void GameObjectInGameView::OnGameObjectPoseChanged(GameObjectId objectId, const GameObjectPose& pose)
{
	auto oIt = m_GameObjects.find(objectId);
	assert(oIt != m_GameObjects.end());

	auto& objectData = oIt->second;
	objectData.Object.Data.Pose = pose;

	unsigned rendererIndex = objectData.RendererIndex;
	auto& renderer = *m_GameObjectRenderer;

	UpdateObjectPoseInRenderer(renderer, rendererIndex, pose);
	UpdateObjectNodeMapping(*m_GameObjectRenderer, objectId, rendererIndex);
}

void GameObjectInGameView::Initialize(const ComponentRenderContext& context)
{
	m_GameObjectRenderer->InitializeRendering(context, m_LightingCB, m_RenderPassCB);
}

void GameObjectInGameView::Destroy()
{
	m_GameObjectRenderer->DestroyRendering();
}

void GameObjectInGameView::PreUpdate(const ComponentPreUpdateContext& context)
{
	assert(m_Camera != nullptr);
	auto& camera = *m_Camera;

	UpdateDynamicObjectPoses();

	m_GameObjectRenderer->UpdateTransformations();

	UpdateVisibleTerrainNodeIndices(context, camera);
	UpdateObjectNodeMapping();
	UpdateVisibleObjectIndices();

	m_GameObjectRenderer->PreUpdate(context, camera, m_VisibleObjectRendererIndices);
}

void GameObjectInGameView::RenderContent(const ComponentRenderContext& context)
{
	m_GameObjectRenderer->RenderContent(context);
}

void GameObjectInGameView::UpdateObjectPoseInRenderer(GameObjectRenderer& renderer, unsigned rendererIndex,
	const GameObjectPose& pose)
{
	auto positionInWorld = pose.GetWorldPosition();
	auto directionInWorld = pose.GetWorldDirection();
	auto upInWorld = pose.GetWorldUp();
	renderer.SetObjectPose(rendererIndex, positionInWorld, directionInWorld, upInWorld);
}

void GameObjectInGameView::UpdateDynamicObjectPoses()
{
	auto& renderer = *m_GameObjectRenderer;
	for (auto objectId : m_DynamicObjectIds)
	{
		auto& gameObjectData = m_GameObjects[objectId];
		UpdateObjectPoseInRenderer(renderer, gameObjectData.RendererIndex, gameObjectData.Object.Data.Pose);
	}
}

void GameObjectInGameView::UpdateVisibleTerrainNodeIndices(const ComponentPreUpdateContext& context,
	EngineBuildingBlocks::Graphics::Camera& camera)
{
	if (m_Level == nullptr || m_Level->GetTerrainTree() == nullptr) return;

	TerrainTree::CullParameters parameters{};
	parameters.outputType = TerrainTree::CullOutputType::Node;
	parameters.maxHeightOffset = c_MaxGameObjectBoundingBoxYFromSurface;
	parameters.minNodeSize = c_GameObjectCullNodeSize;
	parameters.maxNodeSize = c_GameObjectCullNodeSize;
	m_Level->GetTerrainTree()->Cull(*context.ThreadPool, camera, m_VisibleTerrainNodeIndices, parameters);
}

void GameObjectInGameView::UpdateObjectNodeMapping(GameObjectRenderer& renderer,
	GameObjectId objectId, unsigned rendererIndex)
{
	auto transformedBox = renderer.GetTransformedBox(rendererIndex);
	m_ObjectNodeMapping->SetObject(objectId, transformedBox);
}

void GameObjectInGameView::UpdateObjectNodeMapping()
{
	auto& renderer = *m_GameObjectRenderer;
	for (auto objectId : m_DynamicObjectIds)
	{
		auto oIt = m_GameObjects.find(objectId);
		assert(oIt != m_GameObjects.end());

		UpdateObjectNodeMapping(renderer, objectId, oIt->second.RendererIndex);
	}
}

void GameObjectInGameView::UpdateVisibleObjectIndices()
{
	m_VisibleObjectIds.Clear();

	// The both nodes in 'm_VisibleTerrainNodeIndices' and 'm_ObjectNodeMapping' have the same size: 'c_GameObjectCullNodeSize',
	// therefore the mappings can be directly taken without any iteration in the terrain node tree.
	auto countNodes = m_VisibleTerrainNodeIndices.GetSize();
	for (unsigned i = 0; i < countNodes; i++)
	{
		auto objectIds = m_ObjectNodeMapping->GetObjectsForNode(m_VisibleTerrainNodeIndices[i]);

		if (objectIds != nullptr) m_VisibleObjectIds.PushBack(*objectIds);
	}

	// Duplicates must be removed.
	m_VisibleObjectIds.SortAndRemoveDuplicates();

	unsigned countVisibleIndices = m_VisibleObjectIds.GetSize();
	m_VisibleGameObjects.Resize(countVisibleIndices);
	m_VisibleObjectRendererIndices.Resize(countVisibleIndices);
	for (unsigned i = 0; i < countVisibleIndices; i++)
	{
		auto objectId = m_VisibleObjectIds[i];

		auto oIt = m_GameObjects.find(objectId);
		assert(oIt != m_GameObjects.end());

		unsigned rendererIndex = oIt->second.RendererIndex;
		auto box = m_GameObjectRenderer->GetTransformedBox(rendererIndex);

		m_VisibleGameObjects[i] = { objectId, box };
		m_VisibleObjectRendererIndices[i] = rendererIndex;
	}

	// Notifying the game object visibility listeners.
	NotifyGameObjectVisibilityChanged();
}

void GameObjectInGameView::RenderGUI(const ComponentRenderContext& context)
{
	m_GameObjectRenderer->RenderGUI(context);
}

void GameObjectInGameView::AddVisibilityListener(GameObjectVisibilityListener& listener)
{
	m_VisibilityListeners.push_back(&listener);
}

EngineBuildingBlocks::Math::AABoundingBox GameObjectInGameView::GetTransformedBox(GameObjectId id)
{
	auto oIt = m_GameObjects.find(id);
	assert(oIt != m_GameObjects.end());
	return m_GameObjectRenderer->GetTransformedBox(oIt->second.RendererIndex);
}
