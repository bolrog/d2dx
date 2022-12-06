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
#include "MotionPredictor.h"
#include "Vertex.h"

using namespace d2dx;
using namespace DirectX;

const std::int32_t D2_FRAME_LENGTH = (1 << 16) / 25;
const double FLOAT_TO_FIXED_MUL = static_cast<float>(1 << 16);
const double FIXED_TO_FLOAT_MUL = 1.f / FLOAT_TO_FIXED_MUL;
const OffsetT<double> GAME_TO_SCREEN_POS = { 32. / 1.41421356237, 16. / 1.41421356237 };

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
MotionPredictor::MotionPredictor(
	const std::shared_ptr<IGameHelper>& gameHelper) :
	_gameHelper{ gameHelper }
{
	_units.reserve(1024);
	_prevUnits.reserve(1024);
	_shadows.reserve(1024);
	_texts.reserve(128);
	_prevTexts.reserve(128);
}

void MotionPredictor::OnUnexpectedUpdate(char const *cause) noexcept
{
	_update = true;
	if (_sinceLastUpdate < D2_FRAME_LENGTH / 2) {
		D2DX_LOG_PROFILE(
			"MotionPredictor(%s): Unexpected frame update (draw back %d)",
			cause, (_sinceLastUpdate - 10) / 4);
		// In the first half of the current frame.
		// Dial back the time a bit as this is likely ahead of the game.
		_sinceLastUpdate -= (_sinceLastUpdate - 10) / 4;
		_frameTimeAdjustment = 0;
	}
	else {
		D2DX_LOG_PROFILE(
			"MotionPredictor(%s): Unexpected frame update (jump ahead %d)",
			cause, D2_FRAME_LENGTH - _sinceLastUpdate);
		// In the second half of the current frame.
		// Jump ahead to the next frame as this is likely behind the game.
		_frameTimeAdjustment = D2_FRAME_LENGTH - _sinceLastUpdate;
		_sinceLastUpdate = 0;
	}

	// Remove the prediction from all previous units.
	for (auto& pred : _units) {
		pred.baseOffset = pred.lastRenderedPos - pred.actualPos;
		pred.predictionOffset = -pred.baseOffset;
	}
	for (auto& text : _texts) {
		text.baseOffset = text.lastRenderedPos - text.actualPos;
		text.predictionOffset = { 0, 0 };
	}
}

_Use_decl_annotations_
Offset MotionPredictor::GetUnitOffset(
	D2::UnitAny const* unit,
	Offset screenPos,
	bool isPlayer)
{
	auto info = _gameHelper->GetUnitInfo(unit);
	auto prevIter = std::lower_bound(_prevUnits.begin(), _prevUnits.end(), unit, [&](auto const& x, auto const& y) { return x.unit < y; });
	if (prevIter == _prevUnits.end() || prevIter->id != info.id || prevIter->type != info.type) {
		// New unit found.
		if (!_update) {
			OnUnexpectedUpdate("Unit");
		}
		_units.push_back(Unit(unit, info, screenPos));
		return { 0, 0 };
	}
	if (prevIter->nextIdx != -1) {
		// Already processed this frame.
		return _units[prevIter->nextIdx].lastRenderedScreenOffset;
	}

	auto prev = *prevIter;
	prevIter->nextIdx = _units.size();
	prev.screenPos = screenPos;
	if (!_update && prev.actualPos != info.pos) {
		OnUnexpectedUpdate("Unit");
	}

	if (prev.actualPos == info.pos) {
		if (_update) {
			// Unit hasn't moved, snap the unit back to it's actual position.
			// Note the player isn't snapped back as the whole screen jerking
			// backwards is quite annoying.
			prev.baseOffset = prev.lastRenderedPos - prev.actualPos;
			prev.predictionOffset = isPlayer? Offset(0, 0) : -prev.baseOffset;
		}
	}
	else {
		auto predOffset = info.pos - prev.actualPos;
		prev.actualPos = info.pos;
		if (std::abs(predOffset.x) >= (2 << 16) || std::abs(predOffset.y) >= (2 << 16)) {
			// Assume the unit teleported, perform no motion prediction.
			prev.baseOffset = { 0, 0 };
			prev.predictionOffset = { 0, 0 };
		}
		else {
			prev.baseOffset = prev.lastRenderedPos - prev.actualPos;
			prev.predictionOffset = (info.pos + predOffset / 2) - prev.lastRenderedPos;
			if (isPlayer) {
				D2DX_LOG_PROFILE(
					"MotionPredictor: Update player velocity %.4f/frame",
					fixedToDouble(prev.predictionOffset).RealLength()
					* (fixedToDouble(D2_FRAME_LENGTH) / fixedToDouble(D2_FRAME_LENGTH + _frameTimeAdjustment))
				);
			}
		}
	}

	double predFract = fixedToDouble(_sinceLastUpdate + _frameTimeAdjustment)
		/ fixedToDouble(D2_FRAME_LENGTH + _frameTimeAdjustment);
	auto offset = fixedToDouble(prev.baseOffset) + fixedToDouble(prev.predictionOffset) * predFract;
	auto renderPos = prev.actualPos + doubleToFixed(offset);

	if (isPlayer) {
		D2DX_LOG_PROFILE(
			"MotionPredictor: Move player by %f",
			fixedToDouble(prev.lastRenderedPos - renderPos).RealLength()
		);
	}

	prev.lastRenderedPos = renderPos;
	auto screenOffset = GAME_TO_SCREEN_POS * OffsetT<double>{ offset.x - offset.y, offset.x + offset.y } + 0.5;
	prev.lastRenderedScreenOffset = { (int32_t)std::round(screenOffset.x), (int32_t)std::round(screenOffset.y) };
	
	_units.push_back(prev);
	return prev.lastRenderedScreenOffset;
}

