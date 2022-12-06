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
	// static_assert(((D2DX_TMU_MEMORY_SIZE - 1) >> 8) == 0xFFFF, "TMU memory start addresses aren't 16 bit.");

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

	template<class T>
	struct OffsetT final {
		T x;
		T y;
		
		OffsetT() = default;
		OffsetT(OffsetT const&) = default;

		OffsetT(T x_, T y_) noexcept :
			x(x_),
			y(y_)
		{
		}

		template<typename U>
		OffsetT(OffsetT<U> const &other) noexcept:
			x(static_cast<T>(other.x)),
			y(static_cast<T>(other.y))
		{}

		OffsetT& operator+=(const OffsetT& rhs) noexcept
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}

		OffsetT& operator-=(const OffsetT& rhs) noexcept
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}

		OffsetT& operator*=(const OffsetT& rhs) noexcept
		{
			x *= rhs.x;
			y *= rhs.y;
			return *this;
		}

		OffsetT& operator+=(T const &rhs) noexcept
		{
			x += rhs;
			y += rhs;
			return *this;
		}

		OffsetT& operator-=(T const &rhs) noexcept
		{
			x -= rhs;
			y -= rhs;
			return *this;
		}

		OffsetT& operator*=(T const &rhs) noexcept
		{
			x *= rhs;
			y *= rhs;
			return *this;
		}

		OffsetT& operator/=(T const& rhs) noexcept
		{
			x /= rhs;
			y /= rhs;
			return *this;
		}

		OffsetT operator-() const noexcept
		{
			return { -x, -y };
		}

		OffsetT operator+(const OffsetT& rhs) const noexcept
		{
			return { x + rhs.x, y + rhs.y };
		}

		OffsetT operator-(const OffsetT& rhs) const noexcept
		{
			return { x - rhs.x, y - rhs.y };
		}

		OffsetT operator*(const OffsetT& rhs) const noexcept
		{
			return { x * rhs.x, y * rhs.y };
		}

		OffsetT operator+(T const &rhs) const noexcept
		{
			return { x + rhs, y + rhs };
		}

		OffsetT operator-(T const &rhs) const noexcept
		{
			return { x - rhs, y - rhs };
		}

		OffsetT operator*(T const &rhs) const noexcept
		{
			return { x * rhs, y * rhs };
		}

		OffsetT operator/(T const& rhs) const noexcept
		{
			return { x / rhs, y / rhs };
		}

		bool operator==(const OffsetT& rhs) const = default;
		bool operator!=(const OffsetT& rhs) const = default;

		T RealLength() const noexcept
		{
			return std::hypot(x, y);
		}

		T Length() const noexcept
		{
			T lensqr = x * x + y * y;
			return lensqr > T{ 0.01 } ? std::sqrt(lensqr) : T{1};
		}

		OffsetT<T> Round() const noexcept
		{
			return { std::round(x), std::round(y) };
		}

		void Normalize() noexcept
		{
			const T invlen = T{ 1 } / Length();
			x *= invlen;
			y *= invlen;
		}
	};

	using Offset = OffsetT<int32_t>;
	using OffsetF = OffsetT<float>;

	struct Size final
	{
		int32_t width = 0;
		int32_t height = 0;

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

		bool operator==(const Rect& rhs) const = default;
	};
}
