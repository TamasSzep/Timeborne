// Timeborne/LevelEditor/Terrain/TerrainCopyTool.cpp

#include <Timeborne/LevelEditor/Terrain/TerrainCopyTool.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/InGame/Model/Terrain/Terrain.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/LevelEditor/Terrain/TerrainFieldBlockHeightIterator.h>
#include <Timeborne/LevelEditor/Terrain/TerrainLevelEditorView.h>
#include <Timeborne/MainApplication.h>

#include <EngineBuildingBlocks/Math/Intersection.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Input;
using namespace EngineBuildingBlocks::Math;

const int c_MinBlockSize = 1;
const int c_MaxBlockSize = 16;

// @todo: refactorize to input event usage.

TerrainCopyTool::TerrainCopyTool(TerrainLevelEditorView& terrainLevelEditorView)
	: m_TerrainLevelEditorView(terrainLevelEditorView)
	, m_HasSourceIntersection(false)
	, m_HasTargetIntersection(false)
{
	m_Name = "terrain copy";

	m_OnOffButton = std::make_unique<Nuklear_OnOffButton>([this](bool on) {
		m_Active = on;
		
		if (on)
		{
			m_Application->SetGUIActive(false);
		}
		else
		{
			m_State = State::Source1;
			ResetTarget();
		}
	});
}

TerrainCopyTool::~TerrainCopyTool()
{
}

void TerrainCopyTool::SetActive(bool active)
{
	m_OnOffButton->SetOn(active); // Calls the handler on change.
}

bool TerrainCopyTool::HandleEvent(const Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_InputData.EditDragECI)
	{
		m_DragDiff.Update(_event);
		return true;
	}
	return false;
}

inline int GetSourceOffset(int sourceStart, int sourceEnd, int targetStart, int targetEnd)
{
	auto ss = sourceEnd - sourceStart;
	auto ts = targetEnd - targetStart;
	if (ss > ts && targetStart == 0) return ss - ts;
	return 0;
}

void TerrainCopyTool::ResetTarget()
{
	m_TargetStart = {};
	m_TargetEnd = {};
}

void TerrainCopyTool::CopyTerrain()
{
	auto& terrain = m_Level->GetTerrain();
	auto fields = terrain.GetFields();
	auto countFields = terrain.GetCountFields();

	glm::ivec2 sourceStart, sourceEnd;
	Terrain::GetFlippedLimits(m_Source1, m_Source2, &sourceStart, &sourceEnd);

	m_CopyBuffer.Clear();
	for (int y = sourceStart.y; y <= sourceEnd.y; y++)
	{
		for (int x = sourceStart.x; x <= sourceEnd.x; x++)
		{
			m_CopyBuffer.PushBack(fields[y * countFields.x + x].Heights);
		}
	}
	auto copyBufferWidth = sourceEnd.x - sourceStart.x + 1;

	int sourceXOffset = GetSourceOffset(sourceStart.x, sourceEnd.x, m_TargetStart.x, m_TargetEnd.x);
	int sourceYOffset = GetSourceOffset(sourceStart.y, sourceEnd.y, m_TargetStart.y, m_TargetEnd.y);

	auto limit = m_TargetEnd - m_TargetStart;
	for (int y = 0; y <= limit.y; y++)
	{
		auto sourceY = y + sourceYOffset;
		auto targetY = m_TargetStart.y + y;
		for (int x = 0; x <= limit.x; x++)
		{
			auto sourceX = x + sourceXOffset;
			auto targetX = m_TargetStart.x + x;
			fields[targetY * countFields.x + targetX].Heights = m_CopyBuffer[sourceY * copyBufferWidth + sourceX];
		}
	}

	NotifyTerrainHeightsDirty(m_TargetStart, m_TargetEnd);
}

