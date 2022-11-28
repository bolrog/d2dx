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
#include "Vertex.h"

using namespace d2dx;
using namespace DirectX;

const std::int32_t D2_FRAME_LENGTH = (1 << 16) / 25;
const double FLOAT_TO_FIXED_MUL = static_cast<float>(1 << 16);
const double FIXED_TO_FLOAT_MUL = 1.f / FLOAT_TO_FIXED_MUL;

double fixedToDouble(int32_t x) {
	return static_cast<double>(x) * FIXED_TO_FLOAT_MUL;
}

OffsetT<double> fixedToDouble(Offset x) {
	return OffsetT<double>(
		static_cast<double>(x.x) * FIXED_TO_FLOAT_MUL,
		static_cast<double>(x.y) * FIXED_TO_FLOAT_MUL
	);
}

Offset doubleToFixed(OffsetT<double> x) {
	return Offset(
		static_cast<int32_t>(x.x * FLOAT_TO_FIXED_MUL),
		static_cast<int32_t>(x.y * FLOAT_TO_FIXED_MUL)
	);
}

_Use_decl_annotations_
UnitMotionPredictor::UnitMotionPredictor(
	const std::shared_ptr<IGameHelper>& gameHelper) :
	_gameHelper{ gameHelper }
{
	_units.reserve(1024);
	_prevUnits.reserve(1024);
	_shadows.reserve(1024);
}

_Use_decl_annotations_
Offset UnitMotionPredictor::GetOffset(
	D2::UnitAny const* unit,
	Offset screenPos,
	bool isPlayer)
{
	auto info = _gameHelper->GetUnitInfo(unit);
	auto prev = std::lower_bound(_prevUnits.begin(), _prevUnits.end(), unit, [&](auto const& x, auto const& y) { return x.unit < y; });
	if (prev == _prevUnits.end() || prev->id != info.id || prev->type != info.type) {
		if (!_update) {
			D2DX_LOG_PROFILE("MotionPredictor: Unexpected frame update");
			_update = true;
			if (_sinceLastUpdate - _currentUpdateTime < D2_FRAME_LENGTH / 2) {
				_sinceLastUpdate = _currentUpdateTime - _currentUpdateTime / 4;
				_frameTimeAdjustment = 0;
			}
			else {
				_frameTimeAdjustment = D2_FRAME_LENGTH - _sinceLastUpdate + _currentUpdateTime;
				_sinceLastUpdate = 0;
			}

			for (auto& pred : _units) {
				pred.predictedPos = pred.actualPos;
				pred.basePos = pred.lastRenderedPos;
			}
		}
		_units.push_back(Unit(unit, info, screenPos));
		return { 0, 0 };
	}
	if (prev->nextIdx != -1) {
		return _units[prev->nextIdx].lastRenderedScreenOffset;
	}

	auto prevUnit = *prev;
	prev->nextIdx = _units.size();
	prevUnit.screenPos = screenPos;
	if (!_update && prevUnit.actualPos != info.pos) {
		D2DX_LOG_PROFILE("MotionPredictor: Unexpected frame update");
		_update = true;
		if (_sinceLastUpdate - _currentUpdateTime < D2_FRAME_LENGTH / 2) {
			_sinceLastUpdate = _currentUpdateTime - _currentUpdateTime / 4;
			_frameTimeAdjustment = 0;
		}
		else {
			_frameTimeAdjustment = D2_FRAME_LENGTH - _sinceLastUpdate + _currentUpdateTime;
			_sinceLastUpdate = 0;
		}

		for (auto& pred : _units) {
			pred.predictedPos = pred.actualPos;
			pred.basePos = pred.lastRenderedPos;
		}
	}

	if (prevUnit.actualPos == info.pos) {
		if (_update) {
			if (isPlayer) {
				prevUnit.predictedPos = prevUnit.lastRenderedPos;
				prevUnit.basePos = prevUnit.lastRenderedPos;
			}
			else {
				prevUnit.predictedPos = prevUnit.actualPos;
				prevUnit.basePos = prevUnit.lastRenderedPos;
			}
		}
	}
	else {
		auto predOffset = info.pos - prevUnit.actualPos;
		prevUnit.actualPos = info.pos;
		if (std::abs(predOffset.x) >= (2 << 16) || std::abs(predOffset.y) >= (2 << 16)) {
			prevUnit.basePos = info.pos;
			prevUnit.predictedPos = info.pos;
		}
		else {
			prevUnit.basePos = prevUnit.lastRenderedPos;
			prevUnit.predictedPos = info.pos + predOffset;
			if (isPlayer) {
				D2DX_LOG_PROFILE(
					"MotionPredictor: Update player velocity %.4f/frame",
					fixedToDouble(prevUnit.predictedPos - prevUnit.lastRenderedPos).RealLength()
				);
			}
		}
	}

	OffsetT<double> renderPos;
	if (prevUnit.predictedPos == prevUnit.lastRenderedPos) {
		renderPos = fixedToDouble(prevUnit.predictedPos);
	}
	else {
		double predFractFromBase = fixedToDouble(_sinceLastUpdate + _frameTimeAdjustment)
			/ fixedToDouble(D2_FRAME_LENGTH + _frameTimeAdjustment);
		double predFractFromActual = fixedToDouble(_sinceLastUpdate) / fixedToDouble(D2_FRAME_LENGTH);

		auto offsetFromBase = fixedToDouble(prevUnit.predictedPos - prevUnit.basePos) * predFractFromBase;
		auto offsetFromActual = fixedToDouble(prevUnit.predictedPos - prevUnit.actualPos) * predFractFromActual;
		auto renderPosFromBase = fixedToDouble(prevUnit.basePos) + offsetFromBase;
		auto renderPosFromActual = fixedToDouble(prevUnit.actualPos) + offsetFromActual;
		renderPos = renderPosFromBase * (1.f - predFractFromBase) + renderPosFromActual * predFractFromBase;
	}

	if (isPlayer) {
		D2DX_LOG_PROFILE(
			"MotionPredictor: Move player by %f",
			(fixedToDouble(prevUnit.lastRenderedPos) - renderPos).RealLength()
		);
	}

	auto offset = renderPos - fixedToDouble(prevUnit.actualPos);
	prevUnit.lastRenderedPos = doubleToFixed(renderPos);

	const OffsetT<double> scaleFactors{ 32.0f / std::sqrt(2.0), 16.0f / std::sqrt(2.0) };
	auto screenOffset = scaleFactors * OffsetT<double>{ offset.x - offset.y, offset.x + offset.y } + 0.5;
	prevUnit.lastRenderedScreenOffset = { (int32_t)screenOffset.x, (int32_t)screenOffset.y };
	
	_units.push_back(prevUnit);
	return prevUnit.lastRenderedScreenOffset;
}

