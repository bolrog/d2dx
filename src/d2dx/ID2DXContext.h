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

#include "IGlide3x.h"

namespace d2dx
{
	MIDL_INTERFACE("89F801A7-BB81-4729-BD97-AE9090E5612A")
		ID2DXContext : public IGlide3x
	{
		virtual void OnMousePosChanged(
			_In_ Offset pos) = 0;

		virtual void SetCustomResolution(
			_In_ Size size) = 0;

		virtual Size GetSuggestedCustomResolution() = 0;

		virtual GameVersion GetGameVersion() const = 0;

		virtual void DisableBuiltinResMod() = 0;
	};
}
