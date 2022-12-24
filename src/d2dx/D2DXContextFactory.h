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

#include "ID2DXContext.h"

namespace d2dx
{
	class D2DXContextFactory
	{
	public:
		static ID2DXContext* GetInstance(bool createIfNeeded = true);
		static void DestroyInstance();
	};


	class Timer final {
	public:
#pragma warning(push)
#pragma warning(disable: 26495)
		Timer(ProfCategory category) :
			category(category)
#ifdef D2DX_PROFILE
			, start(TimeStart())
#endif // D2DX_PROFILE
		{}
#pragma warning(pop)
#ifdef D2DX_PROFILE
		~Timer() {
			int64_t time = TimeStart() - start;
			if (auto context = D2DXContextFactory::GetInstance(false)) {
				context->AddTime(time, category);
			}
		}
#endif // D2DX_PROFILE

	private:
		ProfCategory category;
		int64_t start;
	};
}
