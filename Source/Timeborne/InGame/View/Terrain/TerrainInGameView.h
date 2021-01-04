// Timeborne/InGame/View/Terrain/TerrainInGameView.h

#pragma once

#include <Timeborne/InGame/View/InGameViewComponent.h>

#include <memory>

class TerrainCommon;
class TerrainField;
class TerrainWall;

class TerrainInGameView : public InGameViewComponent
{
private: // Terrain.

	bool m_IsShowingTerrainGrid = true;

	std::unique_ptr<TerrainCommon> m_TerrainCommon;
	std::unique_ptr<TerrainField> m_TerrainField;
	std::unique_ptr<TerrainWall> m_TerrainWall;

public:

	TerrainInGameView();
	~TerrainInGameView() override;

	void Initialize(const ComponentRenderContext& context) override;

	void OnLoading(const ComponentRenderContext& context) override;

	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void RenderContent(const ComponentRenderContext& context)  override;
};
