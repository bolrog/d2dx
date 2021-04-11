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

// #define SHOW_EDGES

Texture2D sceneTexture : register(t0);
Texture1D gammaTexture : register(t1);

#define FXAA_PC 1
#define FXAA_HLSL_4 1
#define FXAA_QUALITY__PRESET 12

half4 gammaCorrect(half4 c)
{
	return c;
}

#include "FXAA.hlsli"

half4 main(in float2 tc : TEXCOORD0) : SV_TARGET
{
#ifdef SHOW_EDGES
	half4 color = sceneTexture.Load(float3(tc, 0));
	return half4(color.a, color.a, color.a, 1);
#endif

	half4 c;

	if ((flags.x & 1) != 0)
	{
		float2 invTextureSize;
		sceneTexture.GetDimensions(invTextureSize.x, invTextureSize.y);
		invTextureSize = 1.0 / invTextureSize;

		FxaaTex ftx;
		ftx.smpl = BilinearSampler;
		ftx.tex = sceneTexture;
		c = FxaaPixelShader(tc * invTextureSize, 0, ftx, ftx, ftx, invTextureSize, 0, 0, 0, 0.0, 0.05, 0/*.0833*/, 0, 0, 0, 0);
	}
	else
	{
		c = sceneTexture.Load(float3(tc, 0));
	}

	c.r = gammaTexture.Sample(BilinearSampler, c.r).r;
	c.g = gammaTexture.Sample(BilinearSampler, c.g).g;
	c.b = gammaTexture.Sample(BilinearSampler, c.b).b;
	return c;

}
