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

_Use_decl_annotations_
BuiltinResMod::BuiltinResMod(
    HMODULE hModule,
    Size gameSize,
    const std::shared_ptr<IGameHelper>& gameHelper)
{
    if (!hModule || !gameHelper)
    {
        D2DX_CHECK_HR(E_INVALIDARG);
    }

    _isActive = false;

#ifndef D2DX_UNITTEST
    if (IsCompatible(gameHelper.get()))
    {
        D2DX_LOG("Writing SGD2FreeRes files.");

        if (!WriteResourceToFile(hModule, IDR_SGD2FR_MPQ, "mpq", "d2dx_sgd2freeres.mpq"))
        {
            D2DX_LOG("Failed to write d2dx_sgd2freeres.mpq");
        }

        if (!WriteResourceToFile(hModule, IDR_SGD2FR_DLL, "dll", "d2dx_sgd2freeres.dll"))
        {
            D2DX_LOG("Failed to write d2dx_sgd2freeres.mpq");
        }

        if (!WriteConfig(gameSize))
        {
            D2DX_LOG("Failed to write SGD2FreeRes configuration.");
        }

        D2DX_LOG("Initializing SGD2FreeRes.");
        LoadLibraryA("d2dx_sgd2freeres.dll");

        _isActive = true;
        return;
    }
#endif
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

    if (gameVersion != d2dx::GameVersion::Lod109d &&
        gameVersion != d2dx::GameVersion::Lod113c &&
        gameVersion != d2dx::GameVersion::Lod113d &&
        gameVersion != d2dx::GameVersion::Lod114d)
    {
        D2DX_LOG("Unsupported game version, won't use built-in resolution mod.");
        return false;
    }

    return true;
}

_Use_decl_annotations_
bool BuiltinResMod::WriteResourceToFile(
    HMODULE hModule,
    int32_t resourceId,
    const char* ext,
    const char* filename)
{
    void* payloadPtr = nullptr;
    DWORD payloadSize = 0;
    HRSRC resourceInfo = nullptr;
    HGLOBAL resourceData = nullptr;
    HANDLE file = nullptr;
    DWORD bytesWritten = 0;
    bool succeeded = true;

    resourceInfo = FindResourceA(hModule, MAKEINTRESOURCEA(resourceId), ext);
    if (!resourceInfo)
    {
        succeeded = false;
        goto end;
    }

    resourceData = LoadResource(hModule, resourceInfo);
    if (!resourceData)
    { 
        succeeded = false;
        goto end;
    }

    payloadSize = SizeofResource(hModule, resourceInfo);
    payloadPtr = LockResource(resourceData);
    if (!payloadPtr || !payloadSize)
    {
        succeeded = false;
        goto end;
    }
    
    succeeded = DecompressLZMAToFile((const uint8_t*)payloadPtr, payloadSize, filename);

end:
    if (resourceData)
    {
        UnlockResource(resourceData);
    }
    if (file)
    {
        CloseHandle(file);
    }

    return succeeded;
}

_Use_decl_annotations_
bool BuiltinResMod::WriteConfig(
    _In_ Size gameSize)
{
    HANDLE file = nullptr;
    DWORD bytesWritten = 0;
    bool succeeded = true;

    file = CreateFileA("d2dx_sgd2freeres.json", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!file)
    {
        succeeded = false;
        goto end;
    }

    char configStr[1024];

    sprintf(configStr,
        "{\r\n"
        "   \"SlashGaming Diablo II Free Resolution\": {\r\n"
        "       \"!!!Metadata (Do not modify)!!!\": {\r\n"
        "           \"Major Version A\": 3,\r\n"
        "           \"Major Version B\" : 0,\r\n"
        "           \"Minor Version A\" : 1,\r\n"
        "           \"Minor Version B\" : 0\r\n"
        "       },\r\n"
        "       \"Ingame Resolutions\": [\r\n"
        "           \"640x480\",\r\n"
        "           \"800x600\",\r\n"
        "           \"%ix%i\"\r\n"
        "       ],\r\n"
        "       \"Ingame Resolution Mode\" : 2,\r\n"
        "       \"Main Menu Resolution\" : \"800x600\",\r\n"
        "       \"Custom MPQ File\": \"d2dx_sgd2freeres.mpq\",\r\n"
        "       \"Enable Screen Border Frame?\" : true,\r\n"
        "       \"Use Original Screen Border Frame?\" : false,\r\n"
        "       \"Use 800 Interface Bar?\" : true\r\n"
        "   },\r\n"
        "   \"!!!Globals!!!\": {\r\n"
        "       \"Config Tab Width\": 4\r\n"
        "   }\r\n"
        "}\r\n",
        gameSize.width, gameSize.height);

    if (!WriteFile(file, configStr, strlen(configStr), &bytesWritten, nullptr))
    {
        succeeded = false;
        goto end;
    }

end:
    if (file)
    {
        CloseHandle(file);
    }

    return succeeded;
}
