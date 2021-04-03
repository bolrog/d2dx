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
#pragma once

namespace d2dx
{
	int64_t TimeStart();
	float TimeEndMs(int64_t start);

#ifdef NDEBUG
#define DEBUG_PRINT(fmt, ...)
#else
#define DEBUG_PRINT(fmt, ...) \
	{ \
		static char ss[256]; \
		sprintf_s(ss, fmt "\n", __VA_ARGS__); \
		OutputDebugStringA(ss); \
	}
#endif

#define ALWAYS_PRINT(fmt, ...) \
	{ \
		static char ssss[256]; \
		sprintf_s(ssss, fmt "\n", __VA_ARGS__); \
		OutputDebugStringA(ssss); \
	}

	static void release_check(bool expr, const char* exprString)
	{
		if (!expr)
		{
			OutputDebugStringA(exprString);
			MessageBoxA(NULL, exprString, "Failed assertion", MB_OK);
			PostQuitMessage(-1);
		}
	}

	static void release_check_hr(HRESULT hr, const char* exprString)
	{
		if (FAILED(hr))
		{
			OutputDebugStringA(exprString);
			MessageBoxA(NULL, exprString, "Failed assertion", MB_OK);
			PostQuitMessage(-1);
		}
	}

	#define D2DX_RELEASE_CHECK(expr) release_check(expr, #expr)
	#define D2DX_RELEASE_CHECK_HR(expr) release_check_hr(expr, #expr)
}
