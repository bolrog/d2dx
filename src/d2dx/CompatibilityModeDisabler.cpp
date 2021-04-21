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
#include "CompatibilityModeDisabler.h"
#include "Utils.h"

using namespace d2dx;

static const wchar_t* osCompatibilityOptions[] =
{
	L"WIN95",
	L"WIN98",
	L"WINXP",
	L"VISTA",
	L"WIN7",
	L"WIN8"
};

CompatibilityModeDisabler::CompatibilityModeDisabler()
{
}

void CompatibilityModeDisabler::DisableCompatibilityMode()
{
	bool fixedCompatibilityMode = false;

	if (FixCompatibilityMode(HKEY_CURRENT_USER, L"Game.exe"))
	{
		fixedCompatibilityMode = true;
	}

	if (FixCompatibilityMode(HKEY_CURRENT_USER, L"Diablo II.exe"))
	{
		fixedCompatibilityMode = true;
	}

	if (fixedCompatibilityMode)
	{
		MessageBox(NULL, L"D2DX detected that 'compatibility mode' (e.g. for Windows XP) was set for the game, but this isn't necessary for D2DX and will cause problems.\n\nThis has now been fixed for you. Please re-launch the game.", L"D2DX", MB_OK);
		TerminateProcess(GetCurrentProcess(), -1);
	}

	// If compat mode is set for LOCAL_MACHINE, we can't see it in the registry and can't fix that... so try to detect it at least.

	// Get the reported (false) OS version.
	auto reportedWindowsVersion = d2dx::GetWindowsVersion();
	auto realWindowsVersion = d2dx::GetActualWindowsVersion();

	if (reportedWindowsVersion.major != realWindowsVersion.major)
	{
		MessageBox(NULL, L"D2DX detected that 'compatibility mode' (e.g. for Windows XP) was set for the game, but this isn't necessary for D2DX and will cause problems.\n\nPlease disable 'compatibility mode' for both 'Game.exe' and 'Diablo II.exe'.", L"D2DX", MB_OK);
		TerminateProcess(GetCurrentProcess(), -1);
	}

	if (reportedWindowsVersion.major < 6 || (reportedWindowsVersion.major == 6 && reportedWindowsVersion.minor == 0))
	{
		MessageBox(NULL, L"D2DX requires Windows 7 or above.", L"D2DX", MB_OK);
		TerminateProcess(GetCurrentProcess(), -1);
	}
}

_Use_decl_annotations_
bool CompatibilityModeDisabler::FixCompatibilityMode(
	_In_ HKEY hRootKey,
	_In_z_ const wchar_t* filename)
{
	HKEY hKey;
	LPCTSTR compatibilityLayersKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
	LONG result = RegOpenKeyEx(hRootKey, compatibilityLayersKey, 0, KEY_READ | KEY_WRITE, &hKey);
	if (result != ERROR_SUCCESS)
	{
		return false;
	}

	Buffer<wchar_t> fullPath(2048);
	uint32_t numChars = GetModuleFileName(GetModuleHandle(nullptr), fullPath.items, fullPath.capacity);
	if (numChars < 1 || numChars >= fullPath.capacity)
	{
		RegCloseKey(hKey);
		return false;
	}

	bool hasFixed = false;

	Buffer<wchar_t> value(2048);
	uint32_t type = REG_SZ;
	uint32_t size = value.capacity;

	result = RegGetValue(hKey, nullptr, fullPath.items, RRF_RT_REG_SZ, (LPDWORD)&type, nullptr, (LPDWORD)&size);

	if (result == ERROR_SUCCESS)
	{
		if (size > value.capacity)
		{
			RegCloseKey(hKey);
			return false;
		}

		result = RegGetValue(hKey, nullptr, fullPath.items, RRF_RT_REG_SZ, (LPDWORD)&type, (LPBYTE)value.items, (LPDWORD)&size);

		if (result == ERROR_SUCCESS)
		{
			if (HasOsCompatibilityOption(value.items))
			{
				result = RegDeleteValue(hKey, fullPath.items);
				assert(result == ERROR_SUCCESS);

				RegCloseKey(hKey);
				return true;
			}
		}
	}

	RegCloseKey(hKey);
	return false;
}

_Use_decl_annotations_
bool CompatibilityModeDisabler::HasOsCompatibilityOption(
	const wchar_t* options)
{
	for (int32_t i = 0; i < ARRAYSIZE(osCompatibilityOptions); ++i)
	{
		if (wcsstr(options, osCompatibilityOptions[i]))
		{
			return true;
		}
	}

	return false;
}
