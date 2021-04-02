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
#include "pch.h"
#include <array>
#include "CppUnitTest.h"
#include "../d2dx/Batch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace d2dx;

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			template<> static std::wstring ToString<d2dx::AlphaBlend>(const d2dx::AlphaBlend& t) { return ToString((int32_t)t); }
			template<> static std::wstring ToString<d2dx::PrimitiveType>(const d2dx::PrimitiveType& t) { return ToString((int32_t)t); }
			template<> static std::wstring ToString<d2dx::AlphaCombine>(const d2dx::AlphaCombine& t) { return ToString((int32_t)t); }
			template<> static std::wstring ToString<d2dx::RgbCombine>(const d2dx::RgbCombine& t) { return ToString((int32_t)t); }
			template<> static std::wstring ToString<d2dx::TextureCategory>(const d2dx::TextureCategory& t) { return ToString((int32_t)t); }
			template<> static std::wstring ToString<d2dx::GameAddress>(const d2dx::GameAddress& t) { return ToString((int32_t)t); }
		}
	}
}

namespace d2dxtests
{
	TEST_CLASS(TestBatch)
	{
	public:
		TEST_METHOD(DefaultValues)
		{
			Batch batch;
			Assert::IsFalse(batch.IsValid());
			Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
			Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
			Assert::AreEqual(0U, batch.GetAtlasIndex());
			Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
			Assert::AreEqual(0U, batch.GetHash());
			Assert::AreEqual(2, batch.GetHeight());
			Assert::AreEqual(0, batch.GetPaletteIndex());
			Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
			Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
			Assert::AreEqual(0, batch.GetStartVertex());
			Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
			Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
			Assert::AreEqual(0U, batch.GetVertexCount());
			Assert::AreEqual(2, batch.GetWidth());
		}

		TEST_METHOD(SetAlphaBlend)
		{
			Batch batch;
			for (int32_t i = 0; i < (int32_t)AlphaBlend::Count; ++i)
			{
				batch.SetAlphaBlend((d2dx::AlphaBlend)i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual((d2dx::AlphaBlend)i, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetAlphaCombine)
		{
			Batch batch;
			for (int32_t i = 0; i < (int32_t)AlphaCombine::Count; ++i)
			{
				batch.SetAlphaCombine((d2dx::AlphaCombine)i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual((d2dx::AlphaCombine)i, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetAtlasIndex)
		{
			Batch batch;
			for (uint32_t i = 0; i < 4096; ++i)
			{
				batch.SetAtlasIndex(i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(i, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetGameAddress)
		{
			Batch batch;
			for (uint32_t i = 0; i < (int32_t)GameAddress::Count; ++i)
			{
				batch.SetGameAddress((GameAddress)i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual((GameAddress)i, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetHash)
		{
			Batch batch;
			for (uint64_t i = 0; i < (1ULL << 32); i += 999431) /* prime */
			{
				batch.SetTextureHash((uint32_t)i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual((uint32_t)i, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetTextureSize)
		{
			Batch batch;
			for (int32_t h = 3; h <= 8; ++h)
			{
				for (int32_t w = 3; w <= 8; ++w)
				{
					batch.SetTextureSize(1 << w, 1 << h);
					Assert::IsFalse(batch.IsValid());
					Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
					Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
					Assert::AreEqual(0U, batch.GetAtlasIndex());
					Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
					Assert::AreEqual(0U, batch.GetHash());
					Assert::AreEqual(1 << h, batch.GetHeight());
					Assert::AreEqual(0, batch.GetPaletteIndex());
					Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
					Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
					Assert::AreEqual(0, batch.GetStartVertex());
					Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
					Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
					Assert::AreEqual(0U, batch.GetVertexCount());
					Assert::AreEqual(1 << w, batch.GetWidth());
				}
			}
		}

		TEST_METHOD(SetPaletteIndex)
		{
			Batch batch;
			for (int32_t i = 0; i < D2DX_MAX_PALETTES; ++i)
			{
				batch.SetPaletteIndex(i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(i, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetPrimitiveType)
		{
			Batch batch;
			for (int32_t i = 0; i < (int32_t)PrimitiveType::Count; ++i)
			{
				batch.SetPrimitiveType((PrimitiveType)i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual((PrimitiveType)i, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetRgbCombine)
		{
			Batch batch;
			for (int32_t i = 0; i < (int32_t)RgbCombine::Count; ++i)
			{
				batch.SetRgbCombine((RgbCombine)i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual((RgbCombine)i, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetStartVertex)
		{
			Batch batch;
			for (int32_t i = 0; i < 1000000; i += 127) /* prime */
			{
				batch.SetStartVertex(i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(i, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetTextureCategory)
		{
			Batch batch;
			for (int32_t i = 0; i < (int32_t)TextureCategory::Count; ++i)
			{
				batch.SetTextureCategory((TextureCategory)i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual((TextureCategory)i, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetTextureStartAddress)
		{
			Batch batch;
			for (int32_t i = 0; i < D2DX_TMU_MEMORY_SIZE - 256; i += (7 * 256)) /* multiple of prime */
			{
				batch.SetTextureStartAddress(i);
				Assert::IsTrue(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(i, batch.GetTextureStartAddress());
				Assert::AreEqual(0U, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}

		TEST_METHOD(SetVertexCount)
		{
			Batch batch;
			for (uint32_t i = 0; i < 65536; i += 7) /* prime */
			{
				batch.SetVertexCount(i);
				Assert::IsFalse(batch.IsValid());
				Assert::AreEqual(AlphaBlend::Opaque, batch.GetAlphaBlend());
				Assert::AreEqual(AlphaCombine::One, batch.GetAlphaCombine());
				Assert::AreEqual(0U, batch.GetAtlasIndex());
				Assert::AreEqual(GameAddress::Unknown, batch.GetGameAddress());
				Assert::AreEqual(0U, batch.GetHash());
				Assert::AreEqual(2, batch.GetHeight());
				Assert::AreEqual(0, batch.GetPaletteIndex());
				Assert::AreEqual(PrimitiveType::Points, batch.GetPrimitiveType());
				Assert::AreEqual(RgbCombine::ColorMultipliedByTexture, batch.GetRgbCombine());
				Assert::AreEqual(0, batch.GetStartVertex());
				Assert::AreEqual(TextureCategory::Unknown, batch.GetTextureCategory());
				Assert::AreEqual(-D2DX_TMU_ADDRESS_ALIGNMENT, batch.GetTextureStartAddress());
				Assert::AreEqual(i, batch.GetVertexCount());
				Assert::AreEqual(2, batch.GetWidth());
			}
		}
	};
}