_Use_decl_annotations_
Offset MotionPredictor::GetUnitShadowOffset(
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
void MotionPredictor::StartUnitShadow(
	Offset screenPos,
	std::size_t vertexStart)
{
	_shadows.push_back(Shadow(screenPos, vertexStart));
}

_Use_decl_annotations_
void MotionPredictor::AddUnitShadowVerticies(
	std::size_t vertexEnd)
{
	_shadows.back().vertexEnd = vertexEnd;
}

_Use_decl_annotations_
void MotionPredictor::UpdateUnitShadowVerticies(
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
Offset MotionPredictor::GetTextOffset(
	uint64_t id,
	Offset pos)
{
	auto screenOpenMode = _gameHelper->ScreenOpenMode();
	if (((screenOpenMode & 1) && pos.x >= _halfGameWidth)
		|| ((screenOpenMode & 2) && pos.x <= _halfGameWidth))
	{
		return { 0, 0 };
	}

	auto prevIter = std::lower_bound(_prevTexts.begin(), _prevTexts.end(), id, [&](auto const& x, auto const& y) { return x.id < y; });
	if (prevIter == _prevTexts.end()) {
		_texts.push_back(Text(id, pos));
		return { 0, 0 };
	}
	if (prevIter->nextIdx != -1) {
		// Already processed this frame.
		auto prev = _texts[prevIter->nextIdx];
		return prev.lastRenderedPos - prev.actualPos;
	}

	auto prev = *prevIter;
	prevIter->nextIdx = _texts.size();
	if (!_update && prev.actualPos != pos) {
		OnUnexpectedUpdate("Text");
	}

	if (prev.actualPos == pos) {
		if (_update) {
			// Text hasn't moved, stop moving the text as snapping to it's actual
			// position is hard to look at
			prev.baseOffset = prev.lastRenderedPos - prev.actualPos;
			prev.predictionOffset = Offset(0, 0);
		}
	}
	else {
		auto predOffset = pos - prev.actualPos;
		prev.actualPos = pos;
		if (std::abs(predOffset.x) >= 45 || std::abs(predOffset.y) >= 22) {
			// Either texts were rearranged, or the player teleported
			prev.baseOffset = { 0, 0 };
			prev.predictionOffset = { 0, 0 };
		}
		else {
			prev.baseOffset = prev.lastRenderedPos - prev.actualPos;
			prev.predictionOffset = (pos + predOffset / 2) - prev.lastRenderedPos;
		}
	}

	double predFract = static_cast<double>(_sinceLastUpdate + _frameTimeAdjustment)
		/ static_cast<double>(D2_FRAME_LENGTH + _frameTimeAdjustment);
	auto offset = prev.baseOffset + Offset((OffsetT<double>(prev.predictionOffset) * predFract).Round());
	prev.lastRenderedPos = prev.actualPos + offset;

	_texts.push_back(prev);
	return offset;
}

_Use_decl_annotations_
void MotionPredictor::PrepareForNextFrame(
	_In_ uint32_t prevProjectedTime,
	_In_ uint32_t prevActualTime,
	_In_ uint32_t projectedTime)
{
	_sinceLastUpdate -= prevProjectedTime + 10;
	_sinceLastUpdate += prevActualTime;

	std::swap(_units, _prevUnits);
	std::stable_sort(_prevUnits.begin(), _prevUnits.end(), [&](auto& x, auto& y) { return x.unit < y.unit; });
	std::swap(_texts, _prevTexts);
	std::stable_sort(_prevTexts.begin(), _prevTexts.end(), [&](auto& x, auto& y) { return x.id < y.id; });
	_units.clear();
	_shadows.clear();
	_texts.clear();
	_update = false;

	auto timeBeforeUpdate = D2_FRAME_LENGTH - _sinceLastUpdate;
	_sinceLastUpdate += projectedTime + 10;
	if (_sinceLastUpdate >= D2_FRAME_LENGTH) {
		D2DX_LOG_PROFILE("MotionPredictor: Expecting frame update");
		_update = true;
		_sinceLastUpdate -= D2_FRAME_LENGTH;
		_frameTimeAdjustment = timeBeforeUpdate;
		if (_sinceLastUpdate >= D2_FRAME_LENGTH) {
			_prevUnits.clear();
			_prevTexts.clear();
			_sinceLastUpdate = 0;
			_frameTimeAdjustment = 0;
		}
	}
	else if (_sinceLastUpdate < -D2_FRAME_LENGTH) {
		_update = true;
		_prevUnits.clear();
		_prevTexts.clear();
		_sinceLastUpdate = 0;
		_frameTimeAdjustment = 0;
	}

	D2DX_LOG_PROFILE(
		"MotionPredictor: Predict at %d/%d of a frame",
		_sinceLastUpdate + _frameTimeAdjustment,
		D2_FRAME_LENGTH + _frameTimeAdjustment
	);
}