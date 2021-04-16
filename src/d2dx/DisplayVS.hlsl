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
	in float4 color : COLOR,
	in uint2 misc : TEXCOORD1,
	in uint vertexId : SV_VertexID,
	out noperspective float2 o_tc : TEXCOORD0,
	out nointerpolation float2 o_invTextureSize : TEXCOORD1,
	out noperspective float4 o_pos : SV_POSITION)
{
	float2 fpos = (2.0 * float2(pos) / screenSize) - 1.0;

	o_pos = float4(fpos.x, -fpos.y, 0.0, 1.0);

	int srcWidth = misc.y & 16383;
	int srcHeight = misc.x & 4095;
	o_invTextureSize = 1.0 / st;

	switch (vertexId)
	{
	default:
	case 0:
		o_tc = float2(0, 0);
		break;
	case 1:
		o_tc = float2(srcWidth * 2 * o_invTextureSize.x, 0);
		break;
	case 2:
		o_tc = float2(0, srcHeight * 2 * o_invTextureSize.y);
		break;
	}
}
