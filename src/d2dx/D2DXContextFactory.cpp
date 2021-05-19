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
#include "D2DXContextFactory.h"
#include "GameHelper.h"
#include "SimdSse2.h"
#include "D2DXContext.h"
#include "CompatibilityModeDisabler.h"

using namespace d2dx;

static bool destroyed = false;
static std::shared_ptr<ID2DXContext> instance;

ID2DXContext* D2DXContextFactory::GetInstance(
	bool createIfNeeded)
{
	/* The game is single threaded and there's no worry about synchronization. */

	if (!instance && !destroyed && createIfNeeded)
	{
		auto gameHelper = std::make_shared<GameHelper>();
		auto simd = std::make_shared<SimdSse2>();
		auto compatibilityModeDisabler = std::make_shared<CompatibilityModeDisabler>();
		instance = std::make_shared<D2DXContext>(gameHelper, simd, compatibilityModeDisabler);
	}

	return instance.get();
}

void D2DXContextFactory::DestroyInstance()
{
	instance = nullptr;
	destroyed = true;
}
