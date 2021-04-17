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
#include "Display.hlsli"

void main(
	in DisplayVSInput vs_in,
	uint vs_in_vertexId : SV_VertexID,
	out DisplayVSOutput vs_out,
	out noperspective float4 vs_out_pos : SV_POSITION)	
{
	float2 fpos = float2(vs_in.pos) * c_invScreenSize - 0.5;
	vs_out_pos = fpos.xyxx * float4(2, -2, 0, 0) + float4(0,0,0,1);
	
	float2 stf = vs_in.st;
	vs_out.textureSize_invTextureSize = float4(stf, 1/stf);

	const float srcWidth = vs_in.misc.y & 16383;
	const float srcHeight = vs_in.misc.x & 4095;

	switch (vs_in_vertexId)
	{
	default:
	case 0:
		vs_out.tc = float2(0, 0);
		break;
	case 1:
		vs_out.tc = float2(srcWidth * 2 * vs_out.textureSize_invTextureSize.z, 0);
		break;
	case 2:
		vs_out.tc = float2(0, srcHeight * 2 * vs_out.textureSize_invTextureSize.w);
		break;
	}
}
