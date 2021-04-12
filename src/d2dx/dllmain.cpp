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
#include "Detours.h"
#include "GameHelper.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace d2dx;
using namespace std;

static void FixCompatibilityMode(HKEY hRootKey);

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        FixCompatibilityMode(HKEY_LOCAL_MACHINE);
        FixCompatibilityMode(HKEY_CURRENT_USER);
        SetProcessDPIAware();
        AttachDetours();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        DetachDetours();
        break;
    }
    return TRUE;
}

static bool HasIncompatibleCompatibilityOptions(const wchar_t* str)
{
    return
        wcsstr(str, L"WIN95") != nullptr ||
        wcsstr(str, L"WIN98") != nullptr ||
        wcsstr(str, L"WINXP") != nullptr ||
        wcsstr(str, L"VISTA") != nullptr ||
        wcsstr(str, L"WIN7") != nullptr ||
        wcsstr(str, L"WIN8") != nullptr;
}

static void FixCompatibilityMode(HKEY hRootKey)
{
    HKEY hKey;
    LPCTSTR compatibilityLayersKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
    LONG result = RegOpenKeyEx(hRootKey, compatibilityLayersKey, 0, KEY_READ | KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS)
    {
        return;
    }
     
    Buffer<wchar_t> filename(2048);
    uint32_t numChars = GetModuleFileName(GetModuleHandle(nullptr), filename.items, filename.capacity);
    if (numChars < 1 || numChars >= filename.capacity)
    {
        return;
    }

    Buffer<wchar_t> value(2048);
    uint32_t type = REG_SZ;
    uint32_t size = value.capacity;
    result = RegGetValue(hKey, NULL, filename.items, RRF_RT_REG_SZ, (LPDWORD)&type, (LPBYTE)value.items, (LPDWORD)&size);
    if (result != ERROR_SUCCESS)
    {
        return;
    }

    bool hasFixed = false;

    if (HasIncompatibleCompatibilityOptions(value.items))
    {
        RegDeleteValue(hKey, filename.items);
        hasFixed = true;
    }

    HRESULT hr = PathCchRemoveFileSpec(filename.items, filename.capacity);
    if (FAILED(hr))
    {
        return;
    }

    wcscat_s(filename.items, filename.capacity, L"\\Diablo II.exe");

    result = RegGetValue(hKey, NULL, filename.items, RRF_RT_REG_SZ, (LPDWORD)&type, (LPBYTE)value.items, (LPDWORD)&size);
    if (result != ERROR_SUCCESS)
    {
        return;
    }

    if (HasIncompatibleCompatibilityOptions(value.items))
    {
        RegDeleteValue(hKey, filename.items);
        hasFixed = true; 
    }

    if (hasFixed)
    {
        MessageBox(NULL, L"D2DX detected that 'compatibility mode' was set for the game, but this isn't necessary for D2DX and will cause problems.\n\nThis has now been fixed for you. Please re-launch the game.", L"D2DX", MB_OK);
        TerminateProcess(GetCurrentProcess(), -1);
    }
}
