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

Texture2DArray<uint> tex : register(t0);
Texture1DArray palette : register(t1);

void main(
	in GamePSInput ps_in,
	out GamePSOutput ps_out)
{
	const uint atlasIndex = ps_in.atlasIndex_paletteIndex_surfaceId_flags.x;
	const bool chromaKeyEnabled = ps_in.atlasIndex_paletteIndex_surfaceId_flags.w & 1;
	const uint surfaceId = ps_in.atlasIndex_paletteIndex_surfaceId_flags.z;
	const uint paletteIndex = ps_in.atlasIndex_paletteIndex_surfaceId_flags.y;

	const uint indexedColor = tex.Load(int4(ps_in.tc, atlasIndex, 0));

	if (chromaKeyEnabled && indexedColor == 0)
		discard;

	const float4 textureColor = palette.Load(int3(indexedColor, paletteIndex, 0));

	ps_out.color = ps_in.color * textureColor;
	ps_out.surfaceId = surfaceId * 1.0 / 16383.0;
}
