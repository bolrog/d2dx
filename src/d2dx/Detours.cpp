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

bool hasDetoured = false;
bool hasLateDetoured = false;

static IWin32InterceptionHandler* GetWin32InterceptionHandler()
{
	ID2DXContext* d2dxContext = D2DXContextFactory::GetInstance();

	if (!d2dxContext)
	{
		return nullptr;
	}

	return dynamic_cast<IWin32InterceptionHandler*>(d2dxContext);
}

static ID2InterceptionHandler* GetD2InterceptionHandler()
{
	return D2DXContextFactory::GetInstance();
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


typedef void(__stdcall* D2Gfx_DrawImageFunc)(D2::CellContext* pData, int nXpos, int nYpos, DWORD dwGamma, int nDrawMode, BYTE* pPalette);
typedef void(__stdcall* D2Gfx_DrawShiftedImageFunc)(D2::CellContext* pData, int nXpos, int nYpos, DWORD dwGamma, int nDrawMode, int nGlobalPaletteShift);
typedef void(__stdcall* D2Gfx_DrawVerticalCropImageFunc)(D2::CellContext* pData, int nXpos, int nYpos, int nSkipLines, int nDrawLines, int nDrawMode);
typedef void(__stdcall* D2Gfx_DrawClippedImageFunc)(D2::CellContext* pData, int nXpos, int nYpos, void* pCropRect, int nDrawMode);
typedef void(__stdcall* D2Gfx_DrawImageFastFunc)(D2::CellContext* pData, int nXpos, int nYpos, BYTE nPaletteIndex);
typedef void(__stdcall* D2Gfx_DrawShadowFunc)(D2::CellContext* pData, int nXpos, int nYpos);
typedef void(__fastcall* D2Win_DrawTextFunc)(const wchar_t* wStr, int xPos, int yPos, DWORD dwColor, DWORD dwUnk);
typedef uint32_t(__stdcall* D2Client_DrawUnitFunc)(D2::UnitAny* unit, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
typedef void* NakedFunc;

D2Gfx_DrawImageFunc D2Gfx_DrawImage_Real = nullptr;
D2Gfx_DrawShiftedImageFunc D2Gfx_DrawShiftedImage_Real = nullptr;
D2Gfx_DrawVerticalCropImageFunc D2Gfx_DrawVerticalCropImage_Real = nullptr;
D2Gfx_DrawClippedImageFunc D2Gfx_DrawClippedImage_Real = nullptr;
D2Gfx_DrawImageFastFunc D2Gfx_DrawImageFast_Real = nullptr;
D2Gfx_DrawShadowFunc D2Gfx_DrawShadow_Real = nullptr;
D2Win_DrawTextFunc D2Win_DrawText_Real = nullptr;
D2Client_DrawUnitFunc D2Client_DrawUnit_Real = nullptr;
D2Client_DrawUnitFunc D2Client_DrawMissile_Real = nullptr;
NakedFunc D2Client_DrawWeatherParticles_Real = nullptr;

void __stdcall D2Gfx_DrawImage_Hooked(
	D2::CellContext* cellContext,
	int nXpos,
	int nYpos,
	DWORD dwGamma,
	int nDrawMode,
	BYTE * pPalette)
{
	//D2DX_LOG("draw image %i, %i: '%p' nDrawMode %i, class %u, unit %u, dwMode %u, %p, %p, %p, %p, %p, %p", nXpos, nYpos, 
	//	cellContext->szName, 
	//	nDrawMode, 
	//	cellContext->dwClass, 
	//	cellContext->dwUnit,
	//	cellContext->dwMode,
	//	cellContext->_11,
	//	cellContext->_12,
	//	cellContext->_14,
	//	cellContext->_7,
	//	cellContext->_8,
	//	cellContext->_9);

	auto d2InterceptionHandler = GetD2InterceptionHandler();

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawImage(cellContext, { nXpos, nYpos });
	}

	D2Gfx_DrawImage_Real(cellContext, nXpos, nYpos, dwGamma, nDrawMode, pPalette);

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->EndDrawImage();
	}
}

void __stdcall D2Gfx_DrawClippedImage_Hooked(
	D2::CellContext* cellContext,
	int nXpos,
	int nYpos,
	void* pCropRect,
	int nDrawMode)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawImage(cellContext, { nXpos, nYpos });
	}

	D2Gfx_DrawClippedImage_Real(cellContext, nXpos, nYpos, pCropRect, nDrawMode);

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->EndDrawImage();
	}
}

void __stdcall D2Gfx_DrawShiftedImage_Hooked(
	D2::CellContext* cellContext,
	int nXpos, 
	int nYpos, 
	DWORD dwGamma, 
	int nDrawMode, 
	int nGlobalPaletteShift)
{
	/*
	  D2DX_LOG("draw shifted image %i, %i: '%p' nDrawMode %i, class %u, unit %u, dwMode %u, %p, %p, %p, %p, %p, %p",
		  nXpos,
		  nYpos, 
		  cellContext->szName, 
		  nDrawMode, 
		  cellContext->dwClass, 
		  cellContext->dwUnit,
		  cellContext->dwMode,
		  cellContext->_11,
		  cellContext->_12,
		  cellContext->_14,
		  cellContext->_7,
		  cellContext->_8,
		  cellContext->_9);*/

	auto d2InterceptionHandler = GetD2InterceptionHandler();

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawImage(cellContext, { nXpos, nYpos });
	}

	D2Gfx_DrawShiftedImage_Real(cellContext, nXpos, nYpos, dwGamma, nDrawMode, nGlobalPaletteShift);

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->EndDrawImage();
	}
}

