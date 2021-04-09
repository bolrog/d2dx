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
#include "../d2dx/Metrics.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace d2dx;
using namespace DirectX;

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			template<> static std::wstring ToString<Offset>(const Offset& t) { return ToString(t.x) + L", " + ToString(t.y); }
			template<> static std::wstring ToString<Size>(const Size& t) { return ToString(t.width) + L"x" + ToString(t.height); }
		}
	}
}

namespace d2dxtests
{
	TEST_CLASS(TestStandardMetrics)
	{
	public:
		TEST_METHOD(TestGetStandardDesktopSizes)
		{
			auto standardDesktopSizes = d2dx::Metrics::GetStandardDesktopSizes();
			Assert::IsTrue(standardDesktopSizes.capacity > 8);
		}

		TEST_METHOD(TestGetGameSize)
		{
			auto suggestedGameSize = d2dx::Metrics::GetSuggestedGameSize({ 1920, 1080 }, false);
			Assert::AreEqual(Size(720, 540), suggestedGameSize);
			suggestedGameSize = d2dx::Metrics::GetSuggestedGameSize({ 1000, 500 }, true);
			Assert::AreEqual(Size(1000, 500), suggestedGameSize);
			suggestedGameSize = d2dx::Metrics::GetSuggestedGameSize({ 2001, 1003 }, true);
			Assert::AreEqual(Size(1000, 501), suggestedGameSize);
		}

		void AssertThatGameSizeIsIntegerScale(Size desktopSize, bool wide, bool lenient)
		{
			auto suggestedGameSize = d2dx::Metrics::GetSuggestedGameSize(desktopSize, wide);
			auto renderRect = d2dx::Metrics::GetRenderRect(suggestedGameSize, desktopSize, wide);
			Assert::IsTrue(renderRect.offset.x >= 0);
			Assert::IsTrue(renderRect.offset.y >= 0);
			Assert::IsTrue(renderRect.size.width > 0);
			Assert::IsTrue(renderRect.size.height > 0);
			Assert::IsTrue((renderRect.offset.x + renderRect.size.width) <= desktopSize.width);
			Assert::IsTrue((renderRect.offset.y + renderRect.size.height) <= desktopSize.height);

			if (renderRect.offset.x > 0 && renderRect.offset.y > 0)
			{
				Assert::IsTrue(renderRect.offset.x < 16 || renderRect.offset.y < 16);
			}

			int32_t reconstructedDesktopWidth = renderRect.size.width + renderRect.offset.x * 2;
			int32_t reconstructedDesktopHeight = renderRect.size.height + renderRect.offset.y * 2;

			if (lenient)
			{
				/* The "remainder" on the right may not be exactly equal to desktop width. */
				Assert::IsTrue(desktopSize.width == reconstructedDesktopWidth || desktopSize.width == reconstructedDesktopWidth + 1);

				/* The "remainder" on the bottom may not be exactly equal to desktop height. */
				Assert::IsTrue(desktopSize.height == reconstructedDesktopHeight || desktopSize.height == reconstructedDesktopHeight + 1);
			}
			else
			{
				Assert::AreEqual(desktopSize.width, reconstructedDesktopWidth);
				Assert::AreEqual(desktopSize.height, reconstructedDesktopHeight);
			}
		}

		TEST_METHOD(TestRenderRectsAreGoodForStandardDesktopSizes)
		{
			auto standardDesktopSizes = d2dx::Metrics::GetStandardDesktopSizes();
			for (uint32_t i = 0; i < standardDesktopSizes.capacity; ++i)
			{
				AssertThatGameSizeIsIntegerScale(standardDesktopSizes.items[i], false, false);
				AssertThatGameSizeIsIntegerScale(standardDesktopSizes.items[i], true, false);
			}
		}

		TEST_METHOD(TestRenderRectsAreGoodForNonStandardDesktopSizes)
		{
			for (int32_t width = 50; width < 1600; ++width)
			{
				AssertThatGameSizeIsIntegerScale({ width, 503 }, false, true);
				AssertThatGameSizeIsIntegerScale({ width, 503 }, true, true);
			}
			for (int32_t height = 50; height < 1600; ++height)
			{
				AssertThatGameSizeIsIntegerScale({ 614, height }, false, true);
				AssertThatGameSizeIsIntegerScale({ 614, height }, true, true);
			}
		}
	};
}
