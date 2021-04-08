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

		void AssertThatGameSizeIsIntegerScale(Size desktopSize, bool wide)
		{
			auto suggestedGameSize = d2dx::Metrics::GetSuggestedGameSize(desktopSize, wide);
			auto renderRect = d2dx::Metrics::GetRenderRect(suggestedGameSize, desktopSize, wide);
			Assert::AreEqual(desktopSize.width, renderRect.size.width + renderRect.offset.x * 2);
			Assert::AreEqual(desktopSize.height, renderRect.size.height + renderRect.offset.y * 2);
		}

		TEST_METHOD(TestSuggestedGameSizeIsIntegerScale)
		{
			auto standardDesktopSizes = d2dx::Metrics::GetStandardDesktopSizes();
			for (uint32_t i = 0; i < standardDesktopSizes.capacity; ++i)
			{
				AssertThatGameSizeIsIntegerScale(standardDesktopSizes.items[i], false);
				AssertThatGameSizeIsIntegerScale(standardDesktopSizes.items[i], true);
			}
		}
	};
}
