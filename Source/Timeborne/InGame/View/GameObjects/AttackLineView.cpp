// Timeborne/InGame/View/GameObjects/AttackLineView.cpp

#include <Timeborne/InGame/View/GameObjects/AttackLineView.h>

#include <Timeborne/InGame/GameState/LocalGameState.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>
#include <Timeborne/Render/SimpleLineRenderer.h>

AttackLineView::AttackLineView()
{
}

AttackLineView::~AttackLineView()
{
}

void AttackLineView::Initialize(const ComponentRenderContext& context)
{
	m_LineRenderer = std::make_unique<SimpleLineRenderer>();
	m_LineRenderer->InitializeRendering(context, m_RenderPassCB);
}

void AttackLineView::Destroy()
{
	m_LineRenderer.reset();
}

void AttackLineView::OnLoading(const ComponentRenderContext& context)
{
	m_LineRenderer->Clear();

	m_RendererIndices.clear();
	m_SourceGameObjectIds.Clear();
	m_DirtyIds.Clear();

	auto& gameObjectList = m_GameState->GetGameObjects();
	gameObjectList.AddPoseListenerOnce(*this);
	gameObjectList.AddFightListenerOnce(*this);
}

void AttackLineView::OnGameObjectPoseChanged(GameObjectId objectId, const GameObjectPose& pose)
{
	// If it's a source object, it must be set dirty.
	if (m_SourceGameObjectIds.Contains(objectId))
	{
		m_DirtyIds.PushBack(objectId);
		return;
	}

	// Checking whether it's a target object.
	const auto& gameObjects = m_GameState->GetGameObjects().Get();
	const auto& fightList = m_GameState->GetFightList();

	uint32_t countSourceIds = m_SourceGameObjectIds.GetSize();
	for (uint32_t i = 0; i < countSourceIds; i++)
	{
		auto sourceObjectId = m_SourceGameObjectIds[i];
		auto sgIt = gameObjects.find(sourceObjectId);
		assert(sgIt != gameObjects.end());
		const auto& sourceGameObject = sgIt->second;

		if (sourceGameObject.FightIndex == Core::c_InvalidIndexU) continue;

		const auto& fightData = fightList[sourceGameObject.FightIndex];

		if (objectId == fightData.AttackTarget)
		{
			assert(fightData.AttackState != AttackState::None);
			m_DirtyIds.PushBack(objectId);
			return;
		}
	}
}

void AttackLineView::OnGameObjectFightStateChanged(const GameObject& object, const GameObjectFightData& fightData)
{
	if (m_SourceGameObjectIds.Contains(object.Id))
	{
		m_DirtyIds.PushBack(object.Id);
	}
}

void AttackLineView::UpdateSourceGameObjectIds()
{
	assert(m_LocalGameState != nullptr);
	const auto& stateSourceGameObjectIds = m_LocalGameState->GetControllerGameState().SourceGameObjectIds;

	if (m_SourceGameObjectIds != stateSourceGameObjectIds)
	{
		m_SourceGameObjectIds = stateSourceGameObjectIds;

		// Removing all objects.
		m_LineRenderer->Clear();
		m_RendererIndices.clear();

		m_DirtyIds = m_SourceGameObjectIds;
	}
}

void AttackLineView::UpdateDirtyObjects()
{
	if (m_DirtyIds.IsEmpty()) return;

	const glm::vec3 c_ApproachColor(0.0, 0.5, 1.0);
	const glm::vec3 c_AttackColor(1.0, 0.0, 0.0);
	const glm::vec3 c_TurnColor(0.0, 1.0, 0.0);
	const glm::vec3 c_EnRouteTurnColor(1.0, 1.0, 0.0);
	const glm::vec3 c_EnRouteAttackColor(0.5, 0.0, 0.0);
	const glm::vec3 c_Colors[] = { glm::vec3(), c_ApproachColor, c_AttackColor, c_TurnColor, c_EnRouteTurnColor, c_EnRouteAttackColor };

	const auto& gameObjects = m_GameState->GetGameObjects().Get();
	const auto& fightList = m_GameState->GetFightList();

	uint32_t countDirtyIds = m_DirtyIds.GetSize();
	for (uint32_t i = 0; i < countDirtyIds; i++)
	{
		auto objectId = m_DirtyIds[i];
		auto sgIt = gameObjects.find(objectId);
		assert(sgIt != gameObjects.end());
		const auto& sourceGameObject = sgIt->second;

		if (sourceGameObject.FightIndex == Core::c_InvalidIndexU) continue;

		const auto& fightData = fightList[sourceGameObject.FightIndex];

		bool hasAttackLine = (fightData.AttackState != AttackState::None);

		uint32_t lineIndex;
		auto rIt = m_RendererIndices.find(objectId);
		if (rIt == m_RendererIndices.end())
		{
			if (!hasAttackLine) continue;

			lineIndex = m_LineRenderer->AddLine();
			m_RendererIndices[objectId] = lineIndex;
		}
		else
		{
			lineIndex = rIt->second;

			if (!hasAttackLine)
			{
				m_LineRenderer->RemoveLine(lineIndex);
				m_RendererIndices.erase(rIt);
				continue;
			}
		}

		auto tgIt = gameObjects.find(fightData.AttackTarget);
		assert(tgIt != gameObjects.end());
		const auto& targetGameObject = tgIt->second;

		auto& line = m_LineRenderer->AccessLine(lineIndex);
		line.Color = c_Colors[(uint32_t)fightData.AttackState];
		line.Positions.ClearAndReserve(2);
		line.Positions.UnsafePushBack(sourceGameObject.Data.Pose.GetWorldPosition());
		line.Positions.UnsafePushBack(targetGameObject.Data.Pose.GetWorldPosition());
	}

	m_DirtyIds.Clear();
}

void AttackLineView::PreUpdate(const ComponentPreUpdateContext& context)
{
	// Not reacting on object addition/removal. We only consider changes in the selected object list
	// and when those objects' position or fight state is changed.

	// An object may changed multiple times.
	m_DirtyIds.SortAndRemoveDuplicates();

	// Updating the selection clears the renderer and sets every new object dirty upon the change in the selection.
	UpdateSourceGameObjectIds();

	UpdateDirtyObjects();
}

void AttackLineView::RenderContent(const ComponentRenderContext& context)
{
	m_LineRenderer->RenderContent(context);
}
