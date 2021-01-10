// Timeborne/MainApplication.cpp

// For NuklearGUI.h.
#define FUNCTIONS_STATIC

#include <Timeborne/MainApplication.h>

#include <EngineBuildingBlocks/Input/DefaultInputBinder.h>
#include <DirectX11Render/GraphicsDevice.h>

#include <Timeborne/GUI/NuklearGUI.h>
#include <Timeborne/Misc/UserConfiguration.h>
#include <Timeborne/Screens/GameEndScreen.h>
#include <Timeborne/Screens/InGameScreen.h>
#include <Timeborne/Screens/LevelEditorScreen.h>
#include <Timeborne/Screens/MainMenuScreen.h>
#include <Timeborne/Screens/SinglePlayerScreen.h>

#include <windows.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Input;
using namespace DirectXRender;
using namespace DirectX11Render;
using namespace WindowsApplication;

struct LocaleInitializer
{
	LocaleInitializer()
	{
		Core::InitializeLocale();
	}
};
static LocaleInitializer s_LocaleInitializer;

MainApplication::MainApplication(int argc, char *argv[])
	: WindowsApplication::Application<MainApplication>(argc, argv)
	, m_Console(m_CommandLine)
	, m_DX11M(m_PathHandler)
{
	Logger::GetInstance()->SetListener(this, LogSeverity::Warning);
	LoadConfiguration();
	m_Window.SetTitle(m_Title);

	m_Debug_TRT.SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
	m_Debug_TRT.SetPosition(TextPositionConstant::TopLeft);
	m_Debug_TRT.SetLineHeight(0.04f);
}

void MainApplication::InitializeComponents()
{
	// Adding screens.
	m_Screens.push_back(std::make_unique<MainMenuScreen>());
	m_Screens.push_back(std::make_unique<SinglePlayerScreen>());
	m_Screens.push_back(std::make_unique<InGameScreen>());
	m_Screens.push_back(std::make_unique<GameEndScreen>());
	m_Screens.push_back(std::make_unique<LevelEditorScreen>());
	for (auto& screen : m_Screens)
	{
		m_Components.push_back(screen.get());
	}
	
	// Adding other components.
	m_Components.push_back(&m_Console);

	ComponentInitContext context{ this, m_WindowSize, m_ContentSize };
	m_Console.InitializeMain(context);
	for (auto& pComponent : m_Components)
	{
		pComponent->InitializeMain(context);
	}
}

ApplicationScreen* MainApplication::GetCurrentScreen()
{
	return m_Screens[(int)m_CurrentScreen].get();
}

ApplicationScreen* MainApplication::GetScreen(ApplicationScreens screen)
{
	return m_Screens[(int)screen].get();
}

void MainApplication::DerivedParseCommandLineArgs(int argc, char *argv[])
{
}

MainApplication::~MainApplication()
{
}

void MainApplication::InitializeMain()
{
	m_Window.Initialize(m_WindowSize.x, m_WindowSize.y);

	GetKeyHandler()->SetKeyStateProvider(&m_Window);
	GetMouseHandler()->SetMouseStateProvider(&m_Window);
	GetMouseHandler()->SetAntiPrellFilterTimeout(m_MouseAntiPrellFilterTimeout);

	RegisterEvents();

	InitializeComponents();
}

ComponentRenderContext MainApplication::GetComponentRenderContext(void* nuklearContext, ID3D11RenderTargetView* rtv)
{
	return { this, &m_ThreadPool, m_Device.Get(), m_DeviceContext.Get(), rtv, &m_DX11M, &m_DX11U, nuklearContext,
		&m_Settings, m_WindowSize, m_ContentSize };
}

void MainApplication::InitializeRendering()
{
	LoadPipeline();

	auto initContext = UtilityInitializationContext{ m_Device.Get(), m_DeviceContext.Get(),
		&m_PathHandler, &m_DX11M, m_MultisampleCount };
	m_DX11U.Initialize(initContext);

	auto solutionPath = m_PathHandler.GetPathFromSolution("");
	NuklearInitContext nkContext{ m_WindowSize.x, m_WindowSize.y, m_Device.Get(), m_DeviceContext.Get(), solutionPath.c_str(), m_FontSize };
	Nuklear_Initialize(&nkContext);

	auto renderContext = GetComponentRenderContext(nullptr, nullptr);
	for (auto& pComponent : m_Components)
	{
		pComponent->InitializeRendering(renderContext);
	}

	m_CurrentScreen = ApplicationScreens::MainMenu;
	GetCurrentScreen()->Enter(renderContext);
}

