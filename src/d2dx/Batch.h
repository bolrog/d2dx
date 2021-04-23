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

#include "Types.h"
#include "GameHelper.h"

namespace d2dx
{
	class Batch final
	{
	public:
		Batch() noexcept :
			_textureStartAddress(0),
			_startVertexHigh_textureIndex(0),
			_textureHash(0),
			_textureHeight_textureWidth_alphaBlend(0),
			_vertexCount(0),
			_isChromaKeyEnabled_gameAddress_paletteIndex(0),
			_textureCategory_primitiveType_combiners(0),
			_startVertexLow(0),
			_textureAtlas(0)
		{
		}

		inline GameAddress GetGameAddress() const noexcept
		{
			return (GameAddress)((_isChromaKeyEnabled_gameAddress_paletteIndex & 0x70) >> 4);
		}

		inline void SetGameAddress(GameAddress gameAddress) noexcept
		{
			assert((int32_t)gameAddress < 8);
			_isChromaKeyEnabled_gameAddress_paletteIndex &= ~0x70;
			_isChromaKeyEnabled_gameAddress_paletteIndex |= (uint8_t)((uint32_t)gameAddress << 4) & 0x70;
		}

		inline int32_t GetPaletteIndex() const noexcept
		{
			return _isChromaKeyEnabled_gameAddress_paletteIndex & 0xF;
		}

		inline void SetPaletteIndex(int32_t paletteIndex) noexcept
		{
			assert(paletteIndex >= 0 && paletteIndex < 16);
			_isChromaKeyEnabled_gameAddress_paletteIndex &= 0xF0;
			_isChromaKeyEnabled_gameAddress_paletteIndex |= paletteIndex;
		}

		inline bool IsChromaKeyEnabled() const noexcept
		{
			return (_isChromaKeyEnabled_gameAddress_paletteIndex & 0x80) != 0;
		}

		inline void SetIsChromaKeyEnabled(bool enable) noexcept
		{
			_isChromaKeyEnabled_gameAddress_paletteIndex &= 0x7F;
			_isChromaKeyEnabled_gameAddress_paletteIndex |= enable ? 0x80 : 0;
		}

		inline RgbCombine GetRgbCombine() const noexcept
		{
			return (RgbCombine)(_textureCategory_primitiveType_combiners & 0x01);
		}

		inline void SetRgbCombine(RgbCombine rgbCombine) noexcept
		{
			assert((int32_t)rgbCombine >= 0 && (int32_t)rgbCombine < 2);
			_textureCategory_primitiveType_combiners &= ~0x01;
			_textureCategory_primitiveType_combiners |= (uint8_t)rgbCombine & 0x01;
		}

		inline AlphaCombine GetAlphaCombine() const noexcept
		{
			return (AlphaCombine)((_textureCategory_primitiveType_combiners >> 1) & 0x01);
		}

		inline void SetAlphaCombine(AlphaCombine alphaCombine) noexcept
		{
			assert((int32_t)alphaCombine >= 0 && (int32_t)alphaCombine < 2);
			_textureCategory_primitiveType_combiners &= ~0x02;
			_textureCategory_primitiveType_combiners |= ((uint8_t)alphaCombine << 1) & 0x02;
		}

		inline int32_t GetTextureWidth() const noexcept
		{
			return 1 << (((_textureHeight_textureWidth_alphaBlend >> 2) & 7) + 1);
		}

		inline int32_t GetTextureHeight() const noexcept
		{
			return 1 << (((_textureHeight_textureWidth_alphaBlend >> 5) & 7) + 1);
		}

		inline void SetTextureSize(int32_t width, int32_t height) noexcept
		{
			DWORD w, h;
			BitScanReverse(&w, (uint32_t)width);
			BitScanReverse(&h, (uint32_t)height);
			assert(w >= 3 && w <= 8);
			assert(h >= 3 && h <= 8);
			_textureHeight_textureWidth_alphaBlend &= ~0xFC;
			_textureHeight_textureWidth_alphaBlend |= (h - 1) << 5;
			_textureHeight_textureWidth_alphaBlend |= (w - 1) << 2;
		}

		inline AlphaBlend GetAlphaBlend() const noexcept
		{
			return (AlphaBlend)(_textureHeight_textureWidth_alphaBlend & 3);
		}

		inline void SetAlphaBlend(AlphaBlend alphaBlend) noexcept
		{
			assert((uint32_t)alphaBlend < 4);
			_textureHeight_textureWidth_alphaBlend &= ~0x03;
			_textureHeight_textureWidth_alphaBlend |= (uint32_t)alphaBlend & 3;
		}

