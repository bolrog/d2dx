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
	class MotionPredictor final
	{
	public:
		MotionPredictor(
			_In_ const std::shared_ptr<IGameHelper>& gameHelper);

		void PrepareForNextFrame(
			_In_ uint32_t prevProjectedTime,
			_In_ uint32_t prevActualTime,
			_In_ uint32_t projectedTime);

		OffsetF GetUnitOffset(
			_In_ const D2::UnitAny* unit,
			_In_ Offset screenPos,
			_In_ bool isPlayer);

		void StartUnitShadow(
			_In_ Offset screenPos,
			_In_ std::size_t vertexStart);
		
		void AddUnitShadowVerticies(
			_In_ std::size_t vertexEnd);
		
		void UpdateUnitShadowVerticies(
			_Inout_ Vertex *vertices);

		OffsetF GetTextOffset(
			_In_ uint64_t id,
			_In_ Offset pos);

		void UpdateGameSize(
			_In_ Size gameSize)
		{
			_halfGameWidth = gameSize.width / 2;
		}

	private:
		void OnUnexpectedUpdate(char const* cause) noexcept;

		struct Unit final {
			Unit(D2::UnitAny const* unit, UnitInfo const &unitInfo, Offset screenPos) :
				unit(unit),
				id(unitInfo.id),
				type(unitInfo.type),
				actualPos(unitInfo.pos),
				baseOffset(0, 0),
				predictionOffset(0, 0),
				lastRenderedPos(unitInfo.pos),
				lastRenderedScreenOffset({0.f, 0.f}),
				screenPos(screenPos),
				nextIdx(-1)
			{}

			D2::UnitAny const* unit;
			uint32_t id;
			D2::UnitType type;
			Offset actualPos;
			Offset baseOffset;
			Offset predictionOffset;
			Offset lastRenderedPos;
			OffsetF lastRenderedScreenOffset;
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

		struct Text final {
			Text(uint64_t id, Offset pos) :
				id(id),
				actualPos(pos),
				baseOffset({ 0.f, 0.f }),
				predictionOffset({ 0.f, 0.f }),
				lastRenderedPos(OffsetF(pos)),
				nextIdx(-1)
			{}

			uint64_t id;
			Offset actualPos;
			OffsetF baseOffset;
			OffsetF predictionOffset;
			OffsetF lastRenderedPos;
			std::size_t nextIdx;
		};

		std::shared_ptr<IGameHelper> _gameHelper;
		std::vector<Unit> _units;
		std::vector<Unit> _prevUnits;
		std::vector<Shadow> _shadows;
		std::vector<Text> _texts;
		std::vector<Text> _prevTexts;
		int32_t _currentFrameTime = 0;
		int32_t _sinceLastUpdate = 0;
		int32_t _fromPrevFrame = 0;
		int32_t _halfGameWidth = 0;
		bool _update = false;
	};
}
