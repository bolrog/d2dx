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
#include "../d2dx/Simd.h"
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
			auto simd = Simd::Create();
			auto textureProcessor = std::make_shared<TextureProcessor>();
			for (int32_t h = 3; h <= 8; ++h)
			{
				for (int32_t w = 3; w <= 8; ++w)
				{
					TextureCache textureCache(1 << w, 1 << h, 1024, NULL, simd, textureProcessor);
				}
			}
		}

		TEST_METHOD(FindNonExistentTexture)
		{
			auto simd = Simd::Create();
			auto textureProcessor = std::make_shared<TextureProcessor>();
			TextureCache textureCache(256, 128, 2048, NULL, simd, textureProcessor);
			auto tcl = textureCache.FindTexture(0x12345678, -1);
			Assert::AreEqual(-1, tcl.ArrayIndex);
		}

		TEST_METHOD(InsertAndFindTextures)
		{
			auto simd = Simd::Create();
			auto textureProcessor = std::make_shared<TextureProcessor>();
			std::array<uint32_t, 2 * 256 * 128> tmuData;

			Batch batch;
			batch.SetTextureStartAddress(0);
			batch.SetTextureSize(256, 128);

			TextureCache textureCache(256, 128, 64, NULL, simd, textureProcessor);

			for (uint32_t i = 0; i < 64; ++i)
			{
				uint32_t hash = (0xFF << 24) | (i << 16) | (i << 8) | i;
				textureCache.InsertTexture(hash, batch, (const uint8_t*)tmuData.data());
			}

			for (uint32_t i = 0; i < 64; ++i)
			{
				uint32_t hash = (0xFF << 24) | (i << 16) | (i << 8) | i;
				auto tcl = textureCache.FindTexture(hash, -1);
				Assert::AreEqual((int32_t)i, tcl.ArrayIndex);
			}
		}

		TEST_METHOD(FirstInsertedTextureIsReplaced)
		{
			auto simd = Simd::Create();
			auto textureProcessor = std::make_shared<TextureProcessor>();
			std::array<uint32_t, 2 * 256 * 128> tmuData;

			Batch batch;
			batch.SetTextureStartAddress(0);
			batch.SetTextureSize(256, 128);

			TextureCache textureCache(256, 128, 64, NULL, simd, textureProcessor);

			for (uint32_t i = 0; i < 65; ++i)
			{
				uint32_t hash = (0xFF << 24) | (i << 16) | (i << 8) | i;
				auto tcl = textureCache.InsertTexture(hash, batch, (const uint8_t*)tmuData.data());

				if (i == 64)
				{
					Assert::AreEqual(0, tcl.ArrayIndex);
				}
				else
				{
					Assert::AreEqual((int32_t)i, tcl.ArrayIndex);
				}
			}

			for (uint32_t i = 0; i < 64; ++i)
			{
				uint32_t hash = (0xFF << 24) | (i << 16) | (i << 8) | i;
				auto tcl = textureCache.FindTexture(hash, -1);
				
				int32_t expectedTextureIndex = i;

				if (i == 0)
				{
					expectedTextureIndex = -1;
				}
				else if (i == 64)
				{
					expectedTextureIndex = 0;
				}

				Assert::AreEqual(expectedTextureIndex, tcl.ArrayIndex);
			}
		}

		TEST_METHOD(SecondInsertedTextureIsReplacedIfFirstOneWasUsedInFrame)
		{
			auto simd = Simd::Create();
			auto textureProcessor = std::make_shared<TextureProcessor>();
			std::array<uint32_t, 2 * 256 * 128> tmuData;

			Batch batch;
			batch.SetTextureStartAddress(0);
			batch.SetTextureSize(256, 128);

			TextureCache textureCache(256, 128, 64, NULL, simd, textureProcessor);

			for (uint32_t i = 0; i < 65; ++i)
			{
				uint32_t hash = (0xFF << 24) | (i << 16) | (i << 8) | i;

				if (i == 64)
				{
					// Simulate new frame and use of texture in slot 0
					textureCache.OnNewFrame();
					Assert::AreEqual(0, textureCache.FindTexture(0xFF000000, -1).ArrayIndex);
				}

				auto tcl = textureCache.InsertTexture(hash, batch, (const uint8_t*)tmuData.data());

				if (i == 64)
				{
					Assert::AreEqual(1, tcl.ArrayIndex);
				}
				else
				{
					Assert::AreEqual((int32_t)i, tcl.ArrayIndex);
				}
			}

			for (uint32_t i = 0; i < 64; ++i)
			{
				uint32_t hash = (0xFF << 24) | (i << 16) | (i << 8) | i;
				auto tcl = textureCache.FindTexture(hash, -1);

				int32_t expectedTextureIndex = i;

				if (i == 1)
				{
					expectedTextureIndex = -1;
				}
				else if (i == 64)
				{
					expectedTextureIndex = 1;
				}

				Assert::AreEqual(expectedTextureIndex, tcl.ArrayIndex);
			}
		}
	};
}
