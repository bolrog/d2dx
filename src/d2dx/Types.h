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
#pragma once

#define D2DX_TMU_ADDRESS_ALIGNMENT 256
#define D2DX_TMU_MEMORY_SIZE (16 * 1024 * 1024)
#define D2DX_SIDE_TMU_MEMORY_SIZE (1 * 1024 * 1024)
#define D2DX_MAX_BATCHES_PER_FRAME 16384
#define D2DX_MAX_VERTICES_PER_FRAME (1024 * 1024)

#define D2DX_WHITE_PALETTE_INDEX 30
#define D2DX_LOGO_PALETTE_INDEX 31
#define D2DX_MAX_PALETTES 32

namespace d2dx
{
	static_assert(((D2DX_TMU_MEMORY_SIZE - 1) >> 8) == 0xFFFF, "TMU memory start addresses aren't 16 bit.");

	enum class ScreenMode
	{
		Windowed = 0,
		FullscreenDefault = 1,
	};

	struct Options
	{
		ScreenMode screenMode = ScreenMode::Windowed;
		uint32_t defaultZoomLevel = 0;
		bool noClipCursor = false;
		bool noFpsFix = false;
		bool noWide = false;
		bool noLogo = false;
		bool noVSync = false;
		bool noResMod = false;
		bool noAA = false;
	};

	enum class MajorGameState
	{
		Unknown = 0,
		FmvIntro = 1,
		Menus = 2,
		InGame = 3,
		TitleScreen = 4,
	};

	enum class PrimitiveType
	{
		Points = 0,
		Lines = 1,
		Triangles = 2,
		Count = 3
	};

	enum class AlphaBlend
	{
		Opaque = 0,
		SrcAlphaInvSrcAlpha = 1,
		Additive = 2,
		Multiplicative = 3,
		Count = 4
	};

	enum class RgbCombine
	{
		ColorMultipliedByTexture = 0,
		ConstantColor = 1,
		Count = 2
	};

	enum class AlphaCombine
	{
		One = 0,
		FromColor = 1,
		Count = 2
	};

	enum class TextureCategory
	{
		Unknown = 0,
		MousePointer = 1,
		Font = 2,
		LoadingScreen = 3,
		Floor = 4,
		TitleScreen = 5,
		Wall = 6,
		UserInterface = 7,
		Count = 8
	};

	enum class GameVersion
	{
		Unsupported = 0,
		Lod109d = 1,
		Lod110 = 2,
		Lod112 = 3,
		Lod113c = 4,
		Lod113d = 5,
		Lod114d = 6,
	};

	enum class GameAddress
	{
		Unknown = 0,
		DrawWall1 = 1,
		DrawWall2 = 2,
		DrawFloor = 3,
		DrawShadow = 4,
		DrawDynamic = 5,
		DrawSomething1 = 6,
		DrawSomething2 = 7,
		Count = 8,
	};

	struct Offset final
	{
		int32_t x = 0;
		int32_t y = 0;

		bool operator==(const Offset& rhs) const
		{
			return x == rhs.x && y == rhs.y;
		}
	};

	struct Size final
	{
		int32_t width = 0;
		int32_t height = 0;

		Size operator*(int32_t value)
		{
			return { width * value, height * value };
		}

		Size operator*(uint32_t value)
		{
			return { width * (int32_t)value, height * (int32_t)value };
		}

		bool operator==(const Size& rhs) const
		{
			return width == rhs.width && height == rhs.height;
		}
	};

	struct Rect final
	{
		Offset offset;
		Size size;

		bool IsValid() const
		{
			return size.width > 0 && size.height > 0;
		}

		bool operator==(const Rect& rhs) const
		{
			return offset == rhs.offset && size == rhs.size;
		}
	};
}
