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

namespace d2dx
{
	MIDL_INTERFACE("D071E3C6-03D3-4C4C-A498-92C0A6C13447") 
		ISimd abstract : public IUnknown
	{
		virtual int32_t IndexOfUInt32(
			_In_reads_(itemsCount) const uint32_t* __restrict items,
			_In_ uint32_t itemsCount,
			_In_ uint32_t item) = 0;
	};
}