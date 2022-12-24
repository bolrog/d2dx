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

struct GameVSInput
{
	float2 pos : POSITION;
	int2 texCoord : TEXCOORD0;
	float4 color : COLOR0;
	uint2 misc : TEXCOORD1;
};

struct GameVSOutput
{
	noperspective float4 pos : SV_POSITION;
	noperspective float2 tc : TEXCOORD0;
	noperspective float4 color : COLOR0;
	nointerpolation uint4 atlasIndex_paletteIndex_surfaceId_flags : TEXCOORD1;
};

typedef GameVSOutput GamePSInput;

struct GamePSOutput
{
	float4 color : SV_TARGET0;
	float surfaceId : SV_TARGET1;
};
