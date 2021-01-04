// Timeborne/GUI/NuklearGUI.c

#define FUNCTIONS_EXPORTS
#include <Timeborne/GUI/NuklearGUI.h>

// Defining the implementation ONLY here.
#define NK_IMPLEMENTATION
#define NK_D3D11_IMPLEMENTATION

#include <Timeborne/GUI/NuklearInclude.h>
#include <Timeborne/GUI/NuklearD3D11Include.h>

HANDLE ctx_mutex;
static struct nk_context* ctx;
static struct nk_font_atlas* atlas;

void Nuklear_LockMutex()
{
	WaitForSingleObject(ctx_mutex, INFINITE);
}

void Nuklear_UnlockMutex()
{
	ReleaseMutex(ctx_mutex);
}

void* Nuklear_GetContext()
{
	return ctx;
}

void Nuklear_Initialize(const struct NuklearInitContext* pContext)
{
	int width = pContext->WindowWidth;
	int height = pContext->WindowHeight;
	struct ID3D11Device* device = pContext->Device;
	struct ID3D11DeviceContext* context = pContext->DeviceContext;

	ctx_mutex = CreateMutex(NULL, FALSE, NULL);

	ctx = nk_d3d11_init(pContext->Device, width, height, MAX_VERTEX_BUFFER, MAX_INDEX_BUFFER);

	{
		nk_d3d11_font_stash_begin(&atlas);

		char buffer[256];
		snprintf(buffer, 256, "%sExternal/nuklear/extra_font/ProggyClean.ttf", pContext->SolutionPath);
		struct nk_font *appFont = nk_font_atlas_add_from_file(atlas, buffer, (float)pContext->FontSize, NULL);

		nk_d3d11_font_stash_end();

		/* Load Cursor: if you uncomment cursor loading please hide the cursor */
		/*nk_style_load_all_cursors(ctx, atlas->cursors);*/

		nk_style_set_font(ctx, &appFont->handle);
	}

	// Calling resize explicitely.
	nk_d3d11_resize(context, width, height);

	nk_input_begin(ctx);
}

void Nuklear_Destroy()
{
	nk_d3d11_shutdown();
}

int Nuklear_HandleEvents(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	Nuklear_LockMutex();
	int res = nk_d3d11_handle_event(wnd, msg, wparam, lparam);
	Nuklear_UnlockMutex();
	return res;
}

void* Nuklear_StartCreation()
{
	Nuklear_LockMutex();
	nk_input_end(ctx);
	return ctx;
}

void Nuklear_Render(const struct NuklearRenderContext* pContext)
{
	ID3D11DeviceContext* context = pContext->DeviceContext;

	nk_d3d11_render(context, NK_ANTI_ALIASING_ON);

	nk_input_begin(ctx);
	Nuklear_UnlockMutex();
}

int Nuklear_IsMouseInAWindow(void* windowHandle)
{
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	ScreenToClient((HWND)windowHandle, &cursorPos);

	for (struct nk_window* iter = ctx->begin; iter; iter = iter->next)
	{
		NK_ASSERT(iter != iter->next);
		struct nk_rect bounds = iter->bounds;
		if (cursorPos.x >= bounds.x && cursorPos.x < bounds.x + bounds.w
			&& cursorPos.y >= bounds.y && cursorPos.y < bounds.y + bounds.h) return 1;
	}
	return 0;
}