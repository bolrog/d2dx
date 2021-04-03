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
#include "Simd.h"

using namespace d2dx;

TextureCachePolicyBitPmru::TextureCachePolicyBitPmru(uint32_t capacity, std::shared_ptr<Simd> simd) :
	_capacity(capacity),
	_contentKeys(capacity),
	_usedInFrameBits(capacity >> 5),
	_mruBits(capacity >> 5),
	_simd(simd)
{
	assert(!(capacity & 63));
	memset(_contentKeys.items, 0, sizeof(uint32_t) * _contentKeys.capacity);
	memset(_usedInFrameBits.items, 0, sizeof(uint32_t) * _usedInFrameBits.capacity);
	memset(_mruBits.items, 0, sizeof(uint32_t) * _mruBits.capacity);
}

TextureCachePolicyBitPmru::~TextureCachePolicyBitPmru()
{
}

int32_t TextureCachePolicyBitPmru::Find(uint32_t contentKey, int32_t lastIndex)
{
	assert(contentKey != 0);

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

int32_t TextureCachePolicyBitPmru::Insert(uint32_t contentKey, bool& evicted)
{
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
		ALWAYS_PRINT("All texture atlas entries used in a single frame, starting over!");
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

	_contentKeys.items[replacementIndex] = contentKey;

	return replacementIndex;
}

void TextureCachePolicyBitPmru::OnNewFrame()
{
	memset(_usedInFrameBits.items, 0, sizeof(uint32_t) * _usedInFrameBits.capacity);
}