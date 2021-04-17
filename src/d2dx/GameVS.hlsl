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
#include "Constants.hlsli"
#include "Game.hlsli"

void main(
	in GameVSInput vs_in,
	out GameVSOutput vs_out)
{
	float2 unitPos = float2(vs_in.pos) * c_invScreenSize - 0.5;
	vs_out.pos = unitPos.xyxx * float4(2, -2, 0, 0) + float4(0, 0, 0, 1);
	vs_out.tc = vs_in.texCoord;
	vs_out.color = vs_in.color;
	vs_out.atlasIndex_paletteIndex_surfaceId_flags.x = vs_in.misc.x & 4095;
	vs_out.atlasIndex_paletteIndex_surfaceId_flags.y = (vs_in.misc.x >> 12) | ((vs_in.misc.y & 0x8000) ? 0x10 : 0);
	vs_out.atlasIndex_paletteIndex_surfaceId_flags.z = vs_in.misc.y & 16383;
	vs_out.atlasIndex_paletteIndex_surfaceId_flags.w = (vs_in.misc.y & 0x4000) ? 1 : 0;
}
