#include "pch.h"
#include "D2DXContext.h"

using namespace d2dx;
using namespace std;

extern unique_ptr<D2DXContext> g_d2dxContext;

extern "C"
{
    __declspec(dllexport) void D2DX_SetCustomResolution(int32_t width, int32_t height)
    {
        if (g_d2dxContext)
        {
            g_d2dxContext->SetCustomResolution(width, height);
        }
    }

    __declspec(dllexport) void D2DX_GetSuggestedCustomResolution(int32_t* width, int32_t* height)
    {
        if (g_d2dxContext)
        {
            g_d2dxContext->GetSuggestedCustomResolution(width, height);
        }
    }
}
