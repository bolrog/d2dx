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
	in float2 pos : POSITION,
	in int2 tc : TEXCOORD,
	in float4 color : COLOR,
	out float2 o_tc : TEXCOORD0,
	out float4 o_pos : SV_POSITION)
{
	float2 fpos = (2.0 * pos / screenSize) - 1.0;
	
	o_pos = float4(fpos.x, -fpos.y, 0.0, 1.0);
	o_tc = tc;
}