void MainApplication::RegisterEvents()
{
	DefaultInputBinder::Bind(this, &m_KeyHandler, &m_MouseHandler, nullptr);
}

unsigned MainApplication::GetExitECI() const
{
	return Core::c_InvalidIndexU;
}

bool MainApplication::HandleEvent(const EngineBuildingBlocks::Event* _event)
{
	return GetCurrentScreen()->HandleEvent(_event);
}

void MainApplication::SetGUIActive(bool isActive)
{
	m_IsGUIActive = isActive;
}

void MainApplication::SetAllowGUIActiveTracking(bool allow)
{
	m_AllowGUIActiveTracking = allow;
}

bool MainApplication::OnEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
	// The print-screen button never fires a key down event.
	if (message == WM_KEYUP && m_Window.GetGeneralKey((unsigned)wParam) == Keys::PrintScreen)
	{
		m_MakeScreenshot = true;
		return true;
	}

	Nuklear_HandleEvents(m_Window.GetHandle(), message, wParam, lParam);

	if (!IsInputMessage(message)) return false;
	
	if (m_AllowGUIActiveTracking && IsMouseButtonDownMessage(message))
		m_IsGUIActive = Nuklear_IsMouseInAWindow(GetWindow().GetHandle());

	return m_IsGUIActive;
}

void MainApplication::DestroyMain()
{
	for (auto& pComponent : m_Components)
	{
		pComponent->DestroyMain();
	}
}

void MainApplication::LoadPipeline()
{
#ifdef _DEBUG
	bool isCreatingDegviceDebug = true;
#else
	bool isCreatingDegviceDebug = false;
#endif

	ComPtr<IDXGIFactory4> factory;
	CreateDXGIFactory(factory, m_Window.GetHandle());
	CreateGraphicsDevice(GraphicsDeviceType::Hardware, D3D_FEATURE_LEVEL_11_0, factory, m_Device, m_DeviceContext, isCreatingDegviceDebug);
	CreateSwapChain(factory, m_Device, m_SwapChain, m_ColorBackBuffer, m_ColorBackBufferRTV, m_Window.GetHandle(), m_WindowSize.x, m_WindowSize.y,
		m_IsWindowed, m_BackbufferFrameCount, c_DefaultColorBackBufferFormat, true);

	// Creating multisampled color and depth buffers.
	DirectX11Render::Texture2DDescription colorBufferDesc(m_WindowSize.x, m_WindowSize.y, c_DefaultColorBackBufferFormat, 1,
		m_MultisampleCount, D3D11_USAGE_DEFAULT, TextureBindFlag::RenderTarget);
	m_ColorBuffer.Initialize(m_Device.Get(), colorBufferDesc, nullptr, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN,
		c_DefaultColorBackBufferFormat);

	DirectX11Render::Texture2DDescription depthBufferDesc(m_WindowSize.x, m_WindowSize.y, DXGI_FORMAT_R32_TYPELESS, 1,
		m_MultisampleCount, D3D11_USAGE_DEFAULT, TextureBindFlag::DepthStencil);
	m_DepthBuffer.Initialize(m_Device.Get(), depthBufferDesc, nullptr, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_D32_FLOAT);

	{
		PipelineStateDescription description;
		EngineBuildingBlocks::Graphics::VertexInputLayout il;
		il.Elements = { EngineBuildingBlocks::Graphics::c_PositionVertexElement };
		description.InputLayout = VertexInputLayout::Create(il);
		description.DepthStencilState.DisableDepthWriting();
		m_ResolveSubresourcePipelineStateIndex = m_DX11M.PipelineStateManager.GetPipelineStateIndex(m_Device.Get(), description);
	}
}

void MainApplication::HandleComponentTransition()
{
	auto nextScreen = GetCurrentScreen()->GetNextScreen();
	if (m_CurrentScreen != nextScreen)
	{
		auto renderContext = GetComponentRenderContext(nullptr, nullptr);

		auto oldScreen = m_CurrentScreen;
		GetCurrentScreen()->Exit();

		m_CurrentScreen = nextScreen;
		m_IsGUIActive = false;
		m_AllowGUIActiveTracking = true;
		
		auto newScreen = GetCurrentScreen();
		newScreen->SetPreviousScreen(oldScreen);
		newScreen->Enter(renderContext);
	}
}

