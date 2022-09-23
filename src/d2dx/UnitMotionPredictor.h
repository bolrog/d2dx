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
	class UnitMotionPredictor final
	{
	public:
		UnitMotionPredictor(
			_In_ const std::shared_ptr<IGameHelper>& gameHelper);

		void Update(
			_In_ IRenderContext* renderContext);

		Offset GetOffset(
			_In_ const D2::UnitAny* unit);

		void SetUnitScreenPos(
			_In_ const D2::UnitAny* unit,
			_In_ int32_t x,
			_In_ int32_t y);

		Offset GetOffsetForShadow(
			_In_ int32_t x,
			_In_ int32_t y);

		void AddUnit(
			D2::UnitAny* unit);

		void OnBufferClear();

	private:
		struct UnitIdAndType final
		{
			uint32_t unitType = 0;
			uint32_t unitId = 0;
		};

		struct UnitMotion final
		{
			Offset GetOffset() const;

			uint32_t lastUsedFrame = 0;
			Offset lastPos = { 0, 0 };
			Offset velocity = { 0, 0 };
			Offset predictedPos = { 0, 0 };
			Offset correctedPos = { 0, 0 };
			int64_t dtLastPosChange = 0;
		};

		std::shared_ptr<IGameHelper> _gameHelper;
		uint32_t _frame = 0;
		Buffer<UnitIdAndType> _unitIdAndTypes;
		Buffer<UnitMotion> _unitMotions;
		Buffer<Offset> _unitScreenPositions;
		int32_t _unitsCount = 0;

		Buffer<D2::UnitAny*> _units;
		Buffer<D2::UnitAny*> _prevUnits;
		uint32_t _unitsCount2 = 0;
		uint32_t _prevUnitsCount;
	};
}
