// Timeborne/ApplicationComponent.h

#pragma once

#include <Timeborne/Declarations/CoreDeclarations.h>
#include <Timeborne/Declarations/DirectX11RenderDeclarations.h>
#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <EngineBuildingBlocks/Application/PostUpdateContext.h>
#include <EngineBuildingBlocks/Math/GLM.h>

class MainApplication;
struct Settings;

struct ComponentInitContext
{
	MainApplication* Application;

	glm::uvec2 WindowSize;
	glm::uvec2 ContentSize;
};

struct ComponentPreUpdateContext
{
	MainApplication* Application;
	Core::ThreadPool* ThreadPool;

	glm::uvec2 WindowSize;
	glm::uvec2 ContentSize;
};

struct ComponentRenderContext
{
	MainApplication* Application;

	Core::ThreadPool* ThreadPool;

	ID3D11Device* Device;
	ID3D11DeviceContext* DeviceContext;
	ID3D11RenderTargetView* RTV; // Only set on rendering.

	DirectX11Render::Managers* DX11M;
	DirectX11Render::Utilites* DX11U;

	void* NuklearContext; // Only set on rendering.

	const Settings* Settings;

	glm::uvec2 WindowSize;
	glm::uvec2 ContentSize;
};

struct ComponentPostUpdateContext : EngineBuildingBlocks::PostUpdateContext
{
	MainApplication* Application;

	const EngineBuildingBlocks::PathHandler* PathHandler;
	HWND WindowHandle;

	EngineBuildingBlocks::SystemTime* SystemTime;
};

class ApplicationComponent
{
protected:

	MainApplication* m_Application = nullptr;

	virtual void InitializeCommonInputs(const ComponentInitContext& context) {}
	virtual void DerivedInitializeMain(const ComponentInitContext& context) = 0;

public:
	ApplicationComponent(const ApplicationComponent&) = delete;
	ApplicationComponent& operator=(const ApplicationComponent&) = delete;

	ApplicationComponent() {}
	virtual ~ApplicationComponent() {}

	void InitializeMain(const ComponentInitContext& context)
	{
		m_Application = context.Application;
		InitializeCommonInputs(context);
		DerivedInitializeMain(context);
	}

	virtual void InitializeRendering(const ComponentRenderContext& context) = 0;
	virtual void DestroyMain() = 0;
	virtual void DestroyRendering() = 0;
	virtual bool HandleEvent(const EngineBuildingBlocks::Event* _event) = 0;
	virtual void PreUpdate(const ComponentPreUpdateContext& context) = 0;
	virtual void PostUpdate(const ComponentPostUpdateContext& context) = 0;
	virtual void RenderFullscreenClear(const ComponentRenderContext& context) = 0;
	virtual void RenderContent(const ComponentRenderContext& context) = 0;
	virtual void RenderGUI(const ComponentRenderContext& context) = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////

enum class ApplicationScreens
{
	MainMenu = 0,
	SinglePlayer,
	InGame,
	LevelEditor
};

class ApplicationScreen : public ApplicationComponent
{
protected:

	ApplicationScreens m_PreviousScreen = ApplicationScreens::MainMenu;
	ApplicationScreens m_NextScreen = ApplicationScreens::MainMenu;
	bool m_HasConsole = true;

protected: // Input.

	unsigned m_EscapeECI;

	void InitializeCommonInputs(const ComponentInitContext& context) override;

public:

	~ApplicationScreen() override{}

	ApplicationScreens GetNextScreen() const { return m_NextScreen; }
	void SetPreviousScreen(ApplicationScreens screen) { m_PreviousScreen = screen; }
	bool HasConsole() const { return m_HasConsole; }

	virtual void Enter(const ComponentRenderContext& context) = 0;
	virtual void Exit() = 0;
};
