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
#include "IGameHelper.h"

using namespace d2dx;

#pragma comment(lib, "../../thirdparty/detours/detours.lib")

static bool hasDetoured = false;
static bool hasDetachedDetours = false;
static bool hasLateDetoured = false;
static bool hasDetachedLateDetours = false;

D2::UnitAny* currentlyDrawingUnit = nullptr;
uint32_t currentlyDrawingWeatherParticles = 0;
uint32_t* currentlyDrawingWeatherParticleIndexPtr = nullptr;

static IWin32InterceptionHandler* GetWin32InterceptionHandler()
{
	ID2DXContext* d2dxContext = D2DXContextFactory::GetInstance(false);

	if (!d2dxContext)
	{
		return nullptr;
	}

	return static_cast<IWin32InterceptionHandler*>(d2dxContext);
}

static ID2InterceptionHandler* GetD2InterceptionHandler()
{
	return D2DXContextFactory::GetInstance(false);
}

static thread_local bool isInSleepCall = false;

static VOID
(WINAPI*
Sleep_Real)(
	_In_ DWORD dwMilliseconds
) = Sleep;

static VOID WINAPI Sleep_Hooked(
	_In_ DWORD dwMilliseconds)
{
	auto win32InterceptionHandler = GetWin32InterceptionHandler();
	if (win32InterceptionHandler)
	{
		int32_t adjustedMs = win32InterceptionHandler->OnSleep((int32_t)dwMilliseconds);

		if (adjustedMs >= 0)
		{
			isInSleepCall = true;
			Sleep_Real((DWORD)adjustedMs);
			isInSleepCall = false;
		}
	}
	else
	{
		isInSleepCall = true;
		Sleep_Real(dwMilliseconds);
		isInSleepCall = false;
	}
}

static DWORD
(WINAPI*
	SleepEx_Real)(
		_In_ DWORD dwMilliseconds,
		_In_ BOOL bAlertable
		) = SleepEx;

