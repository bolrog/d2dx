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
	_gameHelper{ gameHelper },
	_unitPtrs{ 1024, true },
	_unitMotions{ 1024, true }
{
}

_Use_decl_annotations_
void UnitMotionPredictor::Update(
	IRenderContext* renderContext)
{
	const int32_t dt = renderContext->GetFrameTimeFp();
	int32_t expiredUnitIndex = -1;

	for (int32_t i = 0; i < _unitsCount; ++i)
	{
		const D2::UnitAny* unit = _unitPtrs.items[i];

		if (!_unitPtrs.items[i])
		{
			expiredUnitIndex = i;
			continue;
		}

		UnitMotion& um = _unitMotions.items[i];

		if ((_frame - um.lastUsedFrame) > 1 || unit->dwUnitId == 0xFFFFFFFF || unit->dwUnitId == 0)
		{
			_unitPtrs.items[i] = nullptr;
			expiredUnitIndex = i;
			continue;
		}

		const Offset pos = _gameHelper->GetUnitPos(unit);

		const Offset posWhole{ pos.x >> 16, pos.y >> 16 };
		const Offset predictedPosWhole{ um.predictedPos.x >> 16, um.predictedPos.y >> 16 };
		const int32_t predictionError = max(abs(posWhole.x - predictedPosWhole.x), abs(posWhole.y - predictedPosWhole.y));

		if (predictionError > 3)
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
			um.correctedPos.x = ((int64_t)pos.x + um.lastPos.x) >> 1;
			um.correctedPos.y = ((int64_t)pos.y + um.lastPos.y) >> 1;
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
				//D2DX_DEBUG_LOG("Predicted %f %f", um.predictedPos.x / 65536.0f, um.predictedPos.y / 65536.0f);

			/*	int32_t ex = um.correctedPos.x - um.predictedPos.x;
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
				}*/

				um.predictedPos.x += vStep.x;
				um.predictedPos.y += vStep.y;

				um.correctedPos.x += vStep.x;
				um.correctedPos.y += vStep.y;
			}
		}
	}

	// Gradually (one change per frame) compact the unit list.
	if (_unitsCount > 1)
	{
		if (!_unitPtrs.items[_unitsCount - 1])
		{
			// The last entry is expired. Shrink the list.
			_unitMotions.items[_unitsCount - 1] = { };
			--_unitsCount;
		}
		else if (expiredUnitIndex >= 0 && expiredUnitIndex < (_unitsCount - 1))
		{
			// Some entry is expired. Move the last entry to that place, and shrink the list.
			_unitPtrs.items[expiredUnitIndex] = _unitPtrs.items[_unitsCount - 1];
			_unitMotions.items[expiredUnitIndex] = _unitMotions.items[_unitsCount - 1];
			_unitPtrs.items[_unitsCount - 1] = nullptr;
			_unitMotions.items[_unitsCount - 1] = { };
			--_unitsCount;
		}
	}

	++_frame;
}

_Use_decl_annotations_
OffsetF UnitMotionPredictor::GetOffset(
	const D2::UnitAny* unit)
{
	int32_t unitIndex = -1;

	for (int32_t i = 0; i < _unitsCount; ++i)
	{
		if (_unitPtrs.items[i] == unit)
		{
			_unitMotions.items[i].lastUsedFrame = _frame;
			unitIndex = i;
			break;
		}
	}

	if (unitIndex < 0)
	{
		if (_unitsCount < (int32_t)_unitPtrs.capacity)
		{
			unitIndex = _unitsCount++;
			_unitPtrs.items[unitIndex] = unit;
			_unitMotions.items[unitIndex] = { };
			_unitMotions.items[unitIndex].lastUsedFrame = _frame;
		}
		else
		{
			D2DX_DEBUG_LOG("UMP: Too many units.");
		}
	}

	if (unitIndex < 0)
	{
		return { 0.0f, 0.0f };
	}

	UnitMotion& um = _unitMotions.items[unitIndex];
	return { (um.predictedPos.x - um.lastPos.x) / 65536.0f, (um.predictedPos.y - um.lastPos.y) / 65536.0f };
}
