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

#include "Types.h"

namespace d2dx
{
	class Batch;
	class Vertex;

	class SurfaceIdTracker final
	{
	public:
		void OnNewFrame();

		void UpdateBatchSurfaceId(
			_Inout_ Batch& batch,
			_In_ MajorGameState majorGameState,
			_In_ Size gameSize,
			_Inout_updates_all_(batchVerticesCount) Vertex* batchVertices,
			_In_ int32_t batchVerticesCount);

	private:
		int32_t _nextSurfaceId = 0;
		int32_t _previousSurfaceId = -1;
		Rect _previousDrawCallRect = { 0,0,0,0 };
		uint64_t _previousDrawCallTexture = 0;
	};
}
