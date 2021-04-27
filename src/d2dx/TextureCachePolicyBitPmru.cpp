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
#include "Utils.h"
#include "TextureCachePolicyBitPmru.h"
#include "ISimd.h"

using namespace d2dx;

_Use_decl_annotations_
TextureCachePolicyBitPmru::TextureCachePolicyBitPmru(
	uint32_t capacity,
	const std::shared_ptr<ISimd>& simd) :
	_capacity{ capacity },
	_contentKeys{ capacity, true },
	_usedInFrameBits{ capacity >> 5, true },
	_mruBits{ capacity >> 5, true },
	_simd{ simd }
{
	assert(!(capacity & 63));
	assert(simd);
}

_Use_decl_annotations_
int32_t TextureCachePolicyBitPmru::Find(
	uint32_t contentKey,
	int32_t lastIndex)
{
	assert(contentKey != 0);

	if (_capacity == 0)
	{
		return -1;
	}

	if (lastIndex >= 0 && lastIndex < (int32_t)_capacity &&
		contentKey == _contentKeys.items[lastIndex])
	{
		_usedInFrameBits.items[lastIndex >> 5] |= 1 << (lastIndex & 31);
		_mruBits.items[lastIndex >> 5] |= 1 << (lastIndex & 31);
		return lastIndex;
	}

	int32_t findIndex = _simd->IndexOfUInt32(_contentKeys.items, _capacity, contentKey);

	if (findIndex >= 0)
	{
		_usedInFrameBits.items[findIndex >> 5] |= 1 << (findIndex & 31);
		_mruBits.items[findIndex >> 5] |= 1 << (findIndex & 31);
		return findIndex;
	}

	return -1;
}

_Use_decl_annotations_
int32_t TextureCachePolicyBitPmru::Insert(
	uint32_t contentKey,
	bool& evicted)
{
	if (_capacity == 0)
	{
		evicted = false;
		return -1;
	}

	int32_t replacementIndex = -1;

	for (uint32_t i = 0; i < _mruBits.capacity; ++i)
	{
		DWORD ri;
		if (BitScanForward(&ri, (DWORD)~_mruBits.items[i]))
		{
			replacementIndex = (int32_t)(i * 32 + ri);
			break;
		}
	}

	if (replacementIndex < 0)
	{
		memcpy(_mruBits.items, _usedInFrameBits.items, sizeof(uint32_t) * _mruBits.capacity);

		for (uint32_t i = 0; i < _mruBits.capacity; ++i)
		{
			DWORD ri;
			if (BitScanForward(&ri, (DWORD)~_mruBits.items[i]))
			{
				replacementIndex = (int32_t)(i * 32 + ri);
				break;
			}
		}
	}

	if (replacementIndex < 0)
	{
		D2DX_LOG("All texture atlas entries used in a single frame, starting over!");
		memset(_mruBits.items, 0, sizeof(uint32_t) * _mruBits.capacity);
		memset(_usedInFrameBits.items, 0, sizeof(uint32_t) * _usedInFrameBits.capacity);

		for (uint32_t i = 0; i < _mruBits.capacity; ++i)
		{
			DWORD ri;
			if (BitScanForward(&ri, (DWORD)~_mruBits.items[i]))
			{
				replacementIndex = (int32_t)(i * 32 + ri);
				break;
			}
		}
	}

	_mruBits.items[replacementIndex >> 5] |= 1 << (replacementIndex & 31);
	_usedInFrameBits.items[replacementIndex >> 5] |= 1 << (replacementIndex & 31);

	evicted = _contentKeys.items[replacementIndex] != 0;

	if (!evicted)
	{
		++_usedCount;
	}

	_contentKeys.items[replacementIndex] = contentKey;

	return replacementIndex;
}

void TextureCachePolicyBitPmru::OnNewFrame()
{
	memset(_usedInFrameBits.items, 0, sizeof(uint32_t) * _usedInFrameBits.capacity);
}

uint32_t TextureCachePolicyBitPmru::GetUsedCount() const
{
	return _usedCount;
}
