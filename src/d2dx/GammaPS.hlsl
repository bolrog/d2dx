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

half4 main(
	in noperspective float2 tc : TEXCOORD0) : SV_TARGET
{
	half4 c = sceneTexture.Load(int3(tc, 0));
	c.r = gammaTexture.Load(int2(c.r * 255.0, 0)).r;
	c.g = gammaTexture.Load(int2(c.g * 255.0, 0)).g;
	c.b = gammaTexture.Load(int2(c.b * 255.0, 0)).b;
	c.a = dot(c.rgb, float3(0.299, 0.587, 0.114));
	return c;
}
