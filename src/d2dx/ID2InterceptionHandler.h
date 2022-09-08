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

#include "IGameHelper.h"
#include "Types.h"
#include "D2Types.h"

namespace d2dx
{
	struct ID2InterceptionHandler abstract
	{
		virtual ~ID2InterceptionHandler() noexcept {}

		virtual Offset BeginDrawText(
			_Inout_z_ wchar_t* str,
			_In_ Offset pos,
			_In_ uint32_t returnAddress,
			_In_ D2Function d2Function) = 0;

		virtual void EndDrawText() = 0;

		virtual Offset BeginDrawImage(
			_In_ const D2::CellContextAny* cellContext,
			_In_ uint32_t drawMode,
			_In_ Offset pos,
			_In_ D2Function d2Function) = 0;

		virtual void EndDrawImage() = 0;
	};
}
