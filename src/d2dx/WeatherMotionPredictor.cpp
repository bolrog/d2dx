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
#include "WeatherMotionPredictor.h"

using namespace d2dx;
using namespace DirectX;

_Use_decl_annotations_
WeatherMotionPredictor::WeatherMotionPredictor(
	const std::shared_ptr<IGameHelper>& gameHelper) :
	_gameHelper{ gameHelper },
	_particleMotions{ 512, true }
{
}

_Use_decl_annotations_
void WeatherMotionPredictor::Update(
	IRenderContext* renderContext)
{
	_dt = _gameHelper->IsGameMenuOpen() ? 0.0f : renderContext->GetFrameTime();
	++_frame;
}

_Use_decl_annotations_
OffsetF WeatherMotionPredictor::GetOffset(
	int32_t particleIndex,
	OffsetF posFromGame) 
{
	ParticleMotion& pm = _particleMotions.items[particleIndex & 511];

	float dx = posFromGame.x - pm.lastPos.x;
	float dy = posFromGame.y - pm.lastPos.y;

	float error = max(abs(dx), abs(dy));

	if (abs((int32_t)_frame - (int32_t)pm.lastUsedFrame) > 2 ||
		error > 100.0f)
	{
		pm.velocity = { 0.0f, 0.0f };
		pm.lastPos = posFromGame;
		pm.predictedPos = pm.lastPos;
	}
	else
	{
		if (dx != 0 || dy != 0)
		{
			pm.velocity = { 25.0f * dx, 25.0f * dy };
			pm.lastPos = posFromGame;
		}
	}

	pm.predictedPos.x += pm.velocity.x * _dt;
	pm.predictedPos.y += pm.velocity.y * _dt;

	pm.lastUsedFrame = _frame;

	return { pm.predictedPos.x - pm.lastPos.x, pm.predictedPos.y - pm.lastPos.y };
}
