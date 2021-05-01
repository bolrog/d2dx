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

#include "Types.h"

namespace d2dx
{
	enum class D2Function
	{
		D2Gfx_DrawImage = 0, /* call dword ptr [eax+84h] */
		D2Gfx_DrawShiftedImage = 1, /* call dword ptr [eax+88h] */
		D2Gfx_DrawVerticalCropImage = 2, /* call dword ptr [eax+8Ch] */
		D2Gfx_DrawClippedImage = 3, /* call dword ptr [eax+98h] */
		D2Gfx_DrawImageFast = 4, /* call dword ptr [eax+94h] */
		D2Gfx_DrawShadow = 5, /* call dword ptr [eax+90h] */
		D2Win_DrawText = 6, /* mov ebx, [esp+4+arg_8] */
	};

	struct CellContext		//size 0x48
	{
		DWORD nCellNo;					//0x00
		DWORD _0a;						//0x04
		DWORD dwUnit;					//0x08
		DWORD dwClass;					//0x0C
		DWORD dwMode;					//0x10
		DWORD _3;						//0x14
		DWORD dwPlayerType;				//0x18
		BYTE _5;						//0x1C
		BYTE _5a;						//0x1D
		WORD _6;						//0x1E
		DWORD _7;						//0x20
		DWORD _8;						//0x24
		DWORD _9;						//0x28
		char* szName;					//0x2C
		DWORD _11;						//0x30
		void* pCellFile;			//0x34 also pCellInit
		DWORD _12;						//0x38
		void* pGfxCells;				//0x3C
		DWORD direction;				//0x40
		DWORD _14;						//0x44
	};

	static_assert(sizeof(CellContext) == 0x48, "CellContext size");

	struct IGameHelper abstract
	{
		virtual ~IGameHelper() noexcept {}

		virtual GameVersion GetVersion() const = 0;

		virtual _Ret_z_ const char* GetVersionString() const = 0;

		virtual uint32_t ScreenOpenMode() const = 0;

		virtual Size GetConfiguredGameSize() const = 0;

		virtual GameAddress IdentifyGameAddress(
			_In_ uint32_t returnAddress) const = 0;

		virtual TextureCategory GetTextureCategoryFromHash(
			_In_ uint32_t textureHash) const = 0;

		virtual TextureCategory RefineTextureCategoryFromGameAddress(
			_In_ TextureCategory previousCategory,
			_In_ GameAddress gameAddress) const = 0;

		virtual bool TryApplyFpsFix() = 0;

		virtual Offset GetPlayerPos()  = 0;

		virtual Offset GetPlayerTargetPos() const = 0;

		virtual void* GetFunction(
			_In_ D2Function function) const = 0;
	};
}
