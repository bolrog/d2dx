#include "pch.h"
#include "D2DXDetours.h"

#pragma comment(lib, "../../thirdparty/detours/detours.lib")

bool hasDetoured = false;

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
    return FALSE;
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
        /* The game tries to move the mouse pointer using a SendMessage call in some situations.
           This screws up the cursor if windowscale != 1. So just drop the message. */
        return 0;
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
        MessageBox(HWND_DESKTOP, L"Failed to detour", L"timb3r", MB_OK);
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
        MessageBox(HWND_DESKTOP, L"Failed to detour", L"timb3r", MB_OK);
    }
}