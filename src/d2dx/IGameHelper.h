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
	MIDL_INTERFACE("2C7585A9-7A8F-42B7-9FAC-C75C004AD8C3")
		IGameHelper : public IUnknown
	{
		virtual GameVersion GetVersion() const = 0;

		virtual _Ret_z_ const char* GetVersionString() const = 0;

		virtual uint32_t ScreenOpenMode() const = 0;

		virtual Size GetConfiguredGameSize() const = 0;

		virtual void SetIngameMousePos(
			_In_ Offset pos) = 0;

		virtual GameAddress IdentifyGameAddress(
			_In_ uint32_t returnAddress) const = 0;

		virtual TextureCategory GetTextureCategoryFromHash(
			_In_ uint32_t textureHash) const = 0;

		virtual TextureCategory RefineTextureCategoryFromGameAddress(
			_In_ TextureCategory previousCategory,
			_In_ GameAddress gameAddress) const = 0;

		virtual bool TryApplyFpsFix() = 0;
	};
}
