// Timeborne/InGame/View/GameObjects/PathView.cpp

#include <Timeborne/InGame/View/GameObjects/PathView.h>

#include <Timeborne/InGame/GameState/ServerGameState.h>
#include <Timeborne/InGame/Model/Level.h>
#include <Timeborne/Render/SimpleLineRenderer.h>

PathView::PathView()
{
}

PathView::~PathView()
{
}

void PathView::Initialize(const ComponentRenderContext& context)
{
	m_LineRenderer = std::make_unique<SimpleLineRenderer>();
	m_LineRenderer->InitializeRendering(context, m_RenderPassCB);
}

void PathView::Destroy()
{
	m_LineRenderer.reset();
}

void PathView::OnLoading(const ComponentRenderContext& context)
{
	m_LineRenderer->Clear();

	m_RendererIndices.clear();

	m_GameState->GetRoutes().AddListenerOnce(*this);
}

void PathView::RenderContent(const ComponentRenderContext& context)
{
	m_LineRenderer->RenderContent(context);
}

void PathView::OnRouteAdded(GameObjectId objectId, const GameObjectRoute& route)
{
	const glm::vec3 c_NormalPathColor(0.0, 1.0, 1.0);
	const glm::vec3 c_SingleFieldPathColor(0.0, 0.5, 1.0);

	assert(m_Level != nullptr);
	auto& terrain = m_Level->GetTerrain();

	auto& pathFields = route.Path.Fields;
	auto countPositions = pathFields.GetSize();
	bool isSingleField = (countPositions < 2);

	uint32_t lineIndex = m_LineRenderer->AddLine();
	auto& lineData = m_LineRenderer->AccessLine(lineIndex);
	lineData.Color = isSingleField ? c_SingleFieldPathColor : c_NormalPathColor;

	lineData.Positions.Clear();

	if (isSingleField)
	{
		// Marking the field.
		auto field = route.Path.SourceField;
		lineData.Positions.PushBack(terrain.GetPosition(field, 0.5f, 0.0f));
		lineData.Positions.PushBack(terrain.GetPosition(field, 1.0f, 0.5f));
		lineData.Positions.PushBack(terrain.GetPosition(field, 0.5f, 1.0f));
		lineData.Positions.PushBack(terrain.GetPosition(field, 0.0f, 0.5f));
		lineData.Positions.PushBack(terrain.GetPosition(field, 0.5f, 0.0f));
	}
	else
	{
		for (uint32_t i = 0; i < countPositions; i++)
		{
			lineData.Positions.PushBack(terrain.GetMiddlePosition(pathFields[i].FieldIndex));
		}
	}

	m_RendererIndices[objectId] = lineIndex;
}

void PathView::OnRouteRemoved(GameObjectId objectId, RouteRemoveReason reason)
{
	m_LineRenderer->RemoveLine(m_RendererIndices[objectId]);
}
