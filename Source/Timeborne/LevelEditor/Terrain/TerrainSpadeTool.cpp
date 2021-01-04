// Timeborne/LevelEditor/Terrain/TerrainSpadeTool.cpp

#include <Timeborne/LevelEditor/Terrain/TerrainSpadeTool.h>

#include <Timeborne/GUI/NuklearHelper.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/LevelEditor/Terrain/TerrainFieldBlockHeightIterator.h>
#include <Timeborne/LevelEditor/Terrain/TerrainLevelEditorView.h>
#include <Timeborne/MainApplication.h>

#include <EngineBuildingBlocks/Input/MouseHandler.h>
#include <EngineBuildingBlocks/Math/Intersection.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Input;
using namespace EngineBuildingBlocks::Math;

const int c_MinBlockSize = 1;
const int c_MaxBlockSize = 16;

// @todo: refactorize to input event usage.

TerrainSpadeTool::TerrainSpadeTool(TerrainLevelEditorView& terrainLevelEditorView)
	: m_TerrainLevelEditorView(terrainLevelEditorView)
	, m_BlockSize(4)
{
	m_Name = "terrain spade";

	m_OnOffButton = std::make_unique<Nuklear_OnOffButton>([this](bool on) {
		m_Active = on;
		
		if (on)
		{
			m_Application->SetGUIActive(false);
		}
	});
}

TerrainSpadeTool::~TerrainSpadeTool()
{
}

void TerrainSpadeTool::SetActive(bool active)
{
	m_OnOffButton->SetOn(active); // Calls the handler on change.
}

bool TerrainSpadeTool::HandleEvent(const Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_InputData.EditDragECI)
	{
		m_DragDiff.Update(_event);
		return true;
	}
	else if (eci == m_InputData.PlusECI) m_BlockSize = std::min(m_BlockSize + 1, c_MaxBlockSize);
	else if (eci == m_InputData.MinusECI) m_BlockSize = std::max(m_BlockSize - 1, c_MinBlockSize);
	return false;
}

void TerrainSpadeTool::PreUpdate(const ComponentPreUpdateContext& context)
{
	if (m_Level == nullptr) return;
	
	const float dragDiffDiv = 10.0f;
	float dragDiff = m_DragDiff.RemoveIntegerY(dragDiffDiv);

	auto& terrain = m_Level->GetTerrain();
	auto fields = terrain.GetFields();
	auto countFields = terrain.GetCountFields();

	auto& mouseHandler = *m_Application->GetMouseHandler();

	if (mouseHandler.GetMouseButtonState(MouseButton::Right) == MouseButtonState::Released)
	{
		auto blockSize = glm::ivec2(m_BlockSize);
		switch (m_BlockType)
		{
			case BlockType::Circle:
			{
				blockSize = glm::ivec2(1);
				break;
			}
		}

		m_Intersection = GetTerrainBlockIntersection(m_Camera, context.WindowSize, context.ContentSize,
			m_Level->GetTerrain(), &m_Level->GetFieldHeightQuadtree(), blockSize,
			mouseHandler.GetMouseCursorPosition());

		m_EditedFieldIndices.Clear();
		switch (m_BlockType)
		{
			case BlockType::Square:
			{
				for (int z = m_Intersection.Start.y; z <= m_Intersection.End.y; z++)
				{
					for (int x = m_Intersection.Start.x; x <= m_Intersection.End.x; x++)
					{
						m_EditedFieldIndices.PushBack({ x,z });
					}
				}
				break;
			}
			case BlockType::Circle:
			{
				auto hSize = m_BlockSize / 2;
				auto hSizeSqr = std::pow((float)m_BlockSize * 0.5, 2.0);
				auto start = glm::max(m_Intersection.Start - hSize, 0);
				auto end = glm::min(m_Intersection.Start + hSize, glm::ivec2(countFields) - 1);
				for (int z = start.y; z <= end.y; z++)
				{
					for (int x = start.x; x <= end.x; x++)
					{
						auto diff = glm::ivec2(x, z) - m_Intersection.Start;
						if ((float)(diff.x * diff.x + diff.y * diff.y) <= hSizeSqr)
						{
							m_EditedFieldIndices.PushBack({ x,z });
						}
					}
				}
				m_Intersection.Start = start;
				m_Intersection.End = end;
				if (m_BlockSize > 1)
				{
					m_Intersection.CornerIndex = Core::c_InvalidIndexU;
				}
				break;
			}
		}
	}

	auto hasIntersection = m_Intersection.HasIntersection;
	auto start = m_Intersection.Start;
	auto end = m_Intersection.End;
	auto cornerIndex = m_Intersection.CornerIndex;

	if (hasIntersection && dragDiff != 0.0f)
	{
		float sizeDiffY = dragDiff * 0.5f;

		if (cornerIndex == Core::c_InvalidIndexU)
		{
			float minHeight(1e20f);
			float maxHeight(-1e20f);
			for (TerrainFieldBlockHeightIterator it(&m_EditedFieldIndices, countFields, fields); it; ++it)
			{
				auto height = *it;
				minHeight = std::min(minHeight, height);
				maxHeight = std::max(maxHeight, height);
			}

			if (minHeight == maxHeight)
			{
				for (TerrainFieldBlockHeightIterator it(&m_EditedFieldIndices, countFields, fields); it; ++it)
				{
					*it += sizeDiffY;
				}
			}
			else
			{
				if (sizeDiffY < 0.0f) // Sinking.
				{
					float nextHeight = maxHeight + sizeDiffY;
					for (TerrainFieldBlockHeightIterator it(&m_EditedFieldIndices, countFields, fields); it; ++it)
					{
						*it = std::max(std::min(nextHeight, *it), minHeight);
					}
				}
				else // Elevating.
				{
					float nextHeight = minHeight + sizeDiffY;
					for (TerrainFieldBlockHeightIterator it(&m_EditedFieldIndices, countFields, fields); it; ++it)
					{
						*it = std::min(std::max(nextHeight, *it), maxHeight);
					}
				}
			}
		}
		else
		{
			auto& fieldIndex = m_EditedFieldIndices[0];
			fields[fieldIndex.y * countFields.x + fieldIndex.x].Heights[cornerIndex] += sizeDiffY;
		}

		NotifyTerrainHeightsDirty(start, end);
	}

	// Updating info string.
	{
		char buffer[1024];
		snprintf(buffer, 1024, "start: (%d, %d), end: (%d, %d)", start.x, start.y, end.x, end.y);
		m_InfoString = buffer;
	}
}

void TerrainSpadeTool::RenderContent(const ComponentRenderContext& context)
{
	if (m_Intersection.HasIntersection)
		m_TerrainLevelEditorView.RenderFieldMarkers(context, m_EditedFieldIndices, m_Intersection.CornerIndex,
			glm::vec4(0.0f, 1.0f, 0.0f, 0.5f));
}

void TerrainSpadeTool::RenderGUI(const ComponentRenderContext& context)
{
	auto ctx = (nk_context*)context.NuklearContext;

	nk_layout_row_dynamic(ctx, c_ButtonSize.y, 2);
	nk_label(ctx, "Spade tool", NK_TEXT_LEFT);
	m_OnOffButton->RenderGUI(ctx);

	Nuklear_Slider(ctx, "Block size:", c_MinBlockSize, c_MaxBlockSize, &m_BlockSize);

	if (nk_option_label(ctx, "square", m_BlockType == BlockType::Square)) m_BlockType = BlockType::Square;
	if (nk_option_label(ctx, "circle", m_BlockType == BlockType::Circle)) m_BlockType = BlockType::Circle;
}