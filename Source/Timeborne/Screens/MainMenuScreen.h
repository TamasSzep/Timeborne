// Timeborne/Screens/MainMenuScreen.h

#pragma once

#include <Timeborne/ApplicationComponent.h>

struct nk_context;

class MainMenuScreen : public ApplicationScreen
{
	bool m_IsExiting = false;

protected:

	void DerivedInitializeMain(const ComponentInitContext& context);

public:

	MainMenuScreen();
	~MainMenuScreen() override;

	void InitializeRendering(const ComponentRenderContext& context) override;
	void DestroyMain() override;
	void DestroyRendering() override;
	bool HandleEvent(const EngineBuildingBlocks::Event* _event) override;
	void PreUpdate(const ComponentPreUpdateContext& context) override;
	void PostUpdate(const ComponentPostUpdateContext& context) override;
	void RenderFullscreenClear(const ComponentRenderContext& context) override;
	void RenderContent(const ComponentRenderContext& context) override;
	void RenderGUI(const ComponentRenderContext& context) override;

	void Enter(const ComponentRenderContext& context) override;
	void Exit() override;

private:
	void Reset();
};