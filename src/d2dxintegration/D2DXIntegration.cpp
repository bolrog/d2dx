#include "D2DXIntegration.h"
#include <Windows.h>

typedef void (*PD2DX_SetCustomResolution)(int32_t width, int32_t height);
typedef void (*PD2DX_GetSuggestedCustomResolution)(int32_t* width, int32_t* height);

static HMODULE hGlide3x = NULL;
static bool initialized = false;
static PD2DX_SetCustomResolution d2dxSetCustomResolution;
static PD2DX_GetSuggestedCustomResolution d2dxGetSuggestedCustomResolution;

static void EnsureInitialized()
{
	if (initialized)
	{
		return;
	}

	hGlide3x = LoadLibraryW(L"glide3x.dll");
	
	if (hGlide3x)
	{
		d2dxSetCustomResolution = (PD2DX_SetCustomResolution)GetProcAddress(hGlide3x, "D2DX_SetCustomResolution");
		d2dxGetSuggestedCustomResolution = (PD2DX_GetSuggestedCustomResolution)GetProcAddress(hGlide3x, "D2DX_GetSuggestedCustomResolution");
	}

	initialized = true;
}

bool d2dx::IsD2DXLoaded()
{
	EnsureInitialized();

	return d2dxSetCustomResolution != nullptr &&
		d2dxGetSuggestedCustomResolution != nullptr;
}

void d2dx::SetCustomResolution(int32_t width, int32_t height)
{
	EnsureInitialized();

	if (d2dxSetCustomResolution)
	{
		d2dxSetCustomResolution(width, height);
	}
}

void d2dx::GetSuggestedCustomResolution(int32_t* width, int32_t* height)
{
	EnsureInitialized();

	if (d2dxGetSuggestedCustomResolution)
	{
		d2dxGetSuggestedCustomResolution(width, height);
	}
	else
	{
		*width = 0;
		*height = 0;
	}
}

