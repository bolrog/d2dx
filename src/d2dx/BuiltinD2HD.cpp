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
#include "BuiltinD2HD.h"
#include "Utils.h"
#include "resource.h"
#include "GameHelper.h"

using namespace d2dx;

#ifndef D2DX_UNITTEST
bool SDHD_Initialize();
#endif

static bool IsCompatible();
static void EnsureMpqExists(HMODULE hModule);

bool d2dx::BuiltinResMod::TryInitialize(HMODULE hModule)
{
    static bool initialized = false;

    if (initialized)
    {
        return true;
    }

    initialized = true;

#ifndef D2DX_UNITTEST
    if (IsCompatible())
    {
        ALWAYS_PRINT("Writing MPQ.");
        EnsureMpqExists(hModule);

        ALWAYS_PRINT("Initializing built-in D2HD.");
        SDHD_Initialize();

        return true;
    }
    else
    {
        return false;
    }
#endif
}

static bool IsCompatible()
{
    GameHelper gameHelper;
    auto gameVersion = gameHelper.GetVersion();

    if (gameVersion != d2dx::GameVersion::Lod112 &&
        gameVersion != d2dx::GameVersion::Lod113c &&
        gameVersion != d2dx::GameVersion::Lod113d)
    {
        ALWAYS_PRINT("Unsupported game version, won't use built-in D2HD.");
        return false;
    }

    if (LoadLibraryA("D2Sigma.dll"))
    {
        ALWAYS_PRINT("Detected Median XL, won't use built-in D2HD.");
        return false;
    }

    return true;
}

static void EnsureMpqExists(HMODULE hModule)
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
