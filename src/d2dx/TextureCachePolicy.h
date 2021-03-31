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
	class TextureCachePolicy abstract
	{
	public:
		virtual ~TextureCachePolicy() {}

		virtual int32_t Find(uint32_t contentKey, int32_t lastIndex) = 0;
		virtual int32_t Insert(uint32_t contentKey, bool& evicted) = 0;
		virtual void OnNewFrame() = 0;
	};
}
