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
#include "TextureHasher.h"
#include "Types.h"
#include "Utils.h"

using namespace d2dx;

TextureHasher::TextureHasher() :
	_cache{ D2DX_TMU_MEMORY_SIZE / 256, true },
	_cacheHits{ 0 },
	_cacheMisses{ 0 }
{
}

_Use_decl_annotations_
void TextureHasher::Invalidate(
	uint32_t startAddress)
{
	_cache.items[startAddress >> 8] = 0;
}

_Use_decl_annotations_
XXH64_hash_t TextureHasher::GetHash(
	uint32_t startAddress,
	const uint8_t* pixels,
	uint32_t pixelsSize,
	uint32_t largeLog2,
	uint32_t ratioLog2)
{
	assert((startAddress & 255) == 0);

	XXH64_hash_t hash = _cache.items[startAddress >> 8];

	if (hash)
	{
		++_cacheHits;
	}
	else
	{
		++_cacheMisses;
		hash = XXH3_64bits((void *)pixels, pixelsSize);
		hash ^= static_cast<XXH64_hash_t>(largeLog2 * 0x01000193u);
		hash ^= static_cast<XXH64_hash_t>(ratioLog2 * 0x01000193u) << 32;
		_cache.items[startAddress >> 8] = hash;
	}

	return hash;
}

void TextureHasher::PrintStats()
{
	D2DX_DEBUG_LOG("Texture hash cache hits: %u (%i%%) misses %u",
		_cacheHits,
		(int32_t)(100.0f * (float)_cacheHits / (_cacheHits + _cacheMisses)),
		_cacheMisses
	);
}
