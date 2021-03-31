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

namespace d2dx
{
	template<typename T>
	struct Buffer final
	{
		Buffer() noexcept :
			items(nullptr),
			capacity(0)
		{
		}

		Buffer(uint32_t capacity_) noexcept :
			items((T* __restrict)_aligned_malloc(sizeof(T)* capacity_, 256)),
			capacity(capacity_)
		{
			assert(items);
		}

		Buffer(const Buffer&) = delete;

		Buffer& operator=(const Buffer&) = delete;

		Buffer(Buffer&& rhs) noexcept : Buffer()
		{
			std::swap(items, rhs.items);
			std::swap(capacity, rhs.capacity);
		}

		Buffer& operator=(Buffer&& rhs) noexcept
		{
			std::swap(items, rhs.items);
			std::swap(capacity, rhs.capacity);
			return *this;
		}

		~Buffer() noexcept
		{
			_aligned_free(items);
		}

		T* __restrict items;
		uint32_t capacity;
	};
}
