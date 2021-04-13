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

PixelShaderInput main(
	in float2 pos : POSITION,
	in uint s_t_batchIndex : TEXCOORD0,
	in float4 color : COLOR0,
	in uint2 misc : TEXCOORD1)
{
	float2 fpos = (2.0 * pos / screenSize) - 1.0;

	int s = s_t_batchIndex & 511;
	int t = (s_t_batchIndex >> 9) & 511;
	int batchIndex = s_t_batchIndex >> 18;

	PixelShaderInput psInput;
	psInput.pos = float4(fpos.x, -fpos.y, 0.0, 1.0);
	psInput.tc = float2(s, t);
	psInput.color = color;
	psInput.misc.xy = misc;
	psInput.misc.z = batchIndex;
	return psInput;
}
