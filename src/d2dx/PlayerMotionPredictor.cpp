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
	_gameHelper{ gameHelper }
{
}

_Use_decl_annotations_
void PlayerMotionPredictor::Update(
	IRenderContext* renderContext)
{
	const int32_t dt = (int32_t)(renderContext->GetFrameTime() * 65536.0);
	const Offset playerPos = _gameHelper->GetPlayerPos();

	if (max(abs(playerPos.x - _predictedPlayerPos.x), abs(playerPos.y - _predictedPlayerPos.y)) > 15 * 65536)
	{
		_predictedPlayerPos = playerPos;
		_correctedPlayerPos = playerPos;
	}

	const int32_t dx = playerPos.x - _lastPlayerPos.x;
	const int32_t dy = playerPos.y - _lastPlayerPos.y;

	_dtLastPosChange += dt;

	if (dx != 0 || dy != 0 || _dtLastPosChange >= (65536 / 25))
	{
		_correctedPlayerPos = playerPos;
		D2DX_DEBUG_LOG("Server %f %f", playerPos.x / 65536.0f, playerPos.y / 65536.0f);

		_velocity.x = 25 * dx;
		_velocity.y = 25 * dy;

		_lastPlayerPos = playerPos;
		_dtLastPosChange = 0;
	}

	if (_velocity.x != 0 || _velocity.y != 0)
	{
		if (_dtLastPosChange < (65536 / 25))
		{
			Offset vStep{
				(int32_t)(((int64_t)dt * _velocity.x) >> 16),
				(int32_t)(((int64_t)dt * _velocity.y) >> 16) };

			const int32_t correctionAmount = 3000;
			const int32_t oneMinusCorrectionAmount = 65536 - correctionAmount;

			_predictedPlayerPos.x = (int32_t)(((int64_t)_predictedPlayerPos.x * oneMinusCorrectionAmount + (int64_t)_correctedPlayerPos.x * correctionAmount) >> 16);
			_predictedPlayerPos.y = (int32_t)(((int64_t)_predictedPlayerPos.y * oneMinusCorrectionAmount + (int64_t)_correctedPlayerPos.y * correctionAmount) >> 16);
			//D2DX_DEBUG_LOG("Predicted %f %f", _predictedPlayerPos.x / 65536.0f, _predictedPlayerPos.y / 65536.0f);

			int32_t ex = _correctedPlayerPos.x - _predictedPlayerPos.x;
			int32_t ey = _correctedPlayerPos.y - _predictedPlayerPos.y;
			D2DX_DEBUG_LOG("Error %f %f", ex / 65536.0f, ey / 65536.0f);

			_predictedPlayerPos.x += vStep.x;
			_predictedPlayerPos.y += vStep.y;

			_correctedPlayerPos.x += vStep.x;
			_correctedPlayerPos.y += vStep.y;
		}
	}
}

void PlayerMotionPredictor::GetOffset(float& offsetX, float& offsetY)
{
	offsetX = (_predictedPlayerPos.x - _lastPlayerPos.x) / 65536.0f;
	offsetY = (_predictedPlayerPos.y - _lastPlayerPos.y) / 65536.0f;
}
