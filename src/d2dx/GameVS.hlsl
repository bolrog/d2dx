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

void main(
	in int2 pos : POSITION,
	in int2 st : TEXCOORD0,
	in float4 color : COLOR0,
	in uint2 misc : TEXCOORD1,
	out PixelShaderInput psInput)
{
	float2 fpos = (2.0 * float2(pos) / screenSize) - 1.0;
	psInput.pos = float4(fpos.x, -fpos.y, 0.0, 1.0);
	psInput.tc = st;
	psInput.color = color;
	psInput.atlasIndex_paletteIndex_surfaceId_flags.x = misc.x & 4095;
	psInput.atlasIndex_paletteIndex_surfaceId_flags.y = (misc.x >> 12) | ((misc.y & 0x8000) ? 0x10 : 0);
	psInput.atlasIndex_paletteIndex_surfaceId_flags.z = misc.y & 16383;
	psInput.atlasIndex_paletteIndex_surfaceId_flags.w = (misc.y & 0x4000) ? 1 : 0;
}
