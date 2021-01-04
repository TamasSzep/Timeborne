// Timeborne/MainApplication.h

#pragma once

#include <WindowsApplication/Application.hpp>
#include <Timeborne/Logger.h>

#include <Timeborne/ApplicationComponent.h>
#include <Timeborne/Console.h>
#include <Timeborne/CommandLine.h>
#include <Timeborne/Settings.h>

#include <Core/Windows.h>
#include <Core/DataStructures/Properties.h>
#include <DirectX11Render/DirectX11Includes.h>
#include <DirectX11Render/Resources/Texture2D.h>
#include <DirectX11Render/Managers.h>
#include <DirectX11Render/Utilities/Utilities.h>

#include <atomic>

class MainApplication : public WindowsApplication::Application<MainApplication>
                      , public ILoggerListener
{
	std::vector<ApplicationComponent*> m_Components;
	std::vector<std::unique_ptr<ApplicationScreen>> m_Screens;
	CommandLine m_CommandLine;
	Console m_Console;
	ApplicationScreens m_CurrentScreen;
	Settings m_Settings;

	void InitializeComponents();
	ApplicationScreen* GetCurrentScreen();
	void HandleComponentTransition();

private: // System.

	Core::ThreadPool m_ThreadPool;

private: // Contexts.

	ComponentRenderContext GetComponentRenderContext(void* nuklearContext,
		ID3D11RenderTargetView* rtv);

public: // Interface for screens.

	ApplicationScreen* GetScreen(ApplicationScreens screen);

private: // Configuration.

	Core::Properties m_Configuration;

	std::string m_Title;

	glm::uvec2 m_WindowSize = {0, 0};
	glm::uvec2 m_ContentSize = {0, 0};
	unsigned m_MultisampleCount = 0;
	int m_FontSize = 0;
	unsigned m_MouseAntiPrellFilterTimeout = 0;

	unsigned m_BackbufferFrameCount = 0;
	bool m_IsWindowed = false;

	void LoadConfiguration();

private: // Event listening.

	std::atomic<bool> m_IsGUIActive = false;
	std::atomic<bool> m_AllowGUIActiveTracking = true;

	std::atomic<bool> m_MakeScreenshot = false;

	void RegisterEvents();

protected: // Rendering.

	ComPtr<ID3D11Device> m_Device;
	ComPtr<ID3D11DeviceContext> m_DeviceContext;
	ComPtr<IDXGISwapChain1> m_SwapChain;

	ComPtr<ID3D11Texture2D> m_ColorBackBuffer;
	ComPtr<ID3D11RenderTargetView> m_ColorBackBufferRTV;

	DirectX11Render::Managers m_DX11M;
	DirectX11Render::Utilites m_DX11U;

	// Depth and render targets for multisampled rendering.
	DirectX11Render::Texture2D m_ColorBuffer;
	DirectX11Render::Texture2D m_DepthBuffer;

	// For main debug text.
	DirectX11Render::UpdatableTextRenderTask m_Debug_TRT;

	// Pipeline state index, for setting before resolving the subresource.
	unsigned m_ResolveSubresourcePipelineStateIndex;

	void LoadPipeline();
	void RenderDebug();

public: // ILoggerListener

	void OnLog(const char* message, LogSeverity severity, LogFlags flags) override;

public: // Interface for contained applications.

	EngineBuildingBlocks::Graphics::ModelLoader& GetModelLoader();

public: // Interface for user.

	MainApplication(int argc, char *argv[]);
	~MainApplication();

	unsigned GetExitECI() const;
	bool HandleEvent(const EngineBuildingBlocks::Event* _event);

	void InitializeMain();
	void InitializeRendering();
	void DestroyMain();
	void DestroyRendering();

	void DerivedPreUpdate();
	void DerivedPostUpdate(EngineBuildingBlocks::PostUpdateContext& context);
	void DerivedRender(EngineBuildingBlocks::RenderContext& context);

	void DerivedPresent();

	bool OnEvent(UINT message, WPARAM wParam, LPARAM lParam);

	void DerivedParseCommandLineArgs(int argc, char *argv[]);

	void SetGUIActive(bool isActive);
	void SetAllowGUIActiveTracking(bool allow);

	const Settings& GetSettings() const;
};
