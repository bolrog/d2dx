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
using namespace DirectX;

_Use_decl_annotations_
UnitMotionPredictor::UnitMotionPredictor(
	const std::shared_ptr<IGameHelper>& gameHelper) :
	_gameHelper{ gameHelper },
	_unitIdAndTypes{ 1024, true },
	_unitMotions{ 1024, true },
	_unitScreenPositions{ 1024, true },
	_units{ 1024, true },
	_prevUnits{ 1024, true }
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
		UnitIdAndType& uiat = _unitIdAndTypes.items[i];

		if (!uiat.unitId)
		{
			expiredUnitIndex = i;
			continue;
		}

		auto unit = _gameHelper->FindUnit(uiat.unitId, (D2::UnitType)uiat.unitType);

		if (!unit)
		{
			uiat.unitId = 0;
			expiredUnitIndex = i;
			continue;
		}

		UnitMotion& um = _unitMotions.items[i];
		const Offset pos = _gameHelper->GetUnitPos(unit);

		Offset posWhole{ pos.x >> 16, pos.y >> 16 };
		Offset lastPosWhole{ um.lastPos.x >> 16, um.lastPos.y >> 16 };
		Offset predictedPosWhole{ um.predictedPos.x >> 16, um.predictedPos.y >> 16 };

		int32_t lastPosMd = max(abs(posWhole.x - lastPosWhole.x), abs(posWhole.y - lastPosWhole.y));
		int32_t predictedPosMd = max(abs(posWhole.x - predictedPosWhole.x), abs(posWhole.y - predictedPosWhole.y));

		if (lastPosMd > 2 || predictedPosMd > 2)
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
		if (!_unitIdAndTypes.items[_unitsCount - 1].unitId)
		{
			// The last entry is expired. Shrink the list.
			_unitMotions.items[_unitsCount - 1] = { };
			--_unitsCount;
		}
		else if (expiredUnitIndex >= 0 && expiredUnitIndex < (_unitsCount - 1))
		{
			// Some entry is expired. Move the last entry to that place, and shrink the list.
			_unitIdAndTypes.items[expiredUnitIndex] = _unitIdAndTypes.items[_unitsCount - 1];
			_unitMotions.items[expiredUnitIndex] = _unitMotions.items[_unitsCount - 1];
			_unitIdAndTypes.items[_unitsCount - 1] = { 0,0 };
			_unitMotions.items[_unitsCount - 1] = { };
			--_unitsCount;
		}
	}

	++_frame;
}

_Use_decl_annotations_
Offset UnitMotionPredictor::GetOffset(
	const D2::UnitAny* unit)
{
	int32_t unitIndex = -1;

	auto unitId = _gameHelper->GetUnitId(unit);
	auto unitType = _gameHelper->GetUnitType(unit);

	for (int32_t i = 0; i < _unitsCount; ++i)
	{
		if (_unitIdAndTypes.items[i].unitId == unitId &&
			_unitIdAndTypes.items[i].unitType == (uint32_t)unitType)
		{
			_unitMotions.items[i].lastUsedFrame = _frame;
			unitIndex = i;
			break;
		}
	}

	if (unitIndex < 0)
	{
		if (_unitsCount < (int32_t)_unitIdAndTypes.capacity)
		{
			unitIndex = _unitsCount++;
			_unitIdAndTypes.items[unitIndex].unitId = unitId;
			_unitIdAndTypes.items[unitIndex].unitType = (uint32_t)unitType;
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
		return { 0, 0 };
	}

	return _unitMotions.items[unitIndex].GetOffset();
}

_Use_decl_annotations_
void UnitMotionPredictor::SetUnitScreenPos(
	const D2::UnitAny* unit,
	int32_t x,
	int32_t y)
{
	auto unitId = _gameHelper->GetUnitId(unit);
	auto unitType = _gameHelper->GetUnitType(unit);

	for (int32_t unitIndex = 0; unitIndex < _unitsCount; ++unitIndex)
	{
		if (_unitIdAndTypes.items[unitIndex].unitId == unitId &&
			_unitIdAndTypes.items[unitIndex].unitType == (uint32_t)unitType)
		{
			_unitScreenPositions.items[unitIndex] = { x, y };
			break;
		}
	}
}

_Use_decl_annotations_
Offset UnitMotionPredictor::GetOffsetForShadow(
	_In_ int32_t x,
	_In_ int32_t y)
{
	for (int32_t i = 0; i < _unitsCount; ++i)
	{
		if (!_unitIdAndTypes.items[i].unitId)
		{
			continue;
		}

		const int32_t dist = max(abs(_unitScreenPositions.items[i].x - x), abs(_unitScreenPositions.items[i].y - y));

		if (dist < 8)
		{
			return _unitMotions.items[i].GetOffset();
		}
	}

	return { 0, 0 };
}

Offset UnitMotionPredictor::UnitMotion::GetOffset() const
{
	const OffsetF offset{ (predictedPos.x - lastPos.x) / 65536.0f, (predictedPos.y - lastPos.y) / 65536.0f };
	const OffsetF scaleFactors{ 32.0f / sqrtf(2.0f), 16.0f / sqrtf(2.0f) };
	const OffsetF screenOffset = scaleFactors * OffsetF{ offset.x - offset.y, offset.x + offset.y } + 0.5f;
	return { (int32_t)screenOffset.x, (int32_t)screenOffset.y };
}

void UnitMotionPredictor::AddUnit(
	D2::UnitAny* unit)
{
	if (_unitsCount2 <= 1024) {
		_units.items[_unitsCount2++] = unit;
	}
}

bool isSameUnit(
	IGameHelper* gameHelper,
	D2::UnitAny* a,
	D2::UnitAny* b)
{
	return a == b
		&& gameHelper->GetUnitId(a) == gameHelper->GetUnitId(b)
		&& gameHelper->GetUnitType(a) == gameHelper->GetUnitType(b);
}

void UnitMotionPredictor::OnBufferClear()
{
	D2::UnitAny* removedItems[1024];
	uint32_t removedItemsCount = 0;
	uint32_t i = 0;

	for (
		uint32_t end = min(_unitsCount2, _prevUnitsCount);
		i != end;
		++i)
	{
		D2::UnitAny* unit = _units.items[i];
		D2::UnitAny* prevUnit = _prevUnits.items[i];

		if (!isSameUnit(_gameHelper.get(), unit, prevUnit)) {
			removedItems[removedItemsCount++] = prevUnit;
		}
		_prevUnits.items[i] = unit;
	}

	if (i < _prevUnitsCount) {
		uint32_t count = _prevUnitsCount - i;
		memcpy(&removedItems[removedItemsCount], &_prevUnits.items[i], count * sizeof(_units.items[0]));
		removedItemsCount += count;
	}
	else if (i < _unitsCount2) {
		uint32_t count = _unitsCount2 - i;
		memcpy(&_prevUnits.items[i], &_units.items[i], count * sizeof(_units.items[0]));
	}
	_prevUnitsCount = _unitsCount2;
	_unitsCount2 = 0;
}