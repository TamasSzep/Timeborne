// Timeborne/Render/Terrain/TerrainField.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

class Level;

class TerrainField
{
private:

	unsigned m_PSIndices[8];

	unsigned& GetPSIndex(bool isInGame, bool wireframe, bool showGrid);

public:

	void InitializeRendering(const ComponentRenderContext& context);
	void Render(const ComponentRenderContext& context, const Level* level,
		int countFieldsToRender,
		bool wireframe, bool showGrid);
};