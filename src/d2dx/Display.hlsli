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

struct DisplayVSInput
{
	float2 pos : POSITION;
	int2 st : TEXCOORD0;
	float4 color : COLOR;
	uint2 misc : TEXCOORD1;
};

struct DisplayVSOutput
{
	noperspective float2 tc : TEXCOORD0;
	nointerpolation float4 textureSize_invTextureSize : TEXCOORD1;
};

typedef DisplayVSOutput DisplayPSInput;
