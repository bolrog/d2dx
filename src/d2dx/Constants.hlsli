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

cbuffer Constants : register(b0)
{
	float2 screenSize;
	uint2 flags;
};

struct PixelShaderInput
{
	noperspective float4 pos : SV_POSITION;
	noperspective float2 tc : TEXCOORD0;
	noperspective half4 color : COLOR0;
	nointerpolation uint2 misc : TEXCOORD1;
};

SamplerState PointSampler : register(s0);
SamplerState BilinearSampler : register(s1);

#define MISC_RGB_ITERATED_COLOR_MULTIPLIED_BY_TEXTURE	(0 << 0)
#define MISC_RGB_CONSTANT_COLOR							(1 << 0)
#define MISC_RGB_MASK									(1 << 0)

#define MISC_ALPHA_ONE									(0 << 1)
#define MISC_ALPHA_TEXTURE								(1 << 1)
#define MISC_ALPHA_MASK									(1 << 1)

#define MISC_CHROMAKEY_ENABLED_MASK						(1 << 2)
#define MISC_CHROMAKEY_THRESHOLD_LOW_MASK				(1 << 3)
