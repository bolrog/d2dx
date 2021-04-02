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

#include "Types.h"
#include "GameHelper.h"

namespace d2dx
{
	class Batch final
	{
	public:
		Batch() :
			_textureStartAddress(0),
			_atlasIndexHigh(0),
			_textureHash(0),
			_textureCategory_atlasIndexLow(0),
			_textureHeight_textureWidth_alphaBlend(0),
			_vertexCount(0),
			_isChromaKeyEnabled_gameAddress_paletteIndex(0),
			_primitiveType_combiners(0),
			_startVertexLow(0),
			_isRgba_startVertexHigh(0)
		{
		}

		inline GameAddress GetGameAddress() const
		{
			return (GameAddress)((_isChromaKeyEnabled_gameAddress_paletteIndex & 0x70) >> 4);
		}

		inline void SetGameAddress(GameAddress gameAddress)
		{
			assert((int32_t)gameAddress >= 0 && (int32_t)gameAddress < (int32_t)GameAddress::Count);
			_isChromaKeyEnabled_gameAddress_paletteIndex &= 0x8F;
			_isChromaKeyEnabled_gameAddress_paletteIndex |= (uint8_t)gameAddress << 4;
		}

		inline int32_t GetPaletteIndex() const
		{
			return _isChromaKeyEnabled_gameAddress_paletteIndex & 0xF;
		}

		inline void SetPaletteIndex(int32_t paletteIndex)
		{
			assert(paletteIndex >= 0 && paletteIndex < 16);
			_isChromaKeyEnabled_gameAddress_paletteIndex &= 0xF0;
			_isChromaKeyEnabled_gameAddress_paletteIndex |= paletteIndex;
		}

		inline bool IsChromaKeyEnabled() const
		{
			return (_isChromaKeyEnabled_gameAddress_paletteIndex & 0x80) != 0;
		}

		inline void SetIsChromaKeyEnabled(bool enable)
		{
			_isChromaKeyEnabled_gameAddress_paletteIndex &= 0x7F;
			_isChromaKeyEnabled_gameAddress_paletteIndex |= enable ? 0x80 : 0;
		}

		inline PrimitiveType GetPrimitiveType() const
		{
			return (PrimitiveType)((_primitiveType_combiners >> 2) & 0x03);
		}

		inline void SetPrimitiveType(PrimitiveType primitiveType)
		{
			assert((int32_t)primitiveType < 4);
			_primitiveType_combiners &= ~0x0C;
			_primitiveType_combiners |= ((uint8_t)primitiveType << 2) & 0x0C;
		}

		inline RgbCombine GetRgbCombine() const
		{
			return (RgbCombine)(_primitiveType_combiners & 0x01);
		}

		inline void SetRgbCombine(RgbCombine rgbCombine)
		{
			assert((int32_t)rgbCombine >= 0 && (int32_t)rgbCombine < 2);
			_primitiveType_combiners &= ~0x01;
			_primitiveType_combiners |= (uint8_t)rgbCombine & 0x01;
		}

		inline AlphaCombine GetAlphaCombine() const
		{
			return (AlphaCombine)((_primitiveType_combiners >> 1) & 0x01);
		}

		inline void SetAlphaCombine(AlphaCombine alphaCombine)
		{
			assert((int32_t)alphaCombine >= 0 && (int32_t)alphaCombine < 2);
			_primitiveType_combiners &= ~0x02;
			_primitiveType_combiners |= ((uint8_t)alphaCombine << 1) & 0x02;
		}

		inline bool IsRgba() const
		{
			return (_isRgba_startVertexHigh & 0x10) != 0;
		}

		inline void SetIsRgba(bool isRgba)
		{
			_isRgba_startVertexHigh &= ~0x10;
			_isRgba_startVertexHigh |= isRgba ? 0x10 : 0;
		}

		inline int32_t GetWidth() const
		{
			return 1 << (((_textureHeight_textureWidth_alphaBlend >> 2) & 7) + 1);
		}

		inline int32_t GetHeight() const
		{
			return 1 << (((_textureHeight_textureWidth_alphaBlend >> 5) & 7) + 1);
		}

		inline void SetTextureSize(int32_t width, int32_t height)
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

		inline AlphaBlend GetAlphaBlend() const
		{
			return (AlphaBlend)(_textureHeight_textureWidth_alphaBlend & 3);
		}

