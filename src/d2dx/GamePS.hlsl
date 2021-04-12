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

half4 main(PixelShaderInput psInput) : SV_TARGET
{
	const uint atlasIndex = psInput.misc.x;
	const bool chromaKeyEnabled = (psInput.misc.y & MISC_CHROMAKEY_ENABLED_MASK) != 0;

	uint indexedColor = tex.Load(int4(psInput.tc, atlasIndex, 0));
	
	if (chromaKeyEnabled && indexedColor == 0)
		discard;

	const uint paletteIndex = psInput.misc.y >> 8;	
	const half4 textureColor = palette.Load(int3(indexedColor, paletteIndex, 0));

	const uint colorCombine = psInput.misc.y & MISC_RGB_MASK;
	const uint alphaCombine = psInput.misc.y & MISC_ALPHA_MASK;

	half3 c =
		(colorCombine == MISC_RGB_ITERATED_COLOR_MULTIPLIED_BY_TEXTURE) ? textureColor.rgb * psInput.color.rgb :
		((colorCombine == MISC_RGB_CONSTANT_COLOR) ? psInput.color.rgb :
		half3(1, 0, 1));
	
	half a =
		(alphaCombine == MISC_ALPHA_ONE) ? 1 :
		(alphaCombine == MISC_ALPHA_TEXTURE) ? psInput.color.a : 1;

	return half4(c, a);
}