static DWORD WINAPI SleepEx_Hooked(
	_In_ DWORD dwMilliseconds,
	_In_ BOOL bAlertable)
{
	if (isInSleepCall)
	{
		return SleepEx_Real(dwMilliseconds, bAlertable);
	}

	auto win32InterceptionHandler = GetWin32InterceptionHandler();
	if (win32InterceptionHandler)
	{
		int32_t adjustedMs = win32InterceptionHandler->OnSleep((int32_t)dwMilliseconds);

		if (adjustedMs >= 0)
		{
			return SleepEx_Real((DWORD)adjustedMs, bAlertable);
		}
	}
	else
	{
		return SleepEx_Real(dwMilliseconds, bAlertable);
	}

	return 0;
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


typedef void(__stdcall* D2Gfx_DrawImageFunc)(D2::CellContextAny* pData, int nXpos, int nYpos, DWORD dwGamma, int nDrawMode, BYTE* pPalette);
typedef void(__stdcall* D2Gfx_DrawShiftedImageFunc)(D2::CellContextAny* pData, int nXpos, int nYpos, DWORD dwGamma, int nDrawMode, int nGlobalPaletteShift);
typedef void(__stdcall* D2Gfx_DrawVerticalCropImageFunc)(D2::CellContextAny* pData, int nXpos, int nYpos, int nSkipLines, int nDrawLines, int nDrawMode);
typedef void(__stdcall* D2Gfx_DrawClippedImageFunc)(D2::CellContextAny* pData, int nXpos, int nYpos, void* pCropRect, int nDrawMode);
typedef void(__stdcall* D2Gfx_DrawImageFastFunc)(D2::CellContextAny* pData, int nXpos, int nYpos, BYTE nPaletteIndex);
typedef void(__stdcall* D2Gfx_DrawShadowFunc)(D2::CellContextAny* pData, int nXpos, int nYpos);
typedef void(__fastcall* D2Win_DrawTextFunc)(const wchar_t* wStr, int xPos, int yPos, DWORD dwColor, DWORD centered);
typedef void(__fastcall* D2Win_DrawTextExFunc)(const wchar_t* wStr, int xPos, int yPos, DWORD dwColor, DWORD centered, DWORD transparencyLevel);
typedef void(__fastcall* D2Win_DrawFramedTextFunc_109)(const wchar_t* wStr, int xPos, int yPos, DWORD dwColor, DWORD centered, DWORD);
typedef void(__fastcall* D2Win_DrawFramedTextFunc_112)(const wchar_t* wStr, int xPos, int yPos, DWORD dwColor, DWORD centered);
typedef void(__fastcall* D2Win_DrawRectangledTextFunc)(const wchar_t* wStr, int xPos, int yPos, DWORD rectColor, DWORD rectTransparency, DWORD color);
typedef uint32_t(__stdcall* D2Client_DrawUnitFunc)(D2::UnitAny* unit, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
typedef void* NakedFunc;

D2Gfx_DrawImageFunc D2Gfx_DrawImage_Real = nullptr;
D2Gfx_DrawShiftedImageFunc D2Gfx_DrawShiftedImage_Real = nullptr;
D2Gfx_DrawVerticalCropImageFunc D2Gfx_DrawVerticalCropImage_Real = nullptr;
D2Gfx_DrawClippedImageFunc D2Gfx_DrawClippedImage_Real = nullptr;
D2Gfx_DrawImageFastFunc D2Gfx_DrawImageFast_Real = nullptr;
D2Gfx_DrawShadowFunc D2Gfx_DrawShadow_Real = nullptr;
D2Win_DrawTextFunc D2Win_DrawText_Real = nullptr;
//D2Win_DrawTextExFunc D2Win_DrawTextEx_Real = nullptr;
D2Win_DrawFramedTextFunc_109 D2Win_DrawFramedText_109_Real = nullptr;
D2Win_DrawFramedTextFunc_112 D2Win_DrawFramedText_112_Real = nullptr;
D2Win_DrawRectangledTextFunc D2Win_DrawRectangledText_Real = nullptr;
D2Client_DrawUnitFunc D2Client_DrawUnit_Real = nullptr;
D2Client_DrawUnitFunc D2Client_DrawMissile_Real = nullptr;
NakedFunc D2Client_DrawWeatherParticles_Real = nullptr;

void __stdcall D2Gfx_DrawImage_Hooked(
	D2::CellContextAny* cellContext,
	int nXpos,
	int nYpos,
	DWORD dwGamma,
	int nDrawMode,
	BYTE * pPalette)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();
	if (d2InterceptionHandler)
	{
		auto offset = d2InterceptionHandler->BeginDrawImage(cellContext, nDrawMode, { nXpos, nYpos }, D2Function::D2Gfx_DrawImage);
		D2Gfx_DrawImage_Real(cellContext, nXpos + offset.x, nYpos + offset.y, dwGamma, nDrawMode, pPalette);
		d2InterceptionHandler->EndDrawImage();
	}
	else
	{
		D2Gfx_DrawImage_Real(cellContext, nXpos, nYpos, dwGamma, nDrawMode, pPalette);
	}
}

void __stdcall D2Gfx_DrawClippedImage_Hooked(
	D2::CellContextAny* cellContext,
	int nXpos,
	int nYpos,
	void* pCropRect,
	int nDrawMode)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();
	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawImage(cellContext, (uint32_t)nDrawMode, { nXpos, nYpos }, D2Function::D2Gfx_DrawClippedImage);
		D2Gfx_DrawClippedImage_Real(cellContext, nXpos, nYpos, pCropRect, nDrawMode);
		d2InterceptionHandler->EndDrawImage();
	}
	else
	{
		D2Gfx_DrawClippedImage_Real(cellContext, nXpos, nYpos, pCropRect, nDrawMode);
	}
}

