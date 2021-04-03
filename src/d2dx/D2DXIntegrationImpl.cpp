/*
    This file is part of D2DX.

    Copyright (C) 2021  Bolrog

    D2DX is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    D2DX is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with D2DX.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "pch.h"
#include "D2DXContext.h"

using namespace d2dx;
using namespace std;

extern "C"
{
    __declspec(dllexport) void D2DX_SetCustomResolution(int32_t width, int32_t height)
    {
        auto d2dxContext = D2DXContext::Instance();
        if (d2dxContext)
        {
            d2dxContext->SetCustomResolution(width, height);
        }
    }

    __declspec(dllexport) void D2DX_GetSuggestedCustomResolution(int32_t* width, int32_t* height)
    {
        auto d2dxContext = D2DXContext::Instance();
        if (d2dxContext)
        {
            d2dxContext->GetSuggestedCustomResolution(width, height);
        }
    }
}
