/*
	This file is part of D2DX.

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
#include "TextureCachePolicy.h"

namespace d2dx
{
	class Simd;

	class TextureCachePolicyBitPmru final : public TextureCachePolicy
	{
	public:
		TextureCachePolicyBitPmru(uint32_t capacity, std::shared_ptr<Simd> simd);
		virtual ~TextureCachePolicyBitPmru();

		virtual int32_t Find(uint32_t contentKey, int32_t lastIndex) override;
		virtual int32_t Insert(uint32_t contentKey, bool& evicted) override;
		virtual void OnNewFrame() override;

	private:
		uint32_t _capacity;
		std::shared_ptr<Simd> _simd;
		Buffer<uint32_t> _contentKeys;
		Buffer<uint32_t> _usedInFrameBits;
		Buffer<uint32_t> _mruBits;
	};
}
