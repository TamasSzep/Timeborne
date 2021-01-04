// Timeborne/LevelEditor/LevelEditorComponent.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

#include <Core/Constants.h>
#include <Core/Enum.h>

#include <Timeborne/Declarations/DirectX11RenderDeclarations.h>
#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

class Level;
class LevelEditor;

struct nk_context;

struct LEC_InputData
{
	unsigned PlusECI = Core::c_InvalidIndexU;
	unsigned MinusECI = Core::c_InvalidIndexU;

	unsigned EditDragECI = Core::c_InvalidIndexU;
};

class LevelEditorComponent : public ApplicationComponent
{
protected:

	bool m_Active = false;

	Level* m_Level = nullptr;
	LevelEditor* m_LevelEditor = nullptr;

	EngineBuildingBlocks::Graphics::Camera* m_Camera = nullptr;

	LEC_InputData m_InputData;

	std::string m_Name, m_InfoString;

	DirectX11Render::ConstantBuffer* m_RenderPassCB = nullptr;
	DirectX11Render::ConstantBuffer* m_LightingCB = nullptr;

public:

	~LevelEditorComponent() override {}

	bool IsActive() const { return m_Active; }
	virtual void SetActive(bool active) { m_Active = active; }

	const char* GetName() const { return m_Name.c_str(); }
	const char* GetInfoString() const { return m_InfoString.c_str(); }

	void SetLevel(Level* level) { m_Level = level; }
	void SetLevelEditor(LevelEditor* levelEditor) { m_LevelEditor = levelEditor; }
	void SetCamera(EngineBuildingBlocks::Graphics::Camera* camera) { m_Camera = camera; }
	void SetInput(LEC_InputData inputData) { m_InputData = inputData; }
	void SetCBs(DirectX11Render::ConstantBuffer* renderPassCB, DirectX11Render::ConstantBuffer* lightingCB) { m_RenderPassCB = renderPassCB; m_LightingCB = lightingCB; }

	void RenderFullscreenClear(const ComponentRenderContext& context) override {}
	void RenderContent(const ComponentRenderContext& context) override {}
	void RenderGUI(const ComponentRenderContext& context) override {}

	void InitializeRendering(const ComponentRenderContext& context) override {}
	void DestroyMain() override {}
	void DestroyRendering() override {}
	bool HandleEvent(const EngineBuildingBlocks::Event* _event) override { return false; }
	void PreUpdate(const ComponentPreUpdateContext& context) override {}
	void PostUpdate(const ComponentPostUpdateContext& context) override {}

	enum class LevelDirtyFlags
	{
		None = 0x00,

		Terrain = 0x01,
		TerrainTree = 0x02,
		GameObjects = 0x04,

		All = 0xff
	};

	virtual void OnLevelLoaded(LevelDirtyFlags dirtyFlags) {}

	virtual void OnTerrainHeightsDirty(const glm::ivec2& changeStart, const glm::ivec2& changeEnd) {}

protected:

	virtual void DerivedInitializeMain(const ComponentInitContext& initContext) {}

	void NotifyTerrainHeightsDirty(const glm::ivec2& changeStart, const glm::ivec2& changeEnd);
};

UseEnumAsFlagSet(LevelEditorComponent::LevelDirtyFlags)
