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

	auto playerPos = _gameHelper->GetPlayerPos();

	if (max(abs(playerPos.x - _predictedPlayerPos.x), abs(playerPos.y - _predictedPlayerPos.y)) > 15 * 65536)
	{
		this->_predictedPlayerPos = playerPos;
	}

	int32_t dx = playerPos.x - _lastPlayerPos.x;
	int32_t dy = playerPos.y - _lastPlayerPos.y;

	_dtLastPosChange += dt;

	if (dx != 0 || dy != 0)// || _dtLastPosChange > (65536/25))
	{
		int32_t ex = playerPos.x - _predictedPlayerPos.x;
		int32_t ey = playerPos.y - _predictedPlayerPos.y;

		_velocity.x = 25 * dx + 4 * ex;
		_velocity.y = 25 * dy + 4 * ey;

		_lastPlayerPos = playerPos;
		_lastPlayerPosChangeTime = newTime;
		_dtLastPosChange = 0;
	}

	if (_velocity.x != 0 || _velocity.y != 0)
	{
		if (_dtLastPosChange < (65536 / 25))
		{
			_predictedPlayerPos.x += (int32_t)(((int64_t)dt * _velocity.x) >> 16);
			_predictedPlayerPos.y += (int32_t)(((int64_t)dt * _velocity.y) >> 16);
		}
	}
}

void PlayerMotionPredictor::GetOffset(float& offsetX, float& offsetY)
{
	offsetX = (_predictedPlayerPos.x - _lastPlayerPos.x) / 65536.0f;
	offsetY = (_predictedPlayerPos.y - _lastPlayerPos.y) / 65536.0f;
}
