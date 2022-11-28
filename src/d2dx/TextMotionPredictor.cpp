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
#include "TextMotionPredictor.h"

using namespace d2dx;
using namespace DirectX;

_Use_decl_annotations_
TextMotionPredictor::TextMotionPredictor(
	const std::shared_ptr<IGameHelper>& gameHelper) :
	_gameHelper{ gameHelper },
	_textMotions{ 128, true },
	_textsCount{ 0 },
	_frame{ 0 }
{
}

_Use_decl_annotations_
void TextMotionPredictor::Update(
	IRenderContext* renderContext)
{
	renderContext->GetCurrentMetrics(&_gameSize, nullptr, nullptr);

	const float dt = renderContext->GetProjectedFrameTime();
	int32_t expiredTextIndex = -1;

	for (int32_t i = 0; i < _textsCount; ++i)
	{
		TextMotion& tm = _textMotions.items[i];

		if (!tm.id)
		{
			expiredTextIndex = i;
			continue;
		}

		if (abs((int64_t)_frame - (int64_t)tm.lastUsedFrame) > 2)
		{
			tm.id = 0;
			expiredTextIndex = i;
			continue;
		}

		OffsetF targetVec = tm.targetPos - tm.currentPos;
		float targetDistance = targetVec.Length();
		targetVec.Normalize();

		float step = min(dt * 30.0f * targetDistance, targetDistance);

		auto moveVec = targetVec * step;

		tm.currentPos += moveVec;
	}

	// Gradually (one change per frame) compact the list.
	if (_textsCount > 0)
	{
		if (!_textMotions.items[_textsCount - 1].id)
		{
			// The last entry is expired. Shrink the list.
			_textMotions.items[_textsCount - 1] = { };
			--_textsCount;
		}
		else if (expiredTextIndex >= 0 && expiredTextIndex < (_textsCount - 1))
		{
			// Some entry is expired. Move the last entry to that place, and shrink the list.
			_textMotions.items[expiredTextIndex] = _textMotions.items[_textsCount - 1];
			_textMotions.items[_textsCount - 1] = { };
			--_textsCount;
		}
	}

	++_frame;
}

_Use_decl_annotations_
Offset TextMotionPredictor::GetOffset(
	uint64_t textId,
	Offset posFromGame)
{
	OffsetF posFromGameF{ (float)posFromGame.x, (float)posFromGame.y };
	int32_t textIndex = -1;

	for (int32_t i = 0; i < _textsCount; ++i)
	{
		if (_textMotions.items[i].id == textId)
		{
			bool resetCurrentPos = false;

			if ((_gameHelper->ScreenOpenMode() & 1) && posFromGameF.x >= _gameSize.width / 2)
			{
				resetCurrentPos = true;
			}
			else if ((_gameHelper->ScreenOpenMode() & 2) && posFromGameF.x <= _gameSize.width / 2)
			{
				resetCurrentPos = true;
			}
			else
			{
				auto distance = (posFromGameF - _textMotions.items[i].targetPos).Length();
				if (distance > 32.0f)
				{
					resetCurrentPos = true;
				}
			}

			_textMotions.items[i].targetPos = posFromGameF;
			if (resetCurrentPos)
			{
				_textMotions.items[i].currentPos = posFromGameF;
			}
			_textMotions.items[i].lastUsedFrame = _frame;
			textIndex = i;
			break;
		}
	}

	if (textIndex < 0)
	{
		if (_textsCount < (int32_t)_textMotions.capacity)
		{
			textIndex = _textsCount++;
			_textMotions.items[textIndex].id = textId;
			_textMotions.items[textIndex].targetPos = posFromGameF;
			_textMotions.items[textIndex].currentPos = posFromGameF;
			_textMotions.items[textIndex].lastUsedFrame = _frame;
		}
		else
		{
			D2DX_DEBUG_LOG("TMP: Too many texts.");
		}
	}

	if (textIndex < 0)
	{
		return { 0, 0 };
	}

	TextMotion& tm = _textMotions.items[textIndex];
	return { (int32_t)(tm.currentPos.x - posFromGame.x), (int32_t)(tm.currentPos.y - posFromGame.y) };
}
