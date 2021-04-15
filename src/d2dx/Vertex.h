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
			int32_t paletteIndex,
			int32_t batchIndex) :
			_x(DirectX::PackedVector::XMConvertFloatToHalf(x)),
			_y(DirectX::PackedVector::XMConvertFloatToHalf(y)),
			_s_t_batchIndex((batchIndex << 18) | ((t & 511) << 9) | (s & 511)),
			_color(color),
			_paletteIndex_isChromaKeyEnabled_alphaCombine_rgbCombine((paletteIndex << 8) | (isChromaKeyEnabled ? (1U << 2U) : 0) | ((uint32_t)alphaCombine << 1U) | ((uint32_t)rgbCombine)),
			_atlasIndex(atlasIndex)
		{
			assert(s >= INT16_MIN && s <= INT16_MAX);
			assert(t >= INT16_MIN && t <= INT16_MAX);
			assert(atlasIndex >= 0 && atlasIndex <= 4095);
			assert(batchIndex >= 0 && batchIndex <= 16383);
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

		void SetBatchIndex(int32_t batchIndex)
		{
			assert(batchIndex >= 0 && batchIndex <= 16383);
			_s_t_batchIndex &= ~(16383 << 18);
			_s_t_batchIndex |= (batchIndex << 18);
		}

		inline int32_t GetS() const
		{
			return _s_t_batchIndex & 511;
		}

		inline void SetS(int32_t s)
		{
			assert(s >= 0 && s <= 511);
			_s_t_batchIndex &= ~511;
			_s_t_batchIndex |= s & 511;
		}

		inline int32_t GetT() const
		{
			return (_s_t_batchIndex >> 9) & 511;
		}

		inline void SetT(int32_t t)
		{
			assert(t >= 0 && t <= 511);
			_s_t_batchIndex &= ~(511 << 9);
			_s_t_batchIndex |= (t & 511) << 9;
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
		uint32_t _s_t_batchIndex;
		uint32_t _color;
		uint16_t _atlasIndex;
		uint16_t _paletteIndex_isChromaKeyEnabled_alphaCombine_rgbCombine;
	};

	static_assert(sizeof(Vertex) == 16, "sizeof(Vertex)");
}
