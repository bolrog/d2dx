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
#include "UnitMotionPredictor.h"

using namespace d2dx;

_Use_decl_annotations_
UnitMotionPredictor::UnitMotionPredictor(
	const std::shared_ptr<IGameHelper>& gameHelper) :
	_gameHelper{ gameHelper }
{
	memset(_unitPtrs, 0, sizeof(D2::UnitAny*) * ARRAYSIZE(_unitPtrs));
	memset(_unitMotions, 0, sizeof(UnitMotion) * ARRAYSIZE(_unitMotions));
}

_Use_decl_annotations_
void UnitMotionPredictor::Update(
	IRenderContext* renderContext)
{
	const int32_t dt = (int32_t)(renderContext->GetFrameTime() * 65536.0);

	for (int32_t i = 0; i < _unitsCount; ++i)
	{
		const D2::UnitAny* unit = _unitPtrs[i];

		if (!_unitPtrs[i])
		{
			continue;
		}

		UnitMotion& um = _unitMotions[i];

		if ((_frame - um.lastUsedFrame) > 1 || unit->dwUnitId == 0xFFFFFFFF || unit->dwUnitId == 0)
		{
			_unitPtrs[i] = nullptr;
			continue;
		}

		const Offset pos = _gameHelper->GetUnitPos(unit);

		if (max(abs(pos.x - um.predictedPos.x), abs(pos.y - um.predictedPos.y)) > 10 * 65536)
		{
			um.predictedPos = pos;
			um.correctedPos = pos;
			um.lastPos = pos;
			um.velocity = { 0,0 };
		}

		const int32_t dx = pos.x - um.lastPos.x;
		const int32_t dy = pos.y - um.lastPos.y;

		um.dtLastPosChange += dt;

		if (dx != 0 || dy != 0 || um.dtLastPosChange >= (65536 / 25))
		{
			um.correctedPos.x = (pos.x + um.lastPos.x) >> 1;
			um.correctedPos.y = (pos.y + um.lastPos.y) >> 1;
			//D2DX_DEBUG_LOG("Server %f %f", pos.x / 65536.0f, pos.y / 65536.0f);

			um.velocity.x = 25 * dx;
			um.velocity.y = 25 * dy;

			um.lastPos = pos;
			um.dtLastPosChange = 0;
		}

		if (um.velocity.x != 0 || um.velocity.y != 0)
		{
			if (um.dtLastPosChange < (65536 / 25))
			{
				Offset vStep{
					(int32_t)(((int64_t)dt * um.velocity.x) >> 16),
					(int32_t)(((int64_t)dt * um.velocity.y) >> 16) };

				const int32_t correctionAmount = 7000;
				const int32_t oneMinusCorrectionAmount = 65536 - correctionAmount;

				um.predictedPos.x = (int32_t)(((int64_t)um.predictedPos.x * oneMinusCorrectionAmount + (int64_t)um.correctedPos.x * correctionAmount) >> 16);
				um.predictedPos.y = (int32_t)(((int64_t)um.predictedPos.y * oneMinusCorrectionAmount + (int64_t)um.correctedPos.y * correctionAmount) >> 16);
				//D2DX_DEBUG_LOG("Predicted %f %f", _predictedPlayerPos.x / 65536.0f, _predictedPlayerPos.y / 65536.0f);

				int32_t ex = um.correctedPos.x - um.predictedPos.x;
				int32_t ey = um.correctedPos.y - um.predictedPos.y;

				if (unit->dwType == D2::UnitType::Player && (ex != 0 || ey != 0))
				{
					D2DX_DEBUG_LOG("%f, %f, %f, %f, %f, %f, %f, %f", 
						pos.x / 65536.0f, 
						pos.y / 65536.0f, 
						um.correctedPos.x / 65536.0f,
						um.correctedPos.y / 65536.0f,
						um.predictedPos.x / 65536.0f,
						um.predictedPos.y / 65536.0f,
						ex / 65536.0f,
						ey / 65536.0f);
				}

				um.predictedPos.x += vStep.x;
				um.predictedPos.y += vStep.y;

				um.correctedPos.x += vStep.x;
				um.correctedPos.y += vStep.y;
			}
		}
	}

	++_frame;
}

_Use_decl_annotations_
OffsetF UnitMotionPredictor::GetOffset(
	const D2::UnitAny* unit)
{
	int32_t unitIndex = -1;
	int32_t freeUnitIndex = -1;

	for (int32_t i = 0; i < _unitsCount; ++i)
	{
		if (!freeUnitIndex && !_unitPtrs[i])
		{
			freeUnitIndex = i;
		}

		if (_unitPtrs[i] == unit)
		{
			_unitMotions[i].lastUsedFrame = _frame;
			unitIndex = i;
			break;
		}
	}

	if (unitIndex < 0)
	{
		if (freeUnitIndex >= 0)
		{
			unitIndex = freeUnitIndex;
			_unitPtrs[unitIndex] = unit;
			_unitMotions[unitIndex] = { };
			_unitMotions[unitIndex].lastUsedFrame = _frame;
		}
		else if (_unitsCount < ARRAYSIZE(_unitPtrs))
		{
			unitIndex = _unitsCount++;
			_unitPtrs[unitIndex] = unit;
			_unitMotions[unitIndex] = { };
			_unitMotions[unitIndex].lastUsedFrame = _frame;
		}
		else
		{
			D2DX_DEBUG_LOG("UMP: Too many units.");
		}
	}

	if (unitIndex < 0)
	{
		D2DX_LOG("UMP: Did not find unit.");
		return { 0.0f, 0.0f };
	}

	UnitMotion& um = _unitMotions[unitIndex];
	return { (um.predictedPos.x - um.lastPos.x) / 65536.0f, (um.predictedPos.y - um.lastPos.y) / 65536.0f };
}
