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

#define D2DX_MAX_GAME_PALETTES 14
#define D2DX_WHITE_PALETTE_INDEX 14
#define D2DX_LOGO_PALETTE_INDEX 15
#define D2DX_MAX_PALETTES 16

#define D2DX_SURFACE_ID_USER_INTERFACE 16383

namespace d2dx
{
	static_assert(((D2DX_TMU_MEMORY_SIZE - 1) >> 8) == 0xFFFF, "TMU memory start addresses aren't 16 bit.");

	enum class ScreenMode
	{
		Windowed = 0,
		FullscreenDefault = 1,
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
		Player = 2,
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
		Lod110f = 2,
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
		DrawLine = 7,
		Count = 8,
	};

	struct OffsetF final
	{
		float x = 0;
		float y = 0;

		OffsetF(float x_, float y_) noexcept :
			x{ x_ },
			y{ y_ }
		{
		}

		OffsetF& operator+=(const OffsetF& rhs) noexcept
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}

		OffsetF& operator-=(const OffsetF& rhs) noexcept
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}

		OffsetF& operator*=(const OffsetF& rhs) noexcept
		{
			x *= rhs.x;
			y *= rhs.y;
			return *this;
		}

		OffsetF& operator+=(float rhs) noexcept
		{
			x += rhs;
			y += rhs;
			return *this;
		}

		OffsetF& operator-=(float rhs) noexcept
		{
			x -= rhs;
			y -= rhs;
			return *this;
		}

		OffsetF& operator*=(float rhs) noexcept
		{
			x *= rhs;
			y *= rhs;
			return *this;
		}

		OffsetF operator+(const OffsetF& rhs) const noexcept
		{
			return { x + rhs.x, y + rhs.y };
		}

		OffsetF operator-(const OffsetF& rhs) const noexcept
		{
			return { x - rhs.x, y - rhs.y };
		}

		OffsetF operator*(const OffsetF& rhs) const noexcept
		{
			return { x * rhs.x, y * rhs.y };
		}

		OffsetF operator+(float rhs) const noexcept
		{
			return { x + rhs, y + rhs };
		}

		OffsetF operator-(float rhs) const noexcept
		{
			return { x - rhs, y - rhs };
		}

		OffsetF operator*(float rhs) const noexcept
		{
			return { x * rhs, y * rhs };
		}

		bool operator==(const OffsetF& rhs) const noexcept
		{
			return x == rhs.x && y == rhs.y;
		}

		float Length() const noexcept
		{
			const float lensqr = x * x + y * y;
			return lensqr > 0.01f ? sqrtf(lensqr) : 1.0f;
		}

		void Normalize() noexcept
		{
			const float lensqr = x * x + y * y;
			const float len = lensqr > 0.01f ? sqrtf(lensqr) : 1.0f;
			const float invlen = 1.0f / len;
			x *= invlen;
			y *= invlen;
		}
	};

	struct Offset final
	{
		int32_t x = 0;
		int32_t y = 0;

		Offset(int32_t x_, int32_t y_) noexcept :
			x{ x_ },
			y{ y_ }
		{
		}

		Offset(const OffsetF& rhs) noexcept :
			x{ (int32_t)rhs.x },
			y{ (int32_t)rhs.y }
		{
		}

		Offset& operator+=(const Offset& rhs) noexcept
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}

		Offset& operator-=(const Offset& rhs) noexcept
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}

		Offset& operator*=(const Offset& rhs) noexcept
		{
			x *= rhs.x;
			y *= rhs.y;
			return *this;
		}

		Offset& operator+=(int32_t rhs) noexcept
		{
			x += rhs;
			y += rhs;
			return *this;
		}

		Offset& operator-=(int32_t rhs) noexcept
		{
			x -= rhs;
			y -= rhs;
			return *this;
		}

		Offset& operator*=(int32_t rhs) noexcept
		{
			x *= rhs;
			y *= rhs;
			return *this;
		}

		Offset operator+(const Offset& rhs) const noexcept
		{
			return { x + rhs.x, y + rhs.y };
		}

		Offset operator-(const Offset& rhs) const noexcept
		{
			return { x - rhs.x, y - rhs.y };
		}

		Offset operator*(const Offset& rhs) const noexcept
		{
			return { x * rhs.x, y * rhs.y };
		}

		Offset operator+(int32_t rhs) const noexcept
		{
			return { x + rhs, y + rhs };
		}

		Offset operator-(int32_t rhs) const noexcept
		{
			return { x - rhs, y - rhs };
		}

		Offset operator*(int32_t rhs) const noexcept
		{
			return { x * rhs, y * rhs };
		}

		bool operator==(const Offset& rhs) const noexcept
		{
			return x == rhs.x && y == rhs.y;
		}
	};

	struct Size final
	{
		int32_t width = 0;
		int32_t height = 0;

		Size operator*(double value) noexcept
		{
			return { (int32_t)(width * value), (int32_t)(height * value) };
		}

		Size operator*(int32_t value) noexcept
		{
			return { width * value, height * value };
		}

		Size operator*(uint32_t value) noexcept
		{
			return { width * (int32_t)value, height * (int32_t)value };
		}

		bool operator==(const Size& rhs) const noexcept
		{
			return width == rhs.width && height == rhs.height;
		}
	};

	struct Rect final
	{
		Offset offset;
		Size size;

		Rect() noexcept :
			offset{ 0,0 },
			size{ 0,0 }
		{
		}

		Rect(int32_t x, int32_t y, int32_t w, int32_t h) noexcept :
			offset{ x,y },
			size{ w,h }
		{
		}

		bool IsValid() const noexcept
		{
			return size.width > 0 && size.height > 0;
		}

		bool operator==(const Rect& rhs) const noexcept
		{
			return offset == rhs.offset && size == rhs.size;
		}
	};
}