		inline int32_t GetStartVertex() const noexcept
		{
			return _startVertexLow | ((_startVertexHigh_textureIndex & 0xF000) << 4);
		}

		inline void SetStartVertex(int32_t startVertex) noexcept
		{
			assert(startVertex <= 0xFFFFF);
			_startVertexLow = startVertex & 0xFFFF;
			_startVertexHigh_textureIndex &= ~0xF000;
			_startVertexHigh_textureIndex |= (startVertex >> 4) & 0xF000;
		}

		inline uint32_t GetVertexCount() const noexcept
		{
			return _vertexCount;
		}

		inline void SetVertexCount(uint32_t vertexCount) noexcept
		{
			assert(vertexCount >= 0 && vertexCount <= 0xFFFF);
			_vertexCount = vertexCount;
		}

		inline uint32_t SelectColorAndAlpha(uint32_t iteratedColor, uint32_t constantColor) const noexcept
		{
			const auto rgbCombine = GetRgbCombine();
			uint32_t result = (rgbCombine == RgbCombine::ConstantColor ? constantColor : iteratedColor) & 0x00FFFFFF;
			result |= constantColor & 0xFF000000;

			if (GetAlphaBlend() != AlphaBlend::SrcAlphaInvSrcAlpha)
			{
				result |= 0xFF000000;
			}

			return result;
		}

		inline uint32_t GetHash() const noexcept
		{
			return _textureHash;
		}

		void SetTextureHash(uint32_t textureHash) noexcept
		{
			_textureHash = textureHash;
		}

		inline uint32_t GetTextureAtlas() const noexcept
		{
			return (uint32_t)(_textureAtlas & 7);
		}

		inline void SetTextureAtlas(uint32_t textureAtlas) noexcept
		{
			assert(textureAtlas < 8);
			_textureAtlas = textureAtlas & 7;
		}

		inline uint32_t GetTextureIndex() const noexcept
		{
			return (uint32_t)(_startVertexHigh_textureIndex & 0x0FFF);
		}

		inline void SetTextureIndex(uint32_t textureIndex) noexcept
		{
			assert(textureIndex < 4096);
			_startVertexHigh_textureIndex &= ~0x0FFF;
			_startVertexHigh_textureIndex = (uint16_t)(textureIndex & 0x0FFF);
		}

		inline TextureCategory GetTextureCategory() const noexcept
		{
			return (TextureCategory)(_textureCategory_primitiveType_combiners >> 5U);
		}

		inline void SetTextureCategory(TextureCategory category) noexcept
		{
			assert((uint32_t)category < 8);
			_textureCategory_primitiveType_combiners &= ~0xE0;
			_textureCategory_primitiveType_combiners |= ((uint32_t)category << 5U) & 0xE0;
		}

		inline int32_t GetTextureStartAddress() const noexcept
		{
			const uint32_t startAddress = _textureStartAddress;
			return (startAddress - 1) << 8;
		}

		inline void SetTextureStartAddress(int32_t startAddress) noexcept
		{
			assert(!(startAddress & (D2DX_TMU_ADDRESS_ALIGNMENT-1)));
			assert(startAddress >= 0 && startAddress <= (D2DX_TMU_MEMORY_SIZE - D2DX_TMU_ADDRESS_ALIGNMENT));

			startAddress >>= 8;
			++startAddress;

			assert((startAddress & 0xFFFF) != 0);

			_textureStartAddress = startAddress & 0xFFFF;
		}

		inline bool IsValid() const noexcept
		{
			return _textureStartAddress != 0;
		}

	private:
		uint32_t _textureHash;
		uint16_t _startVertexLow;
		uint16_t _vertexCount;
		uint16_t _textureStartAddress;							// byte address / D2DX_TMU_ADDRESS_ALIGNMENT
		uint16_t _startVertexHigh_textureIndex;					// VVVVAAAA AAAAAAAA
		uint8_t _textureHeight_textureWidth_alphaBlend;			// HHHWWWBB
		uint8_t _isChromaKeyEnabled_gameAddress_paletteIndex;	// CGGGPPPP
		uint8_t _textureCategory_primitiveType_combiners;		// TTT.PPCC
		uint8_t _textureAtlas;									// .....AAA
	};

	static_assert(sizeof(Batch) == 16, "sizeof(Batch)");
}
