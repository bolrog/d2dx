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

Texture2DArray<uint> tex : register(t0);
Texture1DArray palette : register(t1);

PixelShaderOutput main(PixelShaderInput psInput)
{
	const uint atlasIndex = psInput.misc.x;
	const bool chromaKeyEnabled = (psInput.misc.y & MISC_CHROMAKEY_ENABLED_MASK) != 0;

	uint indexedColor = tex.Load(int4(psInput.tc, atlasIndex, 0));

	if (chromaKeyEnabled && indexedColor == 0)
		discard;

	const uint paletteIndex = psInput.misc.y >> 8;
	const half3 textureColor = palette.Load(int3(indexedColor, paletteIndex, 0));

	half3 c = psInput.color.rgb * textureColor.rgb;
	half a = psInput.color.a;

	PixelShaderOutput psOutput;
	psOutput.output0 = half4(c, a);
	psOutput.output1 = float2(psInput.misc.z / 16383.0, 0);
	return psOutput;
}
