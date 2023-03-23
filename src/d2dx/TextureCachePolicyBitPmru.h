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
#include "ISimd.h"

namespace d2dx
{
	class TextureCachePolicyBitPmru final
	{
	public:
		TextureCachePolicyBitPmru() = default;
		TextureCachePolicyBitPmru& operator=(TextureCachePolicyBitPmru&& rhs) = default;

		TextureCachePolicyBitPmru(
			_In_ uint32_t capacity,
			_In_ const std::shared_ptr<ISimd>& simd);
		~TextureCachePolicyBitPmru() noexcept {}

		int32_t Find(
			_In_ uint64_t contentKey,
			_In_ int32_t lastIndex);
		
		int32_t Insert(
			_In_ uint64_t contentKey,
			_Out_ bool& evicted);
		
		void OnNewFrame();

		uint32_t GetUsedCount() const;

	private:
		uint32_t _capacity = 0;
		std::shared_ptr<ISimd> _simd;
		Buffer<uint64_t> _contentKeys;
		Buffer<uint32_t> _usedInFrameBits;
		Buffer<uint32_t> _mruBits;
		uint32_t _usedCount = 0;
	};
}
