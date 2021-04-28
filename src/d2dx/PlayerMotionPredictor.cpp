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
#include "PlayerMotionPredictor.h"
#include "IGameHelper.h"
#include "Utils.h"

using namespace d2dx;

_Use_decl_annotations_
PlayerMotionPredictor::PlayerMotionPredictor(
	const std::shared_ptr<IGameHelper>& gameHelper) :
	_gameHelper{ gameHelper },
	_timeStart{ TimeStart() }
{
}

_Use_decl_annotations_
void PlayerMotionPredictor::Update(
	IRenderContext* renderContext)
{
	int64_t newTime = (int64_t)(65536.0 * TimeEndMs(_timeStart) / 1000.0);
	int64_t dt = (int64_t)(renderContext->GetFrameTime() * 65536.0);

	auto playerPosition = _gameHelper->GetPlayerPos();

	int64_t x = playerPosition.x;
	int64_t y = playerPosition.y;

	if (max(abs(x - _itpPlayerX), abs(y - _itpPlayerY)) > 50 * 65536)
	{
		this->_itpPlayerTargetX = x;
		this->_itpPlayerTargetY = y;
		this->_itpPlayerX = x;
		this->_itpPlayerY = y;
	}

	int64_t dx = x - _lastPlayerX[0];
	int64_t dy = y - _lastPlayerY[0];

	_dtLastPosChange += dt;

	if (dx != 0 || dy != 0)// || _dtLastPosChange > (65536/25))
	{
		int64_t ex = x - _itpPlayerTargetX;
		int64_t ey = y - _itpPlayerTargetY;

		_velocityX = 25 * dx + 4 * ex;
		_velocityY = 25 * dy + 4 * ey;

		_lastPlayerX[0] = x;
		_lastPlayerY[0] = y;
		_lastPlayerPosChangeTime = newTime;
		_dtLastPosChange = 0;
	}

	if (_velocityX != 0.0f || _velocityY != 0.0f)
	{
		if (_dtLastPosChange < (65536 / 25))
		{
			_itpPlayerTargetX += (dt * _velocityX) >> 16;
			_itpPlayerTargetY += (dt * _velocityY) >> 16;
		}
	}

	_itpPlayerX = _itpPlayerTargetX;
	_itpPlayerY = _itpPlayerTargetY;

	//_itpPlayerX += ((_itpPlayerTargetX - _itpPlayerX) * 65000) >> 16;
	//_itpPlayerY += ((_itpPlayerTargetY - _itpPlayerY) * 65000) >> 16;
}

void PlayerMotionPredictor::GetOffset(float& offsetX, float& offsetY)
{
	offsetX = (_itpPlayerX - _lastPlayerX[0]) / 65536.0f;
	offsetY = (_itpPlayerY - _lastPlayerY[0]) / 65536.0f;
}