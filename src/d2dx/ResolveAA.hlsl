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
#include "Display.hlsli"

//#define SHOW_SURFACE_IDS
//#define SHOW_MASK
//#define SHOW_AMPLIFIED_DIFFERENCE

Texture2D sceneTexture : register(t0);
Texture2D<float> idTexture : register(t1);

#define FXAA_PC 1
#define FXAA_HLSL_4 1
#define FXAA_QUALITY__PRESET 23
#include "FXAA.hlsli" 

float4 main(
	in DisplayPSInput ps_in) : SV_TARGET
{
	float4 c = sceneTexture.SampleLevel(PointSampler, ps_in.tc, 0);

	/*
	 A B C
	 D E F
	 G H I	
	*/

	float2 tcShifted = ps_in.tc - 0.5 * ps_in.textureSize_invTextureSize.zw;

	float idC = idTexture.SampleLevel(PointSampler, ps_in.tc, 0, int2(1,-1));
	float4 idDEBA = idTexture.Gather(BilinearSampler, tcShifted);
	float4 idHIFE = idTexture.Gather(BilinearSampler, tcShifted, int2(1, 1));
	float idG = idTexture.SampleLevel(PointSampler, ps_in.tc, 0, int2(-1, 1));

	bool isEdge =
		idDEBA.y < (1.0-1.0/16383.0) && (idG != idC || any(idDEBA - idC) || any(idHIFE - idC));

	if (isEdge)
	{
#ifdef SHOW_MASK
		return float4(1, 0, 0, 1);
#else
		FxaaTex ftx;
		ftx.smpl = BilinearSampler;
		ftx.tex = sceneTexture;

#ifdef SHOW_AMPLIFIED_DIFFERENCE
		float4 oldc = c;
#endif
		c = FxaaPixelShader(c, ps_in.tc, ftx, ps_in.textureSize_invTextureSize.zw, 0.5, 0.166, 0.166 * 0.5);
#ifdef SHOW_AMPLIFIED_DIFFERENCE
		c.rgb = 0.5 + 4*(c.rgb - oldc.rgb);
#endif
#endif
	}
#if defined(SHOW_MASK) || defined(SHOW_AMPLIFIED_DIFFERENCE)
	else
	{
		return float4(0.5, 0.5, 0.5, 1);
	}
#endif

#ifdef SHOW_SURFACE_IDS
	uint iid = (uint)(idTexture.SampleLevel(PointSampler, ps_in.tc, 0).x * 16383.0);
	float3 idc;
	idc.r = (iid & 31) / 31.0;
	idc.g = ((iid >> 5) & 31) / 31.0;
	idc.b = ((iid >> 10) & 15) / 15.0;
	c.rgb = lerp(c.rgb, isEdge ? 1 - idc : idc, 0.5);
#endif

	return c;
}
