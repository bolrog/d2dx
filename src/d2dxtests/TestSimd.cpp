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
#include "../d2gex/Simd.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace d2dx;

namespace d2dxtests
{
	TEST_CLASS(TestSimd)
	{
	public:
		TEST_METHOD(Create)
		{
			auto simd = Simd::Create();
		}

		TEST_METHOD(FindUInt32)
		{
			auto simd = Simd::Create();

			alignas(64) std::array<uint32_t, 1024> items;

			for (int32_t i = 0; i < 1024; ++i)
			{
				items[i] = 1023 - i;
			}

			Assert::AreEqual(0, simd->IndexOfUInt32(items.data(), items.size(), 1023));
			Assert::AreEqual(1023, simd->IndexOfUInt32(items.data(), items.size(), 0));
			Assert::AreEqual(1009, simd->IndexOfUInt32(items.data(), items.size(), 14));
			Assert::AreEqual(114, simd->IndexOfUInt32(items.data(), items.size(), 909));
		}
	};
}
