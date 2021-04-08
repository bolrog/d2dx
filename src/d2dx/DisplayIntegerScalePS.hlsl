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
Texture1D gammaTexture : register(t1);

half4 main(in float2 tc : TEXCOORD0) : SV_TARGET
{
	half3 c = sceneTexture.Load(float3(tc, 0)).rgb;
	c.r = gammaTexture.Sample(BilinearSampler, c.r).r;
	c.g = gammaTexture.Sample(BilinearSampler, c.g).g;
	c.b = gammaTexture.Sample(BilinearSampler, c.b).b;
	return half4(c, 1);
}
