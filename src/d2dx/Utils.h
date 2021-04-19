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

#include "Buffer.h"

namespace d2dx
{
	int64_t TimeStart();
	float TimeEndMs(int64_t start);

	void AlwaysPrint(const char* s);

#ifdef NDEBUG
#define DEBUG_PRINT(fmt, ...)
#else
#define DEBUG_PRINT(fmt, ...) \
	{ \
		static char ss[256]; \
		sprintf_s(ss, fmt "\n", __VA_ARGS__); \
		d2dx::AlwaysPrint(ss); \
	}
#endif

#define ALWAYS_PRINT(fmt, ...) \
	{ \
		static char ssss[256]; \
		sprintf_s(ssss, fmt "\n", __VA_ARGS__); \
		d2dx::AlwaysPrint(ssss); \
	}

	static __declspec(noreturn) void fatal(const char* msg)
	{
		d2dx::AlwaysPrint(msg);
		MessageBoxA(nullptr, msg, "D2DX Fatal Error", MB_OK | MB_ICONSTOP);
		TerminateProcess(GetCurrentProcess(), -1);
	}

	static void release_check(bool expr, const char* exprString)
	{
		if (!expr)
		{
			OutputDebugStringA(exprString);
			fatal(exprString);
		}
	}

	static void release_check_hr(HRESULT hr, const char* exprString)
	{
		if (FAILED(hr))
		{
			OutputDebugStringA(exprString);
			fatal(exprString);
		}
	}

	#define D2DX_RELEASE_CHECK(expr) release_check(expr, #expr)
	#define D2DX_RELEASE_CHECK_HR(expr) release_check_hr(expr, #expr)

	struct WindowsVersion 
	{
		uint32_t major;
		uint32_t minor;
		uint32_t build;
	};

	WindowsVersion GetWindowsVersion();

	Buffer<char> ReadTextFile(
		_In_z_ const char* filename);
}
