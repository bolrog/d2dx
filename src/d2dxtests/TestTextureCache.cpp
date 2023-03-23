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
#include "pch.h"
#include <array>
#include "CppUnitTest.h"

#include "../d2dx/Batch.h"
#include "../d2dx/SimdSse2.h"
#include "../d2dx/Types.h"
#include "../d2dx/TextureCache.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace d2dx;

namespace d2dxtests
{
	TEST_CLASS(TestTextureCache)
	{
	public:
		TEST_METHOD(CreateAtlas)
		{
			auto simd = std::make_shared<SimdSse2>();
			for (int32_t h = 3; h <= 8; ++h)
			{
				for (int32_t w = 3; w <= 8; ++w)
				{
					auto textureCache = std::make_unique<TextureCache>(
						1 << w, 1 << h, 1024, 512, (ID3D11Device*)nullptr, simd);
				}
			}
		}

		TEST_METHOD(FindNonExistentTexture)
		{
			auto simd = std::make_shared<SimdSse2>();
			auto textureCache = std::make_unique<TextureCache>(256, 128, 2048, 512, (ID3D11Device*)nullptr, simd);
			auto tcl = textureCache->FindTexture(0x12345678, -1);
			Assert::AreEqual((int16_t)-1, tcl._textureAtlas);
			Assert::AreEqual((int16_t)-1, tcl._textureIndex);
		}

		TEST_METHOD(InsertAndFindTextures)
		{
			auto simd = std::make_shared<SimdSse2>();
			auto tmuData = std::make_unique<std::array<uint32_t, 2 * 256 * 128>>();

			Batch batch;
			batch.SetTextureStartAddress(0);
			batch.SetTextureSize(256, 128);

			auto textureCache = std::make_unique<TextureCache>(256, 128, 64, 512, (ID3D11Device*)nullptr, simd);

			for (uint64_t i = 0; i < 64; ++i)
			{
				uint64_t hash = (0xFFull << 24) | (i << 16) | (i << 8) | i;
				textureCache->InsertTexture(hash, batch, (const uint8_t*)tmuData->data(), (uint32_t)tmuData->size());
			}

			for (uint64_t i = 0; i < 64; ++i)
			{
				uint64_t hash = (0xFFull << 24) | (i << 16) | (i << 8) | i;
				auto tcl = textureCache->FindTexture(hash, -1);
				Assert::AreEqual((int16_t)0, tcl._textureAtlas);
				Assert::AreEqual((int16_t)i, tcl._textureIndex);
			}
		}

		TEST_METHOD(FirstInsertedTextureIsReplaced)
		{
			auto simd = std::make_shared<SimdSse2>();
			auto tmuData = std::make_unique<std::array<uint32_t, 2 * 256 * 128>>();

			Batch batch;
			batch.SetTextureStartAddress(0);
			batch.SetTextureSize(256, 128);

			auto textureCache = std::make_unique<TextureCache>(256, 128, 64, 512, (ID3D11Device*)nullptr, simd);

			for (uint64_t i = 0; i < 65; ++i)
			{
				uint64_t hash = (0xFFull << 24) | (i << 16) | (i << 8) | i;
				auto tcl = textureCache->InsertTexture(hash, batch, (const uint8_t*)tmuData->data(), (uint32_t)tmuData->size());

				if (i == 64)
				{
					Assert::AreEqual((int16_t)0, tcl._textureAtlas);
					Assert::AreEqual((int16_t)0, tcl._textureIndex);
				}
				else
				{
					Assert::AreEqual((int16_t)0, tcl._textureAtlas);
					Assert::AreEqual((int16_t)i, tcl._textureIndex);
				}
			}

			for (uint64_t i = 0; i < 64; ++i)
			{
				uint64_t hash = (0xFFull << 24) | (i << 16) | (i << 8) | i;
				auto tcl = textureCache->FindTexture(hash, -1);
				
				int16_t expectedTextureAtlas = 0;
				int16_t expectedTextureIndex = i;

				if (i == 0)
				{
					expectedTextureAtlas = -1;
					expectedTextureIndex = -1;
				}
				else if (i == 64)
				{
					expectedTextureIndex = 0;
				}
				
				Assert::AreEqual(expectedTextureAtlas, tcl._textureAtlas);
				Assert::AreEqual(expectedTextureIndex, tcl._textureIndex);
			}
		}

		TEST_METHOD(SecondInsertedTextureIsReplacedIfFirstOneWasUsedInFrame)
		{
			auto simd = std::make_shared<SimdSse2>();
			auto tmuData = std::make_unique<std::array<uint32_t, 2 * 256 * 128>>();

			Batch batch;
			batch.SetTextureStartAddress(0);
			batch.SetTextureSize(256, 128);

			auto textureCache = std::make_unique<TextureCache>(256, 128, 64, 512, (ID3D11Device*)nullptr, simd);

			for (uint64_t i = 0; i < 65; ++i)
			{
				uint64_t hash = (0xFFull << 24) | (i << 16) | (i << 8) | i;

				if (i == 64)
				{
					// Simulate new frame and use of texture in slot 0
					textureCache->OnNewFrame();
					auto tcl = textureCache->FindTexture(0xFF000000, -1);
					Assert::AreEqual((int16_t)0, tcl._textureAtlas);
					Assert::AreEqual((int16_t)0, tcl._textureIndex);
				}

				auto tcl = textureCache->InsertTexture(hash, batch, (const uint8_t*)tmuData->data(), (uint32_t)tmuData->size());

				if (i == 64)
				{
					Assert::AreEqual((int16_t)0, tcl._textureAtlas);
					Assert::AreEqual((int16_t)1, tcl._textureIndex);
				}
				else
				{
					Assert::AreEqual((int16_t)0, tcl._textureAtlas);
					Assert::AreEqual((int16_t)i, tcl._textureIndex);
				}
			}

			for (uint64_t i = 0; i < 64; ++i)
			{
				uint64_t hash = (0xFFull << 24) | (i << 16) | (i << 8) | i;
				auto tcl = textureCache->FindTexture(hash, -1);

				int16_t expectedTextureAtlas = 0;
				int16_t expectedTextureIndex = i;

				if (i == 1)
				{
					expectedTextureAtlas = -1;
					expectedTextureIndex = -1;
				}
				else if (i == 64)
				{
					expectedTextureIndex = 1;
				}

				Assert::AreEqual(expectedTextureAtlas, tcl._textureAtlas);
				Assert::AreEqual(expectedTextureIndex, tcl._textureIndex);
			}
		}
	};
}
