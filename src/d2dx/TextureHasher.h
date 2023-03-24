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

#include "Buffer.h"

namespace d2dx
{
	class TextureHasher final
	{
	public:
		TextureHasher();
		~TextureHasher() noexcept {}

		void Invalidate(
			_In_ uint32_t startAddress);

		XXH64_hash_t GetHash(
			_In_ uint32_t startAddress,
			_In_reads_(pixelsSize) const uint8_t* pixels,
			_In_ uint32_t pixelsSize,
			_In_ uint32_t largeLog2,
			_In_ uint32_t ratioLog2);

		void PrintStats();

	private:
		Buffer<XXH64_hash_t> _cache;
        uint32_t _cacheHits;
		uint32_t _cacheMisses;
	};
}