void __stdcall D2Gfx_DrawShiftedImage_Hooked(
	D2::CellContextAny* cellContext,
	int nXpos,
	int nYpos,
	DWORD dwGamma,
	int nDrawMode,
	int nGlobalPaletteShift)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();
	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawImage(cellContext, (uint32_t)nDrawMode, { nXpos, nYpos }, D2Function::D2Gfx_DrawShiftedImage);
		D2Gfx_DrawShiftedImage_Real(cellContext, nXpos, nYpos, dwGamma, nDrawMode, nGlobalPaletteShift);
		d2InterceptionHandler->EndDrawImage();
	}
	else
	{
		D2Gfx_DrawShiftedImage_Real(cellContext, nXpos, nYpos, dwGamma, nDrawMode, nGlobalPaletteShift);
	}
}

void __stdcall D2Gfx_DrawVerticalCropImage_Hooked(
	D2::CellContextAny* cellContext,
	int nXpos,
	int nYpos,
	int nSkipLines,
	int nDrawLines,
	int nDrawMode)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();
	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawImage(cellContext, (uint32_t)nDrawMode, { nXpos, nYpos }, D2Function::D2Gfx_DrawVerticalCropImage);
		D2Gfx_DrawVerticalCropImage_Real(cellContext, nXpos, nYpos, nSkipLines, nDrawLines, nDrawMode);
		d2InterceptionHandler->EndDrawImage();
	}
	else
	{
		D2Gfx_DrawVerticalCropImage_Real(cellContext, nXpos, nYpos, nSkipLines, nDrawLines, nDrawMode);
	}
}

void __stdcall D2Gfx_DrawImageFast_Hooked(
	D2::CellContextAny* cellContext,
	int nXpos,
	int nYpos,
	BYTE nPaletteIndex)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();
	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawImage(cellContext, (uint32_t)-1, { nXpos, nYpos }, D2Function::D2Gfx_DrawImageFast);
		D2Gfx_DrawImageFast_Real(cellContext, nXpos, nYpos, nPaletteIndex);
		d2InterceptionHandler->EndDrawImage();
	}
	else
	{
		D2Gfx_DrawImageFast_Real(cellContext, nXpos, nYpos, nPaletteIndex);
	}
}

void __stdcall D2Gfx_DrawShadow_Hooked(
	D2::CellContextAny* cellContext,
	int nXpos,
	int nYpos)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();

	if (d2InterceptionHandler)
	{
		auto offset = d2InterceptionHandler->BeginDrawImage(cellContext, (uint32_t)-1, { nXpos, nYpos }, D2Function::D2Gfx_DrawShadow);
		D2Gfx_DrawShadow_Real(cellContext, nXpos + offset.x, nYpos + offset.y);
		d2InterceptionHandler->EndDrawImage();
	}
	else
	{
		D2Gfx_DrawShadow_Real(cellContext, nXpos, nYpos);
	}
}

void __fastcall D2Win_DrawText_Hooked(
	wchar_t* wStr,
	int xPos,
	int yPos,
	DWORD dwColor,
	DWORD centered)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();
	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawText(wStr, { 0,0 }, (uint32_t)(uintptr_t)_ReturnAddress(), D2Function::D2Win_DrawText);
		D2Win_DrawText_Real(wStr, xPos, yPos, dwColor, centered);
		d2InterceptionHandler->EndDrawText();
	}
	else
	{
		D2Win_DrawText_Real(wStr, xPos, yPos, dwColor, centered);
	}
}

//void __fastcall D2Win_DrawTextEx_Hooked(
//	const wchar_t* wStr,
//	int xPos,
//	int yPos,
//	DWORD dwColor,
//	DWORD centered,
//	DWORD transparency)
//{
//	auto d2InterceptionHandler = GetD2InterceptionHandler();
//
//	if (d2InterceptionHandler)
//	{
//		d2InterceptionHandler->BeginDrawText();
//	}
//
//	D2Win_DrawTextEx_Real(wStr, xPos, yPos, dwColor, centered, transparency);
//
//	if (d2InterceptionHandler)
//	{
//		d2InterceptionHandler->EndDrawText();
//	}
//}

