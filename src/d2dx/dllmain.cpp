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
#include "D2DXDetours.h"
#include "GameHelper.h"
#include "BuiltinD2HD.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace d2dx;
using namespace std;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        printf("DLL_PROCESS_ATTACH\n");        
        SetProcessDPIAware();
        AttachDetours();
        TryInitializeBuiltinD2HD(hModule);
        break;
    }
    case DLL_THREAD_ATTACH:
        printf("DLL_THREAD_ATTACH\n");
        break;
    case DLL_THREAD_DETACH:
        printf("DLL_THREAD_DETACH\n");
        break;
    case DLL_PROCESS_DETACH:
        printf("DLL_PROCESS_DETACH\n");
        DetachDetours();
        break;
    }
    return TRUE;
}