void __stdcall D2Gfx_DrawVerticalCropImage_Hooked(
	D2::CellContext* cellContext,
	int nXpos, 
	int nYpos, 
	int nSkipLines, 
	int nDrawLines, 
	int nDrawMode)
{
	//D2DX_LOG("draw cropped image %i, %i: '%p' nDrawMode %i, class %u, unit %u, dwMode %u, %p, %p, %p, %p, %p, %p", nXpos, nYpos, pData->szName, nDrawMode, pData->dwClass, pData->dwUnit,
	//    pData->dwMode,
	//    pData->_11,
	//    pData->_12,
	//    pData->_14,
	//    pData->_7,
	//    pData->_8,
	//    pData->_9);

	auto d2InterceptionHandler = GetD2InterceptionHandler();

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawImage(cellContext, { nXpos, nYpos });
	}

	D2Gfx_DrawVerticalCropImage_Real(cellContext, nXpos, nYpos, nSkipLines, nDrawLines, nDrawMode);

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->EndDrawImage();
	}
}

void __stdcall D2Gfx_DrawImageFast_Hooked(
	D2::CellContext* cellContext,
	int nXpos,
	int nYpos,
	BYTE nPaletteIndex)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawImage(cellContext, { nXpos, nYpos });
	}

	D2Gfx_DrawImageFast_Real(cellContext, nXpos, nYpos, nPaletteIndex);

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->EndDrawImage();
	}
}

void __stdcall D2Gfx_DrawShadow_Hooked(
	D2::CellContext* cellContext,
	int nXpos,
	int nYpos)
{
	//D2DX_LOG("draw shadow %i, %i: '%p' class %u, unit %u, dwMode %u, %p, %p, %p, %p, %p, %p", nXpos, nYpos, pData->szName, pData->dwClass, pData->dwUnit,
	//    pData->dwMode,
	//    pData->_9,
	//    pData->_12,
	//    pData->_14,
	//    pData->_3,
	//    pData->_4,
	//    pData->_7);

	auto d2InterceptionHandler = GetD2InterceptionHandler();

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawImage(cellContext, { nXpos, nYpos });
	}

	D2Gfx_DrawShadow_Real(cellContext, nXpos, nYpos);

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->EndDrawImage();
	}
}

void __fastcall D2Win_DrawText_Hooked(
	const wchar_t* wStr,
	int xPos,
	int yPos,
	DWORD dwColor,
	DWORD dwUnk)
{
	auto d2InterceptionHandler = GetD2InterceptionHandler();

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->BeginDrawText();
	}

	D2Win_DrawText_Real(wStr, xPos, yPos, dwColor, dwUnk);

	if (d2InterceptionHandler)
	{
		d2InterceptionHandler->EndDrawText();
	}
}

D2::UnitAny* currentlyDrawingUnit = nullptr;
uint32_t currentlyDrawingWeatherParticles = 0;
uint32_t* currentlyDrawingWeatherParticleIndexPtr = nullptr;

__declspec(naked) void D2Client_DrawUnit_Stack_Hooked()
{
	static void* origReturnAddr = nullptr;

	__asm
	{
		push eax
		push edx
		lea edx, origReturnAddr
		mov eax, dword ptr [esp + 0x08]
		mov dword ptr[edx], eax
		lea eax, patchReturnAddr
		mov dword ptr [esp + 0x08], eax
		lea edx, currentlyDrawingUnit
		mov eax, dword ptr [esp + 0x0c]
		mov dword ptr[edx], eax
		pop edx
		pop eax
	}

	__asm jmp D2Client_DrawUnit_Real

patchReturnAddr:
	__asm
	{
		push eax
		push eax
		push edx
		lea edx, currentlyDrawingUnit
		xor eax, eax
		mov dword ptr [edx], eax
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

patchReturnAddr:
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

patchReturnAddr:
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
	IGameHelper * gameHelper)
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
	DetourAttach(&(PVOID&)D2Gfx_DrawImage_Real, D2Gfx_DrawImage_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawShiftedImage_Real, D2Gfx_DrawShiftedImage_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawVerticalCropImage_Real, D2Gfx_DrawVerticalCropImage_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawClippedImage_Real, D2Gfx_DrawClippedImage_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawImageFast_Real, D2Gfx_DrawImageFast_Hooked);
	DetourAttach(&(PVOID&)D2Gfx_DrawShadow_Real, D2Gfx_DrawShadow_Hooked);
	DetourAttach(&(PVOID&)D2Win_DrawText_Real, D2Win_DrawText_Hooked);

	DetourAttach(&(PVOID&)D2Client_DrawUnit_Real, 
		(gameHelper->GetVersion() == GameVersion::Lod109d || 
		gameHelper->GetVersion() == GameVersion::Lod110 ||
		gameHelper->GetVersion() == GameVersion::Lod114d) ? D2Client_DrawUnit_ESI_Hooked : D2Client_DrawUnit_Stack_Hooked);
	
	if (D2Client_DrawMissile_Real)
	{
		DetourAttach(&(PVOID&)D2Client_DrawMissile_Real, D2Client_DrawMissile_ESI_Hooked);
	}
	
	if (D2Client_DrawWeatherParticles_Real)
	{
		DetourAttach(&(PVOID&)D2Client_DrawWeatherParticles_Real, 
			gameHelper->GetVersion() == GameVersion::Lod114d ? D2Client_DrawWeatherParticles114d_Hooked : D2Client_DrawWeatherParticles_Hooked);
	}

	LONG lError = DetourTransactionCommit();

	if (lError != NO_ERROR) {
		D2DX_FATAL_ERROR("Failed to detour D2Gfx functions.");
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