void __fastcall D2Win_DrawFramedText_109_Hooked(
	wchar_t* wStr,
	int xPos,
	int yPos,
	DWORD dwColor,
	DWORD centered,
	DWORD arg)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();
	if (d2InterceptionHandler)
	{
		auto offset = d2InterceptionHandler->BeginDrawText(wStr, { xPos, yPos }, (uint32_t)(uintptr_t)_ReturnAddress(), D2Function::D2Win_DrawFramedText);
		D2Win_DrawFramedText_109_Real(wStr, xPos + offset.x, yPos + offset.y, dwColor, centered, arg);
		d2InterceptionHandler->EndDrawText();
	}
	else
	{
		D2Win_DrawFramedText_109_Real(wStr, xPos, yPos, dwColor, centered, arg);
	}
}

void __fastcall D2Win_DrawFramedText_112_Hooked(
	wchar_t* wStr,
	int xPos,
	int yPos,
	DWORD dwColor,
	DWORD centered)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();
	if (d2InterceptionHandler)
	{
		auto offset = d2InterceptionHandler->BeginDrawText(wStr, { xPos, yPos }, (uint32_t)(uintptr_t)_ReturnAddress(), D2Function::D2Win_DrawFramedText);
		D2Win_DrawFramedText_112_Real(wStr, xPos + offset.x, yPos + offset.y, dwColor, centered);
		d2InterceptionHandler->EndDrawText();
	}
	else
	{
		D2Win_DrawFramedText_112_Real(wStr, xPos, yPos, dwColor, centered);
	}
}

void __fastcall D2Win_DrawRectangledText_Hooked(
	wchar_t* wStr,
	int xPos,
	int yPos,
	DWORD rectColor,
	DWORD rectTransparency,
	DWORD color)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();
	if (d2InterceptionHandler)
	{
		auto offset = d2InterceptionHandler->BeginDrawText(wStr, { xPos, yPos }, (uint32_t)(uintptr_t)_ReturnAddress(), D2Function::D2Win_DrawRectangledText);
		D2Win_DrawRectangledText_Real(wStr, xPos + offset.x, yPos + offset.y, rectColor, rectTransparency, color);
		d2InterceptionHandler->EndDrawText();
	}
	else
	{
		D2Win_DrawRectangledText_Real(wStr, xPos, yPos, rectColor, rectTransparency, color);
	}
}

__declspec(naked) void D2Client_DrawUnit_Stack_Hooked()
{
	static void* origReturnAddr = nullptr;

	__asm
	{
		push eax
		push edx
		lea edx, origReturnAddr
		mov eax, dword ptr[esp + 0x08]
		mov dword ptr[edx], eax
		lea eax, patchReturnAddr
		mov dword ptr[esp + 0x08], eax
		lea edx, currentlyDrawingUnit
		mov eax, dword ptr[esp + 0x0c]
		mov dword ptr[edx], eax
		pop edx
		pop eax
	}

	__asm jmp D2Client_DrawUnit_Real

	patchReturnAddr :
	__asm
	{
		push eax
		push eax
		push edx
		lea edx, currentlyDrawingUnit
		xor eax, eax
		mov dword ptr[edx], eax
		lea edx, origReturnAddr
		mov eax, dword ptr[edx]
		mov dword ptr[esp + 0x08], eax
		pop edx
		pop eax
		ret
	}
}

__declspec(naked) void D2Client_DrawUnit_ESI_Hooked()
{
	static void* origReturnAddr = nullptr;

	__asm
	{
		push eax
		push edx
		lea edx, origReturnAddr
		mov eax, dword ptr[esp + 0x08]
		mov dword ptr[edx], eax
		lea eax, patchReturnAddr
		mov dword ptr[esp + 0x08], eax
		lea edx, currentlyDrawingUnit
		mov dword ptr[edx], esi
		pop edx
		pop eax
	}

	__asm jmp D2Client_DrawUnit_Real

	patchReturnAddr :
	__asm
	{
		push eax
		push eax
		push edx
		lea edx, currentlyDrawingUnit
		xor eax, eax
		mov dword ptr[edx], eax
		lea edx, origReturnAddr
		mov eax, dword ptr[edx]
		mov dword ptr[esp + 0x08], eax
		pop edx
		pop eax
		ret
	}
}