void MainApplication::DerivedPreUpdate()
{
	m_DX11U.PreUpdate({ m_DeviceContext.Get() });

	HandleComponentTransition();

	ComponentPreUpdateContext context{ this, &m_ThreadPool, m_WindowSize, m_ContentSize };
	GetCurrentScreen()->PreUpdate(context);
}

void MainApplication::DerivedPostUpdate(EngineBuildingBlocks::PostUpdateContext& context)
{
	m_DX11U.PostUpdate({ m_DeviceContext.Get() });

	if (context.IsFPSRefreshed)
	{
		auto fps = m_FPSController.GetFPS();

		if (m_IsWindowed)
		{
			std::stringstream ssTitle;
			ssTitle << m_Title << " :: FPS: " << fps;
			m_Window.SetTitle(ssTitle.str());
		}
		std::stringstream ssTRT;
		ssTRT << "FPS: " << fps;
		m_Debug_TRT.SetText(ssTRT.str().c_str());
		m_Debug_TRT.Update(m_DX11U.TextRenderer);
	}

	ComponentPostUpdateContext puContext;
	puContext.Application = this;
	puContext.IsFPSRefreshed = context.IsFPSRefreshed;
	puContext.PathHandler = &m_PathHandler;
	puContext.WindowHandle = m_Window.GetHandle();
	puContext.SystemTime = &m_SystemTime;
	GetCurrentScreen()->PostUpdate(puContext);
}

