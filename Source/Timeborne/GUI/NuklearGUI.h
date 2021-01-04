// Timeborne/GUI/NuklearGUI.h

#pragma once

#if defined (_WIN32)
#if defined(FUNCTIONS_STATIC)
#define FUNCTIONS_API
#else
#if defined(FUNCTIONS_EXPORTS)
#define FUNCTIONS_API __declspec(dllexport)
#else
#define FUNCTIONS_API __declspec(dllimport)
#endif
#endif
#else
#define FUNCTIONS_API
#endif

#ifdef __cplusplus  
extern "C" {  // only need to export C interface if  
			  // used by C++ source code  
#endif  

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct NuklearInitContext
{
	unsigned WindowWidth, WindowHeight;
	void* Device;
	void* DeviceContext;
	const char* SolutionPath;
	int FontSize;
};

struct NuklearRenderContext
{
	void* DeviceContext;
};

FUNCTIONS_API void* Nuklear_GetContext();

FUNCTIONS_API void Nuklear_Initialize(const struct NuklearInitContext* pContext);
FUNCTIONS_API void Nuklear_Destroy();

FUNCTIONS_API int Nuklear_HandleEvents(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

FUNCTIONS_API void* Nuklear_StartCreation();
FUNCTIONS_API void Nuklear_Render(const struct NuklearRenderContext* pContext);

FUNCTIONS_API int Nuklear_IsMouseInAWindow(void* windowHandle);

#ifdef __cplusplus  
}
#endif 
