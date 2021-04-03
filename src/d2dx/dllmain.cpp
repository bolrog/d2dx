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
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

bool SDHD_Initialize();

using namespace d2dx;
using namespace std;

static void EnsureD2HDMpqExists(HMODULE hModule);

static bool IsWideModeAvailable()
{
    const char* commandLine = GetCommandLineA();
    if (strstr(commandLine, "-dxnowide"))
    {
        return false;
    }

    GameHelper gameHelper;
    auto gameVersion = gameHelper.GetVersion();

    if (gameVersion != d2dx::GameVersion::Lod112 &&
        gameVersion != d2dx::GameVersion::Lod113c &&
        gameVersion != d2dx::GameVersion::Lod113d)
    {
        return false;
    }

    if (LoadLibraryA("D2Sigma.dll"))
    {
        return false;
    }

    return true;
}

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

        if (IsWideModeAvailable())
        {            
            EnsureD2HDMpqExists(hModule);
            SDHD_Initialize();
        }

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

static void EnsureD2HDMpqExists(HMODULE hModule)
{
    void* d2HDMpqPtr = nullptr;
    DWORD d2HDMpqSize = 0;
    HRSRC d2HDMpqResourceInfo = nullptr;
    HGLOBAL d2HDMpqResourceData = nullptr;
    HANDLE d2HDMpqFile = nullptr;
    DWORD bytesWritten = 0;

    d2HDMpqResourceInfo = FindResourceA(hModule, MAKEINTRESOURCEA(IDR_MPQ1), "mpq");
    if (!d2HDMpqResourceInfo)
    {
        goto end;
    }
    
    d2HDMpqResourceData = LoadResource(hModule, d2HDMpqResourceInfo);
    if (!d2HDMpqResourceData)
    {
        goto end;
    }

    d2HDMpqSize = SizeofResource(hModule, d2HDMpqResourceInfo);
    d2HDMpqPtr = LockResource(d2HDMpqResourceData);
    if (!d2HDMpqPtr || !d2HDMpqSize)
    {
        goto end;
    }

    d2HDMpqFile = CreateFileA("d2dx_d2hd.mpq", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!d2HDMpqFile)
    {
        goto end;
    }

    if (!WriteFile(d2HDMpqFile, d2HDMpqPtr, d2HDMpqSize, &bytesWritten, nullptr))
    {
        goto end;
    }

end:
    if (d2HDMpqResourceData)
    {
        UnlockResource(d2HDMpqResourceData);
    }
    if (d2HDMpqFile)
    {
        CloseHandle(d2HDMpqFile);
    }
}
