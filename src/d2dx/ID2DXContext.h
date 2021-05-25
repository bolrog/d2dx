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

#include "IGlide3x.h"
#include "IWin32InterceptionHandler.h"
#include "ID2InterceptionHandler.h"
#include "Options.h"

namespace d2dx
{
	enum class Feature
	{
		UnitMotionPrediction = 1,
		WeatherMotionPrediction = 2,
		TextMotionPrediction = 4,
	};

	struct ID2DXContext abstract : 
		public IGlide3x,
		public IWin32InterceptionHandler,
		public ID2InterceptionHandler
	{
		virtual ~ID2DXContext() noexcept {}

		virtual void SetCustomResolution(
			_In_ Size size) = 0;

		virtual Size GetSuggestedCustomResolution() = 0;

		virtual GameVersion GetGameVersion() const = 0;

		virtual void DisableBuiltinResMod() = 0;

		virtual const Options& GetOptions() const = 0;
		
		virtual bool IsFeatureEnabled(
			_In_ Feature feature) = 0;
	};
}