void TerrainCopyTool::PreUpdate(const ComponentPreUpdateContext& context)
{
	if (m_Level == nullptr) return;

	auto& terrain = m_Level->GetTerrain();
	auto countFields = terrain.GetCountFields();

	auto& mouseHandler = *m_Application->GetMouseHandler();

	auto GetTerrainBlockIntersectionL = [&](const glm::ivec2& blockSize) {
		return GetTerrainBlockIntersection(m_Camera, context.WindowSize, context.ContentSize, terrain,
			&m_Level->GetFieldHeightQuadtree(), blockSize, mouseHandler.GetMouseCursorPosition());
	};

	bool resetTarget = false;

	switch (m_State)
	{
		case State::Source1:
		{
			m_DragDiff.Clear();
		
			auto intersection = GetTerrainBlockIntersectionL(glm::ivec2(1));
			m_HasSourceIntersection = intersection.HasIntersection;
			m_Source1 = intersection.Start;
			m_Source2 = intersection.End;
		
			if (m_HasSourceIntersection
				&& mouseHandler.GetMouseButtonState(MouseButton::Right) == MouseButtonState::Pressed)
			{
				m_State = State::Source2;
			}
			break;
		}
		case State::Source2:
		{
			auto intersection = GetTerrainBlockIntersectionL(glm::ivec2(1));
			m_Source2 = glm::clamp(intersection.Start, glm::ivec2(0), glm::ivec2(countFields) - 1);

			if (mouseHandler.GetMouseButtonState(MouseButton::Right) == MouseButtonState::Released)
			{
				m_State = State::Target;
			}
			break;
		}
		case State::Target:
		{
			glm::ivec2 sourceStart, sourceEnd;
			Terrain::GetFlippedLimits(m_Source1, m_Source2, &sourceStart, &sourceEnd);
			auto blockSize = sourceEnd - sourceStart + 1;
			auto intersection = GetTerrainBlockIntersectionL(blockSize);
			m_HasTargetIntersection = intersection.HasIntersection;
			m_TargetStart = intersection.Start;
			m_TargetEnd = intersection.End;

			if (m_HasTargetIntersection
				&& mouseHandler.GetMouseButtonState(MouseButton::Right) == MouseButtonState::Pressed)
			{
				CopyTerrain();
				m_State = State::End;
			}
			break;
		}
		case State::End:
		{
			if (m_HasTargetIntersection && mouseHandler.GetMouseButtonState(MouseButton::Right) == MouseButtonState::Released)
			{
				resetTarget = true;
				m_State = State::Source1;
			}
			break;
		}
	}

	// Updating info string.
	{
		char buffer[1024];
		snprintf(buffer, 1024, "[(%d, %d), (%d, %d)] -> [(%d, %d), (%d, %d)]",
			m_Source1.x, m_Source1.y, m_Source2.x, m_Source2.y,
			m_TargetStart.x, m_TargetStart.y, m_TargetEnd.x, m_TargetEnd.y);
		m_InfoString = buffer;
	}

	if (resetTarget)
	{
		ResetTarget();
	}
}

void TerrainCopyTool::RenderContent(const ComponentRenderContext& context)
{
	glm::ivec2 sourceStart, sourceEnd;
	Terrain::GetFlippedLimits(m_Source1, m_Source2, &sourceStart, &sourceEnd);

	if(m_HasSourceIntersection)
		m_TerrainLevelEditorView.RenderFieldMarkers(context, sourceStart, sourceEnd, Core::c_InvalidIndexU,  glm::vec4(0.0f, 0.0f, 1.0f, 0.5f));
	
	if (m_HasTargetIntersection && (int)m_State >= (int)State::Target)
		m_TerrainLevelEditorView.RenderFieldMarkers(context, m_TargetStart, m_TargetEnd, Core::c_InvalidIndexU, glm::vec4(0.0f, 1.0f, 0.0f, 0.5f));
}

void TerrainCopyTool::RenderGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	nk_layout_row_dynamic(ctx, c_ButtonSize.y, 2);
	nk_label(ctx, "Copy tool", NK_TEXT_LEFT);
	m_OnOffButton->RenderGUI(ctx);
}