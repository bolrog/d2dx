/*
	This file is part of D2DX.

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
		Vertex(
			float x,
			float y,
			int32_t s,
			int32_t t,
			uint32_t color,
			RgbCombine rgbCombine,
			AlphaCombine alphaCombine,
			bool isChromaKeyEnabled,
			int32_t atlasIndex,
			int32_t paletteIndex) :
			_x(DirectX::PackedVector::XMConvertFloatToHalf(x)),
			_y(DirectX::PackedVector::XMConvertFloatToHalf(y)),
			_s(s),
			_t(t),
			_color(color),
			_paletteIndex_isChromaKeyEnabled_alphaCombine_rgbCombine((paletteIndex << 8) | (isChromaKeyEnabled ? (1U << 2U) : 0) | ((uint32_t)alphaCombine << 1U) | ((uint32_t)rgbCombine)),
			_atlasIndex(atlasIndex)
		{
			assert(s >= INT16_MIN && s <= INT16_MAX);
			assert(t >= INT16_MIN && t <= INT16_MAX);
			assert(atlasIndex >= 0 && atlasIndex <= 4095);
		}

		inline float GetX() const
		{
			return DirectX::PackedVector::XMConvertHalfToFloat(_x);
		}

		inline float GetY() const
		{
			return DirectX::PackedVector::XMConvertHalfToFloat(_y);
		}

		void SetX(float x)
		{
			_x = DirectX::PackedVector::XMConvertFloatToHalf(x);
		}

		void SetY(float y)
		{
			_y = DirectX::PackedVector::XMConvertFloatToHalf(y);
		}

		inline int32_t GetS() const
		{
			return _s;
		}

		inline void SetS(int32_t s)
		{
			assert(s >= INT16_MIN && s <= INT16_MAX);
			_s = s;
		}

		inline int32_t GetT() const
		{
			return _t;
		}

		inline void SetT(int32_t t)
		{
			assert(t >= INT16_MIN && t <= INT16_MAX);
			_t = t;
		}

		inline uint32_t GetColor() const
		{
			return _color;
		}

		inline void SetColor(uint32_t color)
		{
			_color = color;
		}

		inline RgbCombine GetRgbCombine() const
		{
			return (RgbCombine)(_paletteIndex_isChromaKeyEnabled_alphaCombine_rgbCombine & 0x01U);
		}

		inline AlphaCombine GetAlphaCombine() const
		{
			return (AlphaCombine)((_paletteIndex_isChromaKeyEnabled_alphaCombine_rgbCombine >> 1U) & 0x01U);
		}

		inline bool IsChromaKeyEnabled() const
		{
			return (_paletteIndex_isChromaKeyEnabled_alphaCombine_rgbCombine & (1U << 2U)) != 0;
		}

	private:
		DirectX::PackedVector::HALF _x;
		DirectX::PackedVector::HALF _y;
		int16_t _s;
		int16_t _t;
		uint32_t _color;
		uint16_t _atlasIndex;
		uint16_t _paletteIndex_isChromaKeyEnabled_alphaCombine_rgbCombine;
	};

	static_assert(sizeof(Vertex) == 16, "sizeof(Vertex)");
}
