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
#include "Detours.h"
#include "Utils.h"
#include "D2DXContextFactory.h"
#include "IWin32InterceptionHandler.h"

using namespace d2dx;

#pragma comment(lib, "../../thirdparty/detours/detours.lib")

bool hasDetoured = false;

static ComPtr<IWin32InterceptionHandler> GetWin32InterceptionHandler()
{
    ID2DXContext* d2dxContext = D2DXContextFactory::GetInstance();

    if (!d2dxContext)
    {
        return nullptr;
    }

    ComPtr<IWin32InterceptionHandler> handler;
    d2dxContext->QueryInterface(IID_PPV_ARGS(&handler));
    return handler;
}

COLORREF(WINAPI* GetPixel_real)(
    _In_ HDC hdc,
    _In_ int x,
    _In_ int y) = GetPixel;

COLORREF WINAPI GetPixel_Hooked(_In_ HDC hdc, _In_ int x, _In_ int y)
{
    /* Gets rid of the long delay on startup and when switching between menus in < 1.14,
       as the game is doing a ton of pixel readbacks... for some reason. */
    return 0;
}

int (WINAPI* ShowCursor_Real)(
    _In_ BOOL bShow) = ShowCursor;

int WINAPI ShowCursor_Hooked(
    _In_ BOOL bShow)
{
    /* Override how the game hides/shows the cursor. We will take care of that. */
    return bShow ? 1 : -1;
}

BOOL(WINAPI* SetWindowPos_Real)(
    _In_ HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_ int X,
    _In_ int Y,
    _In_ int cx,
    _In_ int cy,
    _In_ UINT uFlags) = SetWindowPos;

BOOL
WINAPI
SetWindowPos_Hooked(
    _In_ HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_ int X,
    _In_ int Y,
    _In_ int cx,
    _In_ int cy,
    _In_ UINT uFlags)
{
    /* Stop the game from moving/sizing the window. */
    return SetWindowPos_Real(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags | SWP_NOSIZE | SWP_NOMOVE);
}

BOOL
(WINAPI*
    SetCursorPos_Real)(
        _In_ int X,
        _In_ int Y) = SetCursorPos;

BOOL
WINAPI
SetCursorPos_Hooked(
    _In_ int X,
    _In_ int Y)
{
    auto win32InterceptionHandler = GetWin32InterceptionHandler();

    if (!win32InterceptionHandler)
    {
        return FALSE;
    }

    auto adjustedPos = win32InterceptionHandler->OnSetCursorPos({ X, Y });
    
    if (adjustedPos.x < 0)
    {
        return FALSE;
    }

    return SetCursorPos_Real(adjustedPos.x, adjustedPos.y);
}

LRESULT
(WINAPI*
    SendMessageA_Real)(
        _In_ HWND hWnd,
        _In_ UINT Msg,
        _Pre_maybenull_ _Post_valid_ WPARAM wParam,
        _Pre_maybenull_ _Post_valid_ LPARAM lParam) = SendMessageA;

LRESULT
WINAPI
SendMessageA_Hooked(
    _In_ HWND hWnd,
    _In_ UINT Msg,
    _Pre_maybenull_ _Post_valid_ WPARAM wParam,
    _Pre_maybenull_ _Post_valid_ LPARAM lParam)
{
    if (Msg == WM_MOUSEMOVE)
    {
        auto win32InterceptionHandler = GetWin32InterceptionHandler();

        if (!win32InterceptionHandler)
        {
            return 0;
        }

        auto x = GET_X_LPARAM(lParam);
        auto y = GET_Y_LPARAM(lParam);

        auto adjustedPos = win32InterceptionHandler->OnMouseMoveMessage({ x, y });

        if (adjustedPos.x < 0)
        {
            return 0;
        }

        lParam = ((uint16_t)adjustedPos.y << 16) | ((uint16_t)adjustedPos.x & 0xFFFF);
    }

    return SendMessageA_Real(hWnd, Msg, wParam, lParam);
}

_Success_(return)
int
WINAPI
DrawTextA_Hooked(
    _In_ HDC hdc,
    _When_((format & DT_MODIFYSTRING), _At_((LPSTR)lpchText, _Inout_grows_updates_bypassable_or_z_(cchText, 4)))
    _When_((!(format & DT_MODIFYSTRING)), _In_bypassable_reads_or_z_(cchText))
    LPCSTR lpchText,
    _In_ int cchText,
    _Inout_ LPRECT lprc,
    _In_ UINT format)
{
    /* This removes the "weird characters" being printed by the game in the top left corner.
       There is still a delay but the GetPixel hook takes care of that... */
    return 0;
}


_Success_(return)
int(
    WINAPI *
    DrawTextA_Real)(
        _In_ HDC hdc,
        _When_((format & DT_MODIFYSTRING), _At_((LPSTR)lpchText, _Inout_grows_updates_bypassable_or_z_(cchText, 4)))
        _When_((!(format & DT_MODIFYSTRING)), _In_bypassable_reads_or_z_(cchText))
        LPCSTR lpchText,
        _In_ int cchText,
        _Inout_ LPRECT lprc,
        _In_ UINT format) = DrawTextA;

void d2dx::AttachDetours()
{
    if (hasDetoured)
    {
        return;
    }

    hasDetoured = true;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)DrawTextA_Real, DrawTextA_Hooked);
    DetourAttach(&(PVOID&)GetPixel_real, GetPixel_Hooked);
    DetourAttach(&(PVOID&)SendMessageA_Real, SendMessageA_Hooked);
    DetourAttach(&(PVOID&)ShowCursor_Real, ShowCursor_Hooked);
    DetourAttach(&(PVOID&)SetCursorPos_Real, SetCursorPos_Hooked);
    DetourAttach(&(PVOID&)SetWindowPos_Real, SetWindowPos_Hooked);

    LONG lError = DetourTransactionCommit();

    if (lError != NO_ERROR) {
        fatal("Failed to detour Win32 functions.");
    }
}

void d2dx::DetachDetours()
{
    if (!hasDetoured)
    {
        return;
    }

    hasDetoured = false;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)DrawTextA_Real, DrawTextA_Hooked);
    DetourDetach(&(PVOID&)GetPixel_real, GetPixel_Hooked);
    DetourDetach(&(PVOID&)SendMessageA_Real, SendMessageA_Hooked);
    DetourDetach(&(PVOID&)ShowCursor_Real, ShowCursor_Hooked);
    DetourDetach(&(PVOID&)SetCursorPos_Real, SetCursorPos_Hooked);
    DetourDetach(&(PVOID&)SetWindowPos_Real, SetWindowPos_Hooked);

    LONG lError = DetourTransactionCommit();

    if (lError != NO_ERROR) {
        /* An error here doesn't really matter. The process is going. */
    }
}
