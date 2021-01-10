// Timeborne/InGame/View/BottomControl.cpp

#include <Timeborne/InGame/View/BottomControl.h>

#include <Timeborne/InGame/GameState/LocalGameState.h>
#include <Timeborne/InGame/GameState/ServerGameState.h>
#include <Timeborne/ApplicationComponent.h>
#include <Timeborne/GUI/NuklearHelper.h>

BottomControl::BottomControl()
{
}

BottomControl::~BottomControl()
{
}

void BottomControl::OnLoading(const ComponentRenderContext& context)
{
	m_SourceGameObjectIds.Clear();
	m_LastCommandId = Core::c_InvalidIndexU;

	m_Stream.str("");
	m_Stream.clear();
	m_SourceObjectStr = "";
	m_LastCommandStr = "";

	m_FightMap = std::make_unique<Core::FastStdMap<GameObjectId, GameObjectFightData>>();

	UpdateSourceGameObjects();
	UpdateLastCommand();
}

void BottomControl::PreUpdate(const ComponentPreUpdateContext& context)
{
	UpdateSourceGameObjects();
	UpdateLastCommand();
}

void BottomControl::UpdateSourceGameObjects()
{
	assert(m_LocalGameState != nullptr && m_FightMap != nullptr);

	const auto& stateSourceGameObjectIds = m_LocalGameState->GetControllerGameState().SourceGameObjectIds;

	bool updateRequired = false;
	if (m_SourceGameObjectIds != stateSourceGameObjectIds)
	{
		m_SourceGameObjectIds = stateSourceGameObjectIds;
		updateRequired = true;	
	}
	else
	{
		assert(m_GameState != nullptr);

		const auto& fightMap = *m_FightMap;

		const auto& gameObjects = m_GameState->GetGameObjects().Get();
		const auto& fightList = m_GameState->GetFightList();

		auto countSourceObjects = m_SourceGameObjectIds.GetSize();
		for (uint32_t i = 0; i < countSourceObjects; i++)
		{
			auto gIt = gameObjects.find(m_SourceGameObjectIds[i]);
			assert(gIt != gameObjects.end());

			auto& sourceObject = gIt->second;

			if (sourceObject.FightIndex == Core::c_InvalidIndexU) continue;

			auto& sourceFightData1 = fightList[sourceObject.FightIndex];

			auto fIt = fightMap.find(sourceObject.Id);
			if (fIt == fightMap.end())
			{
				UpdateFightMap();
				updateRequired = true;
				break;
			}
			auto& sourceFightData2 = fIt->second;

			if (sourceFightData1.HealthPoints != sourceFightData2.HealthPoints
				|| sourceFightData1.AttackTarget != sourceFightData2.AttackTarget
				|| sourceFightData1.AttackState != sourceFightData2.AttackState)
			{
				UpdateFightMap();
				updateRequired = true;
				break;
			}
		}
	}

	if (updateRequired)
	{
		UpdateSourceObjectString();
	}
}

void BottomControl::UpdateFightMap()
{
	auto& fightMap = *m_FightMap;

	fightMap.clear();

	const auto& gameObjects = m_GameState->GetGameObjects().Get();
	const auto& fightList = m_GameState->GetFightList();

	auto countSourceObjects = m_SourceGameObjectIds.GetSize();
	for (uint32_t i = 0; i < countSourceObjects; i++)
	{
		auto gIt = gameObjects.find(m_SourceGameObjectIds[i]);
		assert(gIt != gameObjects.end());

		auto& sourceObject = gIt->second;

		if (sourceObject.FightIndex == Core::c_InvalidIndexU) continue;

		fightMap[sourceObject.Id] = fightList[sourceObject.FightIndex];
	}
}

void BottomControl::UpdateLastCommand()
{
	assert(m_LocalGameState != nullptr);
	auto stateLastCommandId = m_LocalGameState->GetControllerGameState().LastCommandId;

	if (m_LastCommandId != stateLastCommandId)
	{
		m_LastCommandId = stateLastCommandId;
		UpdateLastCommandString();
	}
}

void BottomControl::UpdateSourceObjectString()
{
	auto& fightMap = *m_FightMap;

	const char* attackStateStrs[] = { "-", "->", "X", "T", "RT", "RX" };

	m_Stream.str("");
	m_Stream.clear();
	m_Stream << "Source objects: [";
	unsigned countObjects = m_SourceGameObjectIds.GetSize();
	for (unsigned i = 0; i < countObjects; i++)
	{
		auto objectId = m_SourceGameObjectIds[i];
		m_Stream << (i > 0 ? ", " : "") << (uint32_t)objectId << ": {";

		auto fIt = fightMap.find(objectId);
		if (fIt != fightMap.end())
		{
			auto& sourceFightData = fIt->second;

			m_Stream << " HP: " << sourceFightData.HealthPoints
				<< ", A: " << attackStateStrs[(uint32_t)sourceFightData.AttackState];
			if (sourceFightData.AttackState != AttackState::None)
			{
				m_Stream << " " << (uint32_t)sourceFightData.AttackTarget;
			}
		}

		m_Stream << " }";
	}
	m_Stream << "]";
	m_SourceObjectStr = m_Stream.str();
}

void BottomControl::UpdateLastCommandString()
{
	m_Stream.str("");
	m_Stream.clear();
	m_Stream << "Last command: { ";
	if (m_LastCommandId != Core::c_InvalidIndexU)
	{
		const auto& command = m_LocalGameState->GetControllerGameState().LastGameObjectCommand;
		const char* typeNames[] = { "ObjectToObject", "ObjectToTerrain" };
		m_Stream << "Type: " << typeNames[(int)command.Type] << ", Source objects: [";
		unsigned countSources = command.SourceIds.GetSize();
		for (unsigned i = 0; i < countSources; i++)
		{
			m_Stream << (i > 0 ? ", " : "") << (uint32_t)command.SourceIds[i];
		}
		m_Stream << "], TargetField: (" << command.TargetField.x << ", " << command.TargetField.y << ")"
			<< ", TargetId: ";
		auto targetId = (uint32_t)command.TargetId;
		if (targetId != Core::c_InvalidIndexU)
		{
			m_Stream << (uint32_t)command.TargetId;
		}
		else
		{
			m_Stream << "X";
		}
	}
	m_Stream << " }";
	m_LastCommandStr = m_Stream.str();
}

void BottomControl::RenderGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	// Start and size of the total consolse and command area.
	auto mwStart = glm::vec2(context.WindowSize.x - context.ContentSize.x, context.ContentSize.y);
	auto mwSize = glm::vec2(context.ContentSize.x, context.WindowSize.y - context.ContentSize.y);

	float layoutHeight = 15.0f;

	if (Nuklear_BeginWindow(ctx, nullptr, mwStart, mwSize))
	{
		nk_layout_row_dynamic(ctx, layoutHeight, 1);
		nk_label(ctx, m_SourceObjectStr.c_str(), NK_TEXT_LEFT);
		nk_label(ctx, m_LastCommandStr.c_str(), NK_TEXT_LEFT);
	}
	nk_end(ctx);
}
