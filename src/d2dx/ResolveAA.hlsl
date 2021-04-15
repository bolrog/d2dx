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
//#define SHOW_AMPLIFIED_DIFFERENCE

Texture2D sceneTexture : register(t0);
Texture2D<float2> idTexture : register(t1);

#define FXAA_PC 1
#define FXAA_HLSL_4 1
#define FXAA_QUALITY__PRESET 25
#include "FXAA.hlsli" 

half4 main(
	in noperspective float2 tc : TEXCOORD0) : SV_TARGET
{
	int3 tci = int3(tc, 0);
	
	float2 invTextureSize;
	sceneTexture.GetDimensions(invTextureSize.x, invTextureSize.y);
	invTextureSize = 1.0 / invTextureSize;
	tc *= invTextureSize;

	half4 c = sceneTexture.Load(tci);

	float id = idTexture.Load(tci, int2(-1,-1)).x;

	bool isEdge =
		idTexture.Sample(BilinearSampler, tc + float2(0.5, -1) * invTextureSize).x != id ||
		idTexture.Sample(BilinearSampler, tc + float2(-1, 0.5) * invTextureSize).x != id ||
		idTexture.Sample(BilinearSampler, tc + float2(0.5, 0.5) * invTextureSize).x != id;

#ifdef SHOW_BATCH_IDS
	uint iid = (uint)(idTexture.Load(tci).x * 16383.0);
	c.r = (iid & 31) / 31.0;
	c.g = ((iid >> 5) & 31) / 31.0;
	c.b = ((iid >> 10) & 15) / 31.0;
	return isEdge ? half4(1-c.r, 1-c.g, 1-c.b, 1) : half4(c.r, c.g, c.b, 1);
#else

	if (isEdge)
	{
#ifdef SHOW_MASK
		return half4(1, 0, 0, 1);
#else
		FxaaTex ftx;
		ftx.smpl = BilinearSampler;
		ftx.tex = sceneTexture;

#ifdef SHOW_AMPLIFIED_DIFFERENCE
		half4 oldc = c;
#endif
		c = FxaaPixelShader(c, tc, ftx, invTextureSize, 0.25, 0.166, 0.1);
#ifdef SHOW_AMPLIFIED_DIFFERENCE
		c.rgb = 0.5 + c.rgb - oldc.rgb;
#endif
#endif
	}
#if defined(SHOW_MASK) || defined(SHOW_AMPLIFIED_DIFFERENCE)
	else
	{
		return half4(0.5, 0.5, 0.5, 1);
	}
#endif

	return c;
#endif
}
