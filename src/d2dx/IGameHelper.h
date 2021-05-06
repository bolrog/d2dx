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
#include "D2Types.h"

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
		D2Client_DrawUnit = 7, /* push    52Bh */
		D2Client_DrawMissile = 8, 
		D2Client_FindClientSideUnit = 9,
		D2Client_FindServerSideUnit = 10,
	};

	struct DrawParameters
	{
		uint32_t unitId;
		uint32_t unitType;
		uint32_t unitToken;
	};

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

		virtual Offset GetPlayerTargetPos() const = 0;

		virtual void* GetFunction(
			_In_ D2Function function) const = 0;

		virtual DrawParameters GetDrawParameters(
			_In_ const D2::CellContext* cellContext) const = 0;

		virtual D2::UnitAny* GetPlayerUnit() const = 0;

		virtual Offset GetUnitPos(
			_In_ const D2::UnitAny* unit) const = 0;

		virtual D2::UnitType GetUnitType(
			_In_ const D2::UnitAny* unit) const = 0;

		virtual uint32_t GetUnitId(
			_In_ const D2::UnitAny* unit) const = 0;

		virtual D2::UnitAny* FindUnit(
			_In_ uint32_t unitId,
			_In_ D2::UnitType unitType) const = 0;
	};
}
