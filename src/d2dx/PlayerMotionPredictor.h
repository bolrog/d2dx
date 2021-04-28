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

#include "IGameHelper.h"
#include "IRenderContext.h"

namespace d2dx
{
	class PlayerMotionPredictor
	{
	public:
		PlayerMotionPredictor(
			_In_ const std::shared_ptr<IGameHelper>& gameHelper);

		void Update(
			_In_ IRenderContext* renderContext);

		void GetOffset(float& offsetX, float& offsetY);

	private:
		std::shared_ptr<IGameHelper> _gameHelper;
		int64_t _timeStart;
		int64_t _lastPlayerPosChangeTime = 0;
		int64_t _lastPlayerX[2] = { 0, 0 };
		int64_t _lastPlayerY[2] = { 0, 0 };
		int64_t _velocityX = 0;
		int64_t _velocityY = 0;
		int64_t _dt = 0;
		int64_t _itpPlayerX = 0;
		int64_t _itpPlayerY = 0;
		int64_t _itpPlayerTargetX = 0;
		int64_t _itpPlayerTargetY = 0;
		int64_t _dtLastPosChange = 0;
		int32_t _screenOffsetX = 0;
		int32_t _screenOffsetY = 0;
	};
}
