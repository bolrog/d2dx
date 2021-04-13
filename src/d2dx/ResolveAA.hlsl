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

//#define SHOW_BATCH_IDS
//#define SHOW_MASK

Texture2D sceneTexture : register(t0);
Texture2D<float> idTexture : register(t1);

#define FXAA_PC 1
#define FXAA_HLSL_4 1
#define FXAA_QUALITY__PRESET 23
#include "FXAA.hlsli" 

half4 main(
	in noperspective half2 tc : TEXCOORD0) : SV_TARGET
{
	float2 invTextureSize;
	sceneTexture.GetDimensions(invTextureSize.x, invTextureSize.y);
	invTextureSize = 1.0 / invTextureSize;

	half4 c = sceneTexture.Load(int3(tc, 0));

#ifdef SHOW_BATCH_IDS
	uint id = (uint)idTexture.Load(int3(tc, 0));
	c.r = (id & 31) / 31.0;
	c.g = ((id >> 5) & 31) / 31.0;
	c.b = ((id >> 10) & 15) / 31.0;
	return half4(c.r, c.g, c.b, 1);
#endif

	bool isEdge = false;

	float id = idTexture.Load(int3(tc, 0), int2(-1, -1));
	float2 tcf = tc * invTextureSize;


	if (
		idTexture.Sample(BilinearSampler, tcf + float2(0.5, -1) * invTextureSize) != id ||
		idTexture.Sample(BilinearSampler, tcf + float2(-1, 0.5) * invTextureSize) != id ||
		idTexture.Sample(BilinearSampler, tcf + float2(0.5, 0.5) * invTextureSize) != id)
	{
		isEdge = true;
	}

	if (isEdge)
	{
#ifdef SHOW_MASK
		return half4(1, 0, 0, 1);
#else

		FxaaTex ftx;
		ftx.smpl = BilinearSampler;
		ftx.tex = sceneTexture;
		c = FxaaPixelShader(c, tc * invTextureSize, ftx, invTextureSize, 0.25, 0.166, 0.0625);
#endif
	}
#ifdef SHOW_MASK
	else
	{
		return half4(0, 1, 0, 1);
	}
#endif

	return c;
}