_Use_decl_annotations_
Offset UnitMotionPredictor::GetShadowOffset(
	Offset screenPos)
{
	for (auto &unit: _prevUnits) {
		if (std::abs(unit.screenPos.x - screenPos.x) < 10 && std::abs(unit.screenPos.y - screenPos.y) < 10) {
			return unit.lastRenderedScreenOffset;
		}
	}
	return { 0, 0 };
}


_Use_decl_annotations_
void UnitMotionPredictor::StartShadow(
	Offset screenPos,
	std::size_t vertexStart)
{
	_shadows.push_back(Shadow(screenPos, vertexStart));
}

_Use_decl_annotations_
void UnitMotionPredictor::AddShadowVerticies(
	std::size_t vertexEnd)
{
	_shadows.back().vertexEnd = vertexEnd;
}

_Use_decl_annotations_
void UnitMotionPredictor::UpdateShadowVerticies(
	Vertex *vertices)
{
	for (auto& shadow : _shadows) {
		for (auto unit = _units.rbegin(), end = _units.rend(); unit != end; ++unit) {
			if (std::abs(unit->screenPos.x - shadow.screenPos.x) < 10 && std::abs(unit->screenPos.y - shadow.screenPos.y) < 10) {
				for (auto i = shadow.vertexStart; i != shadow.vertexEnd; ++i) {
					vertices[i].AddOffset(unit->lastRenderedScreenOffset.x, unit->lastRenderedScreenOffset.y);
				}
			}
		}
	}
}

_Use_decl_annotations_
void UnitMotionPredictor::PrepareForNextFrame(
	_In_ uint32_t prevProjectedTime,
	_In_ uint32_t prevActualTime,
	_In_ uint32_t projectedTime)
{
	_sinceLastUpdate -= prevProjectedTime + 10;
	_sinceLastUpdate += prevActualTime;

	std::swap(_units, _prevUnits);
	std::stable_sort(_prevUnits.begin(), _prevUnits.end(), [&](auto& x, auto& y) { return x.unit < y.unit; });
	_units.clear();
	_shadows.clear();
	_update = false;

	auto timeBeforeUpdate = D2_FRAME_LENGTH - _sinceLastUpdate;
	_currentUpdateTime = projectedTime + 10;
	_sinceLastUpdate += projectedTime + 10;
	if (_sinceLastUpdate >= D2_FRAME_LENGTH) {
		D2DX_LOG_PROFILE("MotionPredictor: Expect frame update");
		_update = true;
		_sinceLastUpdate -= D2_FRAME_LENGTH;
		_frameTimeAdjustment = timeBeforeUpdate;
		if (_sinceLastUpdate >= D2_FRAME_LENGTH) {
			_prevUnits.clear();
			_sinceLastUpdate = 0;
			_frameTimeAdjustment = 0;
			_currentUpdateTime = 0;
		}
	}
	else if (_sinceLastUpdate < -D2_FRAME_LENGTH) {
		_prevUnits.clear();
		_sinceLastUpdate = 0;
		_frameTimeAdjustment = 0;
		_currentUpdateTime = 0;
	}

	D2DX_LOG_PROFILE(
		"MotionPredictor: Predict at %d/%d of a frame",
		_sinceLastUpdate + _frameTimeAdjustment,
		D2_FRAME_LENGTH + _frameTimeAdjustment
	);
}