// Render the scene.
void MainApplication::DerivedRender(EngineBuildingBlocks::RenderContext& context)
{
	auto windowSizeF = glm::vec2(m_WindowSize);
	auto contentSizeF = glm::vec2(m_ContentSize);
	D3D11_VIEWPORT fullViewport = { 0.0f, 0.0f, windowSizeF.x, windowSizeF.y, 0.0f, 1.0f };
	D3D11_VIEWPORT contentViewport = { windowSizeF.x - contentSizeF.x, 0, contentSizeF.x, contentSizeF.y, 0.0f, 1.0f };

	// Setting render target, depth buffer, viewport, clearing render target and depth buffers.
	ID3D11RenderTargetView* rtvs[]{ m_ColorBuffer.GetRTV() };
	m_DeviceContext->OMSetRenderTargets(1, rtvs, m_DepthBuffer.GetDSV());
	m_DeviceContext->RSSetViewports(1, &fullViewport);
	m_DeviceContext->ClearDepthStencilView(m_DepthBuffer.GetDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Backing up state for the GUI.
	ID3D11DepthStencilState* dsState = nullptr;
	ID3D11RasterizerState* rsState = nullptr;
	ID3D11BlendState* bState = nullptr;
	unsigned stencilRef = Core::c_InvalidIndexU;
	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	unsigned blendMask = 0;
	m_DeviceContext->RSGetState(&rsState);
	m_DeviceContext->OMGetDepthStencilState(&dsState, &stencilRef);
	m_DeviceContext->OMGetBlendState(&bState, blendFactor, &blendMask);

	auto nuklearContext = Nuklear_StartCreation();
	auto renderContext = GetComponentRenderContext(nuklearContext, rtvs[0]);

	// Clearing in full screen viewport.
	GetCurrentScreen()->RenderFullscreenClear(renderContext);

	// Setting content view port.
	m_DeviceContext->RSSetViewports(1, &contentViewport);

	// Rendering the content and debug elements.
	GetCurrentScreen()->RenderContent(renderContext);
	RenderDebug();

	// Clearing the depth buffer, because GUI elements has to be rendered on top of the content. The other parts of the depth buffer are still unset.
	m_DeviceContext->ClearDepthStencilView(m_DepthBuffer.GetDSV(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Setting full viewport.
	m_DeviceContext->RSSetViewports(1, &fullViewport);

	// Setting state for the GUI.
	{
		m_DeviceContext->RSSetState(rsState);
		m_DeviceContext->OMSetDepthStencilState(dsState, stencilRef);
		m_DeviceContext->OMSetBlendState(bState, blendFactor, blendMask);
		m_DeviceContext->HSSetShader(nullptr, nullptr, 0);
		m_DeviceContext->DSSetShader(nullptr, nullptr, 0);
		m_DeviceContext->GSSetShader(nullptr, nullptr, 0);
		m_DeviceContext->CSSetShader(nullptr, nullptr, 0);
	};

	// Rendering the GUI.
	GetCurrentScreen()->RenderGUI(renderContext);
	if (GetCurrentScreen()->HasConsole())
	{
		m_Console.RenderGUI(renderContext);
	}
	NuklearRenderContext nkContext{ m_DeviceContext.Get() };
	Nuklear_Render(&nkContext);

	// Resolving subresource.
	m_DX11M.PipelineStateManager.GetPipelineState(m_ResolveSubresourcePipelineStateIndex).SetForContext(m_DeviceContext.Get());
	m_DeviceContext->ResolveSubresource(m_ColorBackBuffer.Get(), 0, m_ColorBuffer.GetResource(), 0,
		c_DefaultColorBackBufferFormat);

	bool expectedSetScreenshot = true;
	if (m_MakeScreenshot.compare_exchange_strong(expectedSetScreenshot, false))
	{
		m_DX11U.ScreenshotHandler.MakeScreenshot(m_Device.Get(), m_DeviceContext.Get(), m_ColorBackBuffer.Get(),
			m_WindowSize.x, m_WindowSize.y, c_DefaultColorBackBufferFormat);
	}
}

void MainApplication::DerivedPresent()
{
	auto hr = m_SwapChain->Present(m_SyncInterval, 0);
	if (FAILED(hr))
	{
		auto dhr = m_Device->GetDeviceRemovedReason();
		std::stringstream ss;
		ss << "Failed to present:\n\nError:" << Core::ToString(_com_error(hr).ErrorMessage()) << "\n"
			<< "Device remove reason: " << Core::ToString(_com_error(dhr).ErrorMessage());
		EngineBuildingBlocks::RaiseException(ss);
	}
}

void MainApplication::DestroyRendering()
{
	for (auto& pComponent : m_Components)
	{
		pComponent->DestroyRendering();
	}

	Nuklear_Destroy();

	if (!m_IsWindowed) m_SwapChain->SetFullscreenState(FALSE, nullptr);
}

void MainApplication::RenderDebug()
{
	m_DX11U.TextRenderer.Render({ m_DeviceContext.Get(), &m_DX11M });
}

void MainApplication::LoadConfiguration()
{
	m_Configuration = LoadUserConfiguration(
		m_PathHandler.GetPathFromResourcesDirectory("Configurations/Main_Configuration.xml").c_str(), &m_ResourceDatabase);
	m_Configuration.TryGetRootConfigurationValue("Title", m_Title);
	m_Configuration.TryGetRootConfigurationValue("IsWindowed", m_IsWindowed);
	if (m_IsWindowed)
	{
		m_Configuration.TryGetRootConfigurationValue("WindowedWidth", m_WindowSize.x);
		m_Configuration.TryGetRootConfigurationValue("WindowedHeight", m_WindowSize.y);
	}
	else
	{
		auto fullScreenRes = GetPrimaryMonitorResolution();
		m_WindowSize = { fullScreenRes.x,fullScreenRes.y };
	}
	m_Configuration.TryGetRootConfigurationValue("ContentWidth", m_ContentSize.x);
	m_Configuration.TryGetRootConfigurationValue("ContentHeight", m_ContentSize.y);
	m_Configuration.TryGetRootConfigurationValue("MultisampleCount", m_MultisampleCount);
	m_Configuration.TryGetRootConfigurationValue("BackbufferFrameCount", m_BackbufferFrameCount);
	m_Configuration.TryGetRootConfigurationValue("FontSize", m_FontSize);
	m_Configuration.TryGetRootConfigurationValue("Input.Mouse.AntiPrellFilterTimeout", m_MouseAntiPrellFilterTimeout);

	m_Settings.Load(m_Configuration);
}

const Settings& MainApplication::GetSettings() const
{
	return m_Settings;
}

void MainApplication::OnLog(const char* message, LogSeverity severity, LogFlags flags)
{
	if (HasFlag(flags, LogFlags::AddMessageBox))
	{
		const char* captions[] = { "Critical", "Error", "Warning", "Info", "Debug" };
		const unsigned iconFlags[] = { MB_ICONERROR, MB_ICONERROR, MB_ICONWARNING, MB_ICONINFORMATION, MB_ICONINFORMATION };
		MessageBoxA(m_Window.GetHandle(), message, captions[(int)severity], MB_OK | MB_APPLMODAL | iconFlags[(int)severity]);
	}
}

EngineBuildingBlocks::Graphics::ModelLoader& MainApplication::GetModelLoader()
{
	return m_ModelLoader;
}