__declspec(naked) void D2Client_DrawMissile_ESI_Hooked()
{
	static void* origReturnAddr = nullptr;

	__asm
	{
		push eax
		push edx
		lea edx, origReturnAddr
		mov eax, dword ptr[esp + 0x08]
		mov dword ptr[edx], eax
		lea eax, patchReturnAddr
		mov dword ptr[esp + 0x08], eax
		lea edx, currentlyDrawingUnit
		mov dword ptr[edx], esi
		pop edx
		pop eax
	}

	__asm jmp D2Client_DrawMissile_Real

	patchReturnAddr :
	__asm
	{
		push eax
		push eax
		push edx
		lea edx, currentlyDrawingUnit
		xor eax, eax
		mov dword ptr[edx], eax
		lea edx, origReturnAddr
		mov eax, dword ptr[edx]
		mov dword ptr[esp + 0x08], eax
		pop edx
		pop eax
		ret
	}
}

__declspec(naked) void D2Client_DrawWeatherParticles_Hooked()
{
	static void* origReturnAddr = nullptr;

	__asm
	{
		push eax
		push edx
		lea edx, origReturnAddr
		mov eax, dword ptr[esp + 0x08]
		mov dword ptr[edx], eax
		lea eax, patchReturnAddr
		mov dword ptr[esp + 0x08], eax
		mov eax, 1
		lea edx, currentlyDrawingWeatherParticles
		mov dword ptr[edx], eax
		lea edx, currentlyDrawingWeatherParticleIndexPtr
		mov eax, esp
		add eax, 4
		mov dword ptr[edx], eax
		pop edx
		pop eax
	}

	__asm jmp D2Client_DrawWeatherParticles_Real

	patchReturnAddr :
	__asm
	{
		push eax
		push eax
		push edx
		lea edx, currentlyDrawingWeatherParticles
		xor eax, eax
		mov dword ptr[edx], eax
		lea edx, origReturnAddr
		mov eax, dword ptr[edx]
		mov dword ptr[esp + 0x08], eax
		pop edx
		pop eax
		ret
	}
}

__declspec(naked) void D2Client_DrawWeatherParticles114d_Hooked()
{
	static void* origReturnAddr = nullptr;

	__asm
	{
		push eax
		push edx
		lea edx, origReturnAddr
		mov eax, dword ptr[esp + 0x08]
		mov dword ptr[edx], eax
		lea eax, patchReturnAddr
		mov dword ptr[esp + 0x08], eax
		mov eax, 1
		lea edx, currentlyDrawingWeatherParticles
		mov dword ptr[edx], eax
		lea edx, currentlyDrawingWeatherParticleIndexPtr
		mov eax, esp
		sub eax, 10h
		mov dword ptr[edx], eax
		pop edx
		pop eax
	}

	__asm jmp D2Client_DrawWeatherParticles_Real

	patchReturnAddr :
	__asm
	{
		push eax
		push eax
		push edx
		lea edx, currentlyDrawingWeatherParticles
		xor eax, eax
		mov dword ptr[edx], eax
		lea edx, origReturnAddr
		mov eax, dword ptr[edx]
		mov dword ptr[esp + 0x08], eax
		pop edx
		pop eax
		ret
	}
}

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
	auto r = DetourAttach(&(PVOID&)SetWindowPos_Real, SetWindowPos_Hooked);

	LONG lError = DetourTransactionCommit();

	if (lError != NO_ERROR) {
		D2DX_FATAL_ERROR("Failed to detour Win32 functions.");
	}
}

