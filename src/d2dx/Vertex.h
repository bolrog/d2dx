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

namespace d2dx
{
	class Vertex final
	{
	public:
		Vertex() noexcept :
			_x{ 0 },
			_y{ 0 },
			_s{ 0 },
			_t{ 0 },
			_color{ 0 },
			_isChromaKeyEnabled_surfaceId{ 0 },
			_paletteIndex_atlasIndex{ 0 }
		{
		}

		Vertex(
			float x,
			float y,
			int32_t s,
			int32_t t,
			uint32_t color,
			bool isChromaKeyEnabled,
			int32_t atlasIndex,
			int32_t paletteIndex,
			int32_t surfaceId) noexcept :
			_x(x),
			_y(y),
			_s(s),
			_t(t),
			_color(color),
			_isChromaKeyEnabled_surfaceId((isChromaKeyEnabled ? 0x4000 : 0) | (surfaceId & 16383)),
			_paletteIndex_atlasIndex((paletteIndex << 12) | (atlasIndex & 4095))
		{
			assert(s >= INT16_MIN && s <= INT16_MAX);
			assert(t >= INT16_MIN && t <= INT16_MAX);
			assert(paletteIndex >= 0 && paletteIndex < D2DX_MAX_PALETTES);
			assert(atlasIndex >= 0 && atlasIndex <= 4095);
			assert(surfaceId >= 0 && surfaceId <= 16383);
		}

		inline void AddOffset(
			_In_ float x,
			_In_ float y) noexcept
		{
			_x += x;
			_y += y;
		}

		inline float GetX() const noexcept
		{
			return _x;
		}

		inline float GetY() const noexcept
		{
			return _y;
		}

		inline void SetPosition(float x, float y) noexcept
		{
			_x = x;
			_y = y;
		}

		inline void SetSurfaceId(int32_t surfaceId) noexcept
		{
			assert(surfaceId >= 0 && surfaceId <= 16383);
			_isChromaKeyEnabled_surfaceId &= ~16383;
			_isChromaKeyEnabled_surfaceId |= surfaceId & 16383;
		}

		inline int32_t GetSurfaceId() const noexcept
		{
			return _isChromaKeyEnabled_surfaceId & 16383;
		}

		inline int32_t GetS() const noexcept
		{
			return _s;
		}

		inline int32_t GetT() const noexcept
		{
			return _t;
		}

		inline void SetTexcoord(int32_t s, int32_t t) noexcept
		{
			assert(s >= 0 && s <= 511);
			assert(t >= 0 && t <= 511);
			_s = s;
			_t = t;
		}

		inline uint32_t GetColor() const noexcept
		{
			return _color;
		}

		inline void SetColor(uint32_t color) noexcept
		{
			_color = color;
		}

		inline bool IsChromaKeyEnabled() const noexcept
		{
			return (_isChromaKeyEnabled_surfaceId & 0x4000) != 0;
		}

	private:
		float _x;
		float _y;
		int16_t _s;
		int16_t _t;
		uint32_t _color;
		uint16_t _paletteIndex_atlasIndex;
		uint16_t _isChromaKeyEnabled_surfaceId;
	};

	static_assert(sizeof(Vertex) == 20, "sizeof(Vertex)");
}
