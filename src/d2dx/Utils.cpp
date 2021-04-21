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
#include "Utils.h"

#pragma comment(lib, "Netapi32.lib")

using namespace d2dx;

static bool _hasSetFreq = false;
static double _freq = 0.0;

static void warmup()
{
    if (_hasSetFreq)
        return;

    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    _freq = double(li.QuadPart) / 1000.0;
    _hasSetFreq = true;
}

int64_t d2dx::TimeStart()
{
    warmup();
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return (int64_t)li.QuadPart;
}

float d2dx::TimeEndMs(int64_t sinceThisTime)
{
    warmup();
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    assert(_freq);
    return (float)(double(li.QuadPart - sinceThisTime) / _freq);
}

typedef LONG NTSTATUS, * PNTSTATUS;
#define STATUS_SUCCESS (0x00000000)

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

static WindowsVersion* windowsVersion = nullptr;

WindowsVersion d2dx::GetWindowsVersion()
{
    if (windowsVersion)
    {
        return *windowsVersion;
    }

    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod)
    {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr != nullptr)
        {
            RTL_OSVERSIONINFOW rovi = { 0 };
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if (STATUS_SUCCESS == fxPtr(&rovi))
            {
                windowsVersion = new WindowsVersion();
                windowsVersion->major = rovi.dwMajorVersion;
                windowsVersion->minor = rovi.dwMinorVersion;
                windowsVersion->build = rovi.dwBuildNumber;
                return *windowsVersion;
            }
        }
    }
    WindowsVersion wv = { 0,0,0 };
    return wv;
}

WindowsVersion d2dx::GetActualWindowsVersion()
{
    LPSERVER_INFO_101 bufptr = nullptr;

    DWORD result = NetServerGetInfo(nullptr, 101, (LPBYTE*)&bufptr);

    if (!bufptr)
    {
        return { 0,0,0 };
    }

    WindowsVersion windowsVersion{ bufptr->sv101_version_major, bufptr->sv101_version_minor, 0 };
    NetApiBufferFree(bufptr);
    return windowsVersion;
}

static bool logFileOpened = false;
static FILE* logFile = 0;
static CRITICAL_SECTION logFileCS;

static void EnsureLogFileOpened()
{
    if (!logFileOpened)
    {
        logFileOpened = true;
        fopen_s(&logFile, "d2dx_log.txt", "w");
        InitializeCriticalSection(&logFileCS);
    }
}

static DWORD WINAPI WriteToLogFileWorkItemFunc(PVOID pvContext)
{
    char* s = (char*)pvContext;

    OutputDebugStringA(s);

    EnterCriticalSection(&logFileCS);

    fwrite(s, strlen(s), 1, logFile);
    fflush(logFile);

    LeaveCriticalSection(&logFileCS);

    free(s);

    return 0;
}

_Use_decl_annotations_
void d2dx::detail::Log(
    const char* s)
{
    EnsureLogFileOpened();
    QueueUserWorkItem(WriteToLogFileWorkItemFunc, _strdup(s), WT_EXECUTEDEFAULT);
}

_Use_decl_annotations_
Buffer<char> d2dx::ReadTextFile(
    const char* filename)
{
    FILE* cfgFile = nullptr;

    errno_t err = fopen_s(&cfgFile, "d2dx.cfg", "r");

    if (err < 0 || !cfgFile)
    {
        Buffer<char> str(1);
        str.items[0] = 0;
        return str;
    }

    fseek(cfgFile, 0, SEEK_END);

    long size = ftell(cfgFile);

    if (size <= 0)
    {
        Buffer<char> str(1);
        str.items[0] = 0;
        return str;
    }

    fseek(cfgFile, 0, SEEK_SET);

    Buffer<char> str(size + 1);

    fread_s(str.items, str.capacity, size, 1, cfgFile);
    
    str.items[size] = 0;

    fclose(cfgFile);

    return str;
}

_Use_decl_annotations_
char* d2dx::detail::GetMessageForHRESULT(
    HRESULT hr,
    const char* func,
    int32_t line) noexcept
{
    static Buffer<char> buffer(4096);
    auto msg = std::system_category().message(hr);
    sprintf_s(buffer.items, buffer.capacity, "%s line %i\nHRESULT: 0x%08x\nMessage: %s", func, line, hr, msg.c_str());
    return buffer.items;
}

_Use_decl_annotations_
void d2dx::detail::ThrowFromHRESULT(
    HRESULT hr,
    const char* func,
    int32_t line)
{
#ifndef NDEBUG
    __debugbreak();
#endif

    throw ComException(hr, func, line);
}

void d2dx::detail::FatalException() noexcept
{
    try
    {
        std::rethrow_exception(std::current_exception());
    }
    catch (const std::exception& e)
    {
        D2DX_LOG("%s", e.what());
        MessageBoxA(nullptr, e.what(), "D2DX Fatal Error", MB_OK | MB_ICONSTOP);
        TerminateProcess(GetCurrentProcess(), -1);
    }
}

_Use_decl_annotations_
void d2dx::detail::FatalError(
    const char* msg) noexcept
{
    D2DX_LOG("%s", msg);
    MessageBoxA(nullptr, msg, "D2DX Fatal Error", MB_OK | MB_ICONSTOP);
    TerminateProcess(GetCurrentProcess(), -1);
}