_Use_decl_annotations_
void d2dx::AttachLateDetours(
	IGameHelper* gameHelper,
	ID2DXContext* d2dxContext)
{
	if (hasLateDetoured)
	{
		return;
	}

	hasLateDetoured = true;

	D2Gfx_DrawImage_Real = (D2Gfx_DrawImageFunc)gameHelper->GetFunction(D2Function::D2Gfx_DrawImage);
	D2Gfx_DrawShiftedImage_Real = (D2Gfx_DrawShiftedImageFunc)gameHelper->GetFunction(D2Function::D2Gfx_DrawShiftedImage);
	D2Gfx_DrawVerticalCropImage_Real = (D2Gfx_DrawVerticalCropImageFunc)gameHelper->GetFunction(D2Function::D2Gfx_DrawVerticalCropImage);
	D2Gfx_DrawClippedImage_Real = (D2Gfx_DrawClippedImageFunc)gameHelper->GetFunction(D2Function::D2Gfx_DrawClippedImage);
	D2Gfx_DrawImageFast_Real = (D2Gfx_DrawImageFastFunc)gameHelper->GetFunction(D2Function::D2Gfx_DrawImageFast);
	D2Gfx_DrawShadow_Real = (D2Gfx_DrawShadowFunc)gameHelper->GetFunction(D2Function::D2Gfx_DrawShadow);
	D2Win_DrawText_Real = (D2Win_DrawTextFunc)gameHelper->GetFunction(D2Function::D2Win_DrawText);
	//D2Win_DrawTextEx_Real = (D2Win_DrawTextExFunc)gameHelper->GetFunction(D2Function::D2Win_DrawTextEx);
	D2Win_DrawRectangledText_Real = (D2Win_DrawRectangledTextFunc)gameHelper->GetFunction(D2Function::D2Win_DrawRectangledText);
	D2Client_DrawUnit_Real = (D2Client_DrawUnitFunc)gameHelper->GetFunction(D2Function::D2Client_DrawUnit);
	D2Client_DrawMissile_Real = (D2Client_DrawUnitFunc)gameHelper->GetFunction(D2Function::D2Client_DrawMissile);
	D2Client_DrawWeatherParticles_Real = (NakedFunc)gameHelper->GetFunction(D2Function::D2Client_DrawWeatherParticles);

	if (!D2Gfx_DrawImage_Real ||
		!D2Gfx_DrawShiftedImage_Real ||
		!D2Gfx_DrawVerticalCropImage_Real ||
		!D2Gfx_DrawClippedImage_Real ||
		!D2Gfx_DrawImageFast_Real ||
		!D2Gfx_DrawShadow_Real ||
		!D2Win_DrawText_Real ||
		!D2Client_DrawUnit_Real)
	{
		return;
	}

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)Sleep_Real, Sleep_Hooked);
	DetourAttach(&(PVOID&)SleepEx_Real, SleepEx_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawImage_Real, D2Gfx_DrawImage_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawShiftedImage_Real, D2Gfx_DrawShiftedImage_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawVerticalCropImage_Real, D2Gfx_DrawVerticalCropImage_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawClippedImage_Real, D2Gfx_DrawClippedImage_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawImageFast_Real, D2Gfx_DrawImageFast_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawShadow_Real, D2Gfx_DrawShadow_Hooked);
	DetourAttach(&(PVOID&)D2Win_DrawText_Real, D2Win_DrawText_Hooked);
	//DetourAttach(&(PVOID&)D2Win_DrawTextEx_Real, D2Win_DrawTextEx_Hooked);

	switch (gameHelper->GetVersion()) {
	case GameVersion::Lod109d:
	case GameVersion::Lod110f:
		D2Win_DrawFramedText_109_Real = (D2Win_DrawFramedTextFunc_109)gameHelper->GetFunction(D2Function::D2Win_DrawFramedText);
		if (D2Win_DrawFramedText_109_Real) {
			DetourAttach(&(PVOID&)D2Win_DrawFramedText_109_Real, D2Win_DrawFramedText_109_Hooked);
		}
		break;
	case GameVersion::Lod112:
	case GameVersion::Lod113c:
	case GameVersion::Lod113d:
	case GameVersion::Lod114d:
		D2Win_DrawFramedText_112_Real = (D2Win_DrawFramedTextFunc_112)gameHelper->GetFunction(D2Function::D2Win_DrawFramedText);
		if (D2Win_DrawFramedText_112_Real) {
			DetourAttach(&(PVOID&)D2Win_DrawFramedText_112_Real, D2Win_DrawFramedText_112_Hooked);
		}
		break;
	}

	if (D2Win_DrawRectangledText_Real)
	{
		DetourAttach(&(PVOID&)D2Win_DrawRectangledText_Real, D2Win_DrawRectangledText_Hooked);
	}

	if (d2dxContext->IsFeatureEnabled(Feature::UnitMotionPrediction))
	{
		assert(D2Client_DrawUnit_Real);
		DetourAttach(&(PVOID&)D2Client_DrawUnit_Real,
			(gameHelper->GetVersion() == GameVersion::Lod109d ||
			 gameHelper->GetVersion() == GameVersion::Lod110f ||
			 gameHelper->GetVersion() == GameVersion::Lod114d) ? D2Client_DrawUnit_ESI_Hooked : D2Client_DrawUnit_Stack_Hooked);
	
		if (gameHelper->GetVersion() != GameVersion::Lod109d && gameHelper->GetVersion() != GameVersion::Lod110f)
		{
			assert(D2Client_DrawMissile_Real);
			DetourAttach(&(PVOID&)D2Client_DrawMissile_Real, D2Client_DrawMissile_ESI_Hooked);
		}
	}

	if (d2dxContext->IsFeatureEnabled(Feature::WeatherMotionPrediction))
	{
		assert(D2Client_DrawWeatherParticles_Real);
		DetourAttach(&(PVOID&)D2Client_DrawWeatherParticles_Real,
			gameHelper->GetVersion() == GameVersion::Lod114d ? D2Client_DrawWeatherParticles114d_Hooked : D2Client_DrawWeatherParticles_Hooked);
	}

	LONG lError = DetourTransactionCommit();

	if (lError != NO_ERROR) {
		D2DX_LOG("Failed to detour D2Gfx functions: %i.", lError);
		D2DX_FATAL_ERROR("Failed to detour D2Gfx functions");
	}
}

