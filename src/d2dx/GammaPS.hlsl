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

float4 main(
	in noperspective float2 tc : TEXCOORD0) : SV_TARGET
{
	float4 c = sceneTexture.SampleLevel(PointSampler, tc, 0);
	c.r = gammaTexture.SampleLevel(BilinearSampler, c.r, 0).r;
	c.g = gammaTexture.SampleLevel(BilinearSampler, c.g, 0).g;
	c.b = gammaTexture.SampleLevel(BilinearSampler, c.b, 0).b;
	c.a = dot(c.rgb, float3(0.299, 0.587, 0.114));
	return c;
}
