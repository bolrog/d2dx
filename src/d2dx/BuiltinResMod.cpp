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
#include "BuiltinResMod.h"
#include "Utils.h"
#include "resource.h"
#include "GameHelper.h"

using namespace d2dx;

#ifndef D2DX_UNITTEST
bool SDHD_Initialize();
#endif

_Use_decl_annotations_
BuiltinResMod::BuiltinResMod(
    HMODULE hModule,
    const std::shared_ptr<IGameHelper>& gameHelper)
{
    if (!hModule || !gameHelper)
    {
        D2DX_CHECK_HR(E_INVALIDARG);
    }

#ifndef D2DX_UNITTEST
    if (IsCompatible(gameHelper.get()))
    {
        D2DX_LOG("Writing MPQ.");
        EnsureMpqExists(hModule);

        D2DX_LOG("Initializing built-in resolution mod.");
        SDHD_Initialize();

        _isActive = true;
        return;
    }
#endif

    _isActive = false;
}

BuiltinResMod::~BuiltinResMod()
{
}

bool BuiltinResMod::IsActive() const
{
    return _isActive;
}

_Use_decl_annotations_
bool BuiltinResMod::IsCompatible(
    IGameHelper* gameHelper)
{
    auto gameVersion = gameHelper->GetVersion();

    if (gameVersion != d2dx::GameVersion::Lod112 &&
        gameVersion != d2dx::GameVersion::Lod113c &&
        gameVersion != d2dx::GameVersion::Lod113d)
    {
        D2DX_LOG("Unsupported game version, won't use built-in resolution mod.");
        return false;
    }

    if (LoadLibraryA("D2Sigma.dll"))
    {
        D2DX_LOG("Detected Median XL, won't use built-in resolution mod.");
        return false;
    }

    return true;
}

_Use_decl_annotations_
void BuiltinResMod::EnsureMpqExists(
    HMODULE hModule)
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
