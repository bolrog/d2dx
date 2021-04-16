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

Texture2D sceneTexture : register(t0);

/* Anti-aliased nearest-neighbor sampling, taken from https://www.shadertoy.com/view/WtjyWy by user Amarcoli. */
float2 nearestSampleUV_AA(float2 uv, float sharpness, float2 invTextureSize) {
	float2 tileUv = uv / invTextureSize;
	//float2 dx = float2(ddx(tileUv.x), ddy(tileUv.x));
	//float2 dy = float2(ddx(tileUv.y), ddy(tileUv.y));

	//float2 dxdy = float2(max(abs(dx.x), abs(dx.y)), max(abs(dy.x), abs(dy.y)));
	float2 dxdy = float2(ddx(tileUv.x), ddy(tileUv.y));

	float2 texelDelta = frac(tileUv) - 0.5;
	float2 distFromEdge = 0.5 - abs(texelDelta);
	float2 aa_factor = saturate(distFromEdge * sharpness / dxdy);

	return uv - texelDelta * aa_factor * invTextureSize;
}

half4 main(
	in noperspective float2 tc : TEXCOORD0,
	in nointerpolation float2 invTextureSize : TEXCOORD1) : SV_TARGET
{
	return sceneTexture.Sample(BilinearSampler, nearestSampleUV_AA(tc, 2.0, invTextureSize));
}
