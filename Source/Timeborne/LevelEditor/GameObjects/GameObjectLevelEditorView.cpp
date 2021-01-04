// Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorView.cpp

#include <Timeborne/LevelEditor/GameObjects/GameObjectLevelEditorView.h>

#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/Render/GameObjects/GameObjectRenderer.h>

GameObjectLevelEditorView::GameObjectLevelEditorView(GameObjectLevelEditorModel& model)
	: m_GameObjectModel(model)
	, m_GameObjectRenderer(std::make_unique<GameObjectRenderer>())
{
	m_GameObjectModel.AddListener(this);
}

GameObjectLevelEditorView::~GameObjectLevelEditorView()
{
}

unsigned GameObjectLevelEditorView::GetRendererIndex(GameObjectId objectId) const
{
	auto rIt = m_IndexMapping.find(objectId);
	assert(rIt != m_IndexMapping.end());
	return rIt->second;
}

void GameObjectLevelEditorView::OnObjectAdded(GameObjectId objectId, const GameObjectLevelEditorData& objectData)
{
	m_IndicesDirty = true;

	auto& prototypes = GameObjectPrototype::GetPrototypes();
	auto& prototype = *prototypes[(uint32_t)objectData.LevelData.TypeIndex];

	auto& pose = objectData.LevelData.Pose;

	GameObjectRenderer::ObjectData renderData;
	renderData.TypeIndex = objectData.LevelData.TypeIndex;
	renderData.Size = prototype.GetRenderSize();
	renderData.Position = pose.GetWorldPosition();
	renderData.Direction = pose.GetWorldDirection();
	renderData.Up = pose.GetWorldUp();
	renderData.ColorIndex = objectData.LevelData.PlayerIndex;
	renderData.Alpha = objectData.Alpha;

	unsigned rendererIndex = m_GameObjectRenderer->AddObject(renderData);

	assert(m_IndexMapping.find(objectId) == m_IndexMapping.end());
	m_IndexMapping[objectId] = rendererIndex;
}

void GameObjectLevelEditorView::OnObjectRemoved(GameObjectId objectId)
{
	auto rIt = m_IndexMapping.find(objectId);
	assert(rIt != m_IndexMapping.end());

	m_IndicesDirty = true;

	m_GameObjectRenderer->RemoveObject(rIt->second);

	m_IndexMapping.erase(rIt);
}

void GameObjectLevelEditorView::OnObjectPoseChanged(GameObjectId objectId, const GameObjectPose& pose)
{
	m_GameObjectRenderer->SetObjectPose(GetRendererIndex(objectId),
		pose.GetWorldPosition(), pose.GetWorldDirection(), pose.GetWorldUp());
}

void GameObjectLevelEditorView::OnObjectAlphaChanged(GameObjectId objectId, float alpha)
{
	m_GameObjectRenderer->SetObjectAlpha(GetRendererIndex(objectId), alpha);
}

void GameObjectLevelEditorView::InitializeRendering(const ComponentRenderContext& context)
{
	m_GameObjectRenderer->InitializeRendering(context, m_LightingCB, m_RenderPassCB);
}

void GameObjectLevelEditorView::DestroyRendering()
{
	m_GameObjectRenderer->DestroyRendering();
}

void GameObjectLevelEditorView::PreUpdate(const ComponentPreUpdateContext& context)
{
	assert(m_Camera != nullptr);

	if (m_IndicesDirty)
	{
		auto& objectData = m_GameObjectModel.GetObjectData();
		m_AllGameObjectIndices.ClearAndReserve((unsigned)objectData.size());
		for (auto& gameObject : objectData)
		{
			m_AllGameObjectIndices.UnsafePushBack(GetRendererIndex(gameObject.first));
		}
		m_IndicesDirty = false;
	}

	m_GameObjectRenderer->UpdateTransformations();
	m_GameObjectRenderer->PreUpdate(context, *m_Camera, m_AllGameObjectIndices);
}

void GameObjectLevelEditorView::RenderContent(const ComponentRenderContext& context)
{
	m_GameObjectRenderer->RenderContent(context);
}
