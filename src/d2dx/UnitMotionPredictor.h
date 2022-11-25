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

		void PrepareForNextFrame(
			_In_ int32_t timeToNext);

		Offset GetOffset(
			_In_ const D2::UnitAny* unit,
			_In_ Offset screenPos,
			_In_ bool isPlayer);

		void StartShadow(
			_In_ Offset screenPos,
			_In_ std::size_t vertexStart);
		
		void AddShadowVerticies(
			_In_ std::size_t vertexEnd);
		
		void UpdateShadowVerticies(
			_Inout_ Vertex *vertices);

		Offset GetShadowOffset(
			_In_ Offset screenPos);

	private:
		struct Unit final {
			Unit(D2::UnitAny const* unit, UnitInfo const &unitInfo, Offset screenPos) :
				unit(unit),
				id(unitInfo.id),
				type(unitInfo.type),
				actualPos(unitInfo.pos),
				basePos(unitInfo.pos),
				predictedPos(unitInfo.pos),
				lastRenderedPos(unitInfo.pos),
				lastRenderedScreenOffset({0, 0}),
				screenPos(screenPos),
				nextIdx(-1)
			{}

			D2::UnitAny const* unit;
			uint32_t id;
			D2::UnitType type;
			Offset actualPos;
			Offset basePos;
			Offset predictedPos;
			Offset lastRenderedPos;
			Offset lastRenderedScreenOffset;
			Offset screenPos;
			std::size_t nextIdx;
		};

		struct Shadow final {
			explicit Shadow(
				Offset screenPos,
				std::size_t vertexStart
			) : 
				screenPos(screenPos),
				vertexStart(vertexStart),
				vertexEnd(vertexStart)
			{}

			Offset screenPos;
			std::size_t vertexStart;
			std::size_t vertexEnd;
		};

		std::shared_ptr<IGameHelper> _gameHelper;
		std::vector<Unit> _units;
		std::vector<Unit> _prevUnits;
		std::vector<Shadow> _shadows;
		int32_t _sinceLastUpdate = 0;
		int32_t _frameTimeAdjustment = 0;
		bool _update = false;
	};
}
