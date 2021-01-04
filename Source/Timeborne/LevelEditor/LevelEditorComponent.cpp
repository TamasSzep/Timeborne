// Timeborne/LevelEditor/LevelEditorComponent.cpp

#include <Timeborne/LevelEditor/LevelEditorComponent.h>

#include <Timeborne/LevelEditor/LevelEditor.h>

void LevelEditorComponent::NotifyTerrainHeightsDirty(const glm::ivec2& changeStart, const glm::ivec2& changeEnd)
{
	m_LevelEditor->OnTerrainHeightsDirty(changeStart, changeEnd);
}