void d2dx::DetachLateDetours()
{
	if (!hasLateDetoured || hasDetachedLateDetours)
	{
		return;
	}

	hasDetachedLateDetours = true;
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach(&(PVOID&)D2Gfx_DrawImage_Real, D2Gfx_DrawImage_Hooked);
	DetourDetach(&(PVOID&)D2Gfx_DrawShiftedImage_Real, D2Gfx_DrawShiftedImage_Hooked);
	DetourDetach(&(PVOID&)D2Gfx_DrawVerticalCropImage_Real, D2Gfx_DrawVerticalCropImage_Hooked);
	DetourDetach(&(PVOID&)D2Gfx_DrawClippedImage_Real, D2Gfx_DrawClippedImage_Hooked);
	DetourDetach(&(PVOID&)D2Gfx_DrawImageFast_Real, D2Gfx_DrawImageFast_Hooked);
	DetourDetach(&(PVOID&)D2Gfx_DrawShadow_Real, D2Gfx_DrawShadow_Hooked);
	DetourDetach(&(PVOID&)D2Win_DrawText_Real, D2Win_DrawText_Hooked);
	DetourDetach(&(PVOID&)D2Win_DrawFramedText_109_Real, D2Win_DrawFramedText_109_Hooked);
	DetourDetach(&(PVOID&)D2Win_DrawFramedText_112_Real, D2Win_DrawFramedText_112_Hooked);
	DetourDetach(&(PVOID&)D2Win_DrawRectangledText_Real, D2Win_DrawRectangledText_Hooked);
	DetourDetach(&(PVOID&)D2Client_DrawMissile_Real, D2Client_DrawMissile_ESI_Hooked);
	DetourDetach(&(PVOID&)Sleep_Real, Sleep_Hooked);
	DetourDetach(&(PVOID&)SleepEx_Real, SleepEx_Hooked);

	LONG lError = DetourTransactionCommit();

	if (lError != NO_ERROR) {
		/* An error here doesn't really matter. The process is going. */
	}
}

void d2dx::DetachDetours()
{ 
	if (!hasDetoured || hasDetachedDetours)
	{
		return;
	}

	hasDetachedDetours = true;

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