		inline void SetAlphaBlend(AlphaBlend alphaBlend)
		{
			assert((uint32_t)alphaBlend < 4);
			_textureHeight_textureWidth_alphaBlend &= ~0x03;
			_textureHeight_textureWidth_alphaBlend |= (uint32_t)alphaBlend & 3;
		}

		inline int32_t GetStartVertex() const
		{
			return _startVertexLow | ((_isRgba_startVertexHigh & 0x0F) << 8);
		}

		inline void SetStartVertex(int32_t startVertex)
		{
			assert(startVertex <= 0xFFFFF);
			_startVertexLow = startVertex & 0xFFFF;
			_isRgba_startVertexHigh &= ~0x0F;
			_isRgba_startVertexHigh |= (startVertex >> 16) & 0x0F;
		}

		inline uint32_t GetVertexCount() const
		{
			return _vertexCount;
		}

		inline void SetVertexCount(uint32_t vertexCount)
		{
			assert(vertexCount >= 0 && vertexCount <= 0xFFFF);
			_vertexCount = vertexCount;
		}

		inline uint32_t SelectColorAndAlpha(uint32_t iteratedColor, uint32_t constantColor)
		{
			const auto rgbCombine = GetRgbCombine();
			uint32_t result = (rgbCombine == RgbCombine::ConstantColor ? constantColor : iteratedColor) & 0x00FFFFFF;
			result |= constantColor & 0xFF000000;
			return result;
		}

		inline uint32_t GetHash() const
		{
			return _textureHash;
		}

		void SetTextureHash(uint32_t textureHash)
		{
			_textureHash = textureHash;
		}

		inline uint32_t GetTextureMemSize() const
		{
			return (GetWidth() * GetHeight()) * (IsRgba() ? 4 : 1);
		}

		inline uint32_t GetAtlasIndex() const
		{
			return (uint32_t)(_textureCategory_atlasIndexLow & 0x0F) | (_atlasIndexHigh << 4);
		}

		inline void SetAtlasIndex(uint32_t atlasIndex)
		{
			assert(atlasIndex < 4096);

			_textureCategory_atlasIndexLow &= ~0x0F;
			_textureCategory_atlasIndexLow |= atlasIndex & 0x0F;

			_atlasIndexHigh = atlasIndex >> 4;
		}

		inline TextureCategory GetTextureCategory() const
		{
			return (TextureCategory)(_textureCategory_atlasIndexLow >> 4U);
		}

		inline void SetTextureCategory(TextureCategory category)
		{
			_textureCategory_atlasIndexLow &= 0x0F;
			_textureCategory_atlasIndexLow |= (uint32_t)category << 4U;
		}

		inline int32_t GetTextureStartAddress() const
		{
			const uint32_t startAddress = _textureStartAddress;
			return (startAddress - 1) << 8;
		}

		inline void SetTextureStartAddress(int32_t startAddress)
		{
			assert(!(startAddress & 255));
			assert(startAddress >= 0 && startAddress <= (D2DX_TMU_MEMORY_SIZE - 256));

			startAddress >>= 8;
			++startAddress;

			assert((startAddress & 0xFFFF) != 0);

			_textureStartAddress = startAddress & 0xFFFF;
		}

		inline bool IsStFlipped() const
		{
			return (_primitiveType_combiners & 0x10) != 0;
		}

		inline void SetIsStFlipped(bool isStFlipped)
		{
			_primitiveType_combiners &= ~0x10;
			_primitiveType_combiners |= isStFlipped ? 0x10 : 0;
		}

		inline bool IsValid() const
		{
			return _textureStartAddress != 0;
		}

	private:
		uint32_t _textureHash;
		uint16_t _startVertexLow;
		uint16_t _vertexCount;
		uint16_t _textureStartAddress;
		uint8_t _textureHeight_textureWidth_alphaBlend;			// HHHWWWBB
		uint8_t _atlasIndexHigh;								// AAAAAAAA
		uint8_t _textureCategory_atlasIndexLow;					// TTTTAAAA
		uint8_t _isChromaKeyEnabled_gameAddress_paletteIndex;
		uint8_t _primitiveType_combiners;						// xxxFPPCC
		uint8_t _isRgba_startVertexHigh;
	};

	static_assert(sizeof(Batch) == 16, "sizeof(Batch)");
}
