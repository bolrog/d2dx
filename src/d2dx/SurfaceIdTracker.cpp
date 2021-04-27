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
#include "SurfaceIdTracker.h"
#include "Batch.h"
#include "Vertex.h"

using namespace d2dx;

void SurfaceIdTracker::OnNewFrame()
{
	_nextSurfaceId = 0;
	_previousDrawCallRect = { 0,0,0,0 };
	_previousDrawCallTexture = 0;
	_previousSurfaceId = -1;
}

_Use_decl_annotations_
void SurfaceIdTracker::UpdateBatchSurfaceId(
	Batch& batch,
	MajorGameState majorGameState,
	Size gameSize,
	Vertex* batchVertices,
	int32_t batchVerticesCount)
{
	uint32_t surfaceId = 0;

	if (_previousSurfaceId == D2DX_SURFACE_ID_USER_INTERFACE)
	{
		/* Once the UI has started being drawn, assume the following draw calls are UI too. */
		surfaceId = D2DX_SURFACE_ID_USER_INTERFACE;
	}
	else if (majorGameState != MajorGameState::InGame)
	{
		surfaceId = D2DX_SURFACE_ID_USER_INTERFACE;
	}
	else if (!batch.IsChromaKeyEnabled())
	{
		/* Text background rectangle. */
		surfaceId = D2DX_SURFACE_ID_USER_INTERFACE;
	}
	else
	{
		uint64_t drawCallTexture = (uint64_t)batch.GetTextureIndex() | ((uint64_t)batch.GetTextureAtlas() << 32ULL);

		int32_t minx = INT_MAX;
		int32_t miny = INT_MAX;
		int32_t maxx = INT_MIN;
		int32_t maxy = INT_MIN;

		for (uint32_t i = 0; i < batch.GetVertexCount(); ++i)
		{
			int32_t x = (int32_t)batchVertices[i].GetX();
			int32_t y = (int32_t)batchVertices[i].GetY();
			minx = min(minx, x);
			miny = min(miny, y);
			maxx = max(maxx, x);
			maxy = max(maxy, y);
		}

		switch (batch.GetTextureCategory())
		{
		case TextureCategory::MousePointer:
		case TextureCategory::UserInterface:
		case TextureCategory::Font:
			surfaceId = D2DX_SURFACE_ID_USER_INTERFACE;
			break;
		default:
		case TextureCategory::Floor:
		case TextureCategory::Wall:
		case TextureCategory::Unknown:
		{
			bool isNewSurface = true;

			if (_previousDrawCallTexture == drawCallTexture)
			{
				isNewSurface = false;
			}
			else if (batch.GetTextureWidth() == 32 && batch.GetTextureHeight() == 32)
			{
				if (/* 32x32 wall block drawn to the left of the previous one. */
					(maxx == _previousDrawCallRect.offset.x && miny == _previousDrawCallRect.offset.y) ||

					/* 32x32 wall block drawn on the next row from the previous one. */
					(miny == (_previousDrawCallRect.offset.y + _previousDrawCallRect.size.height)))
				{
					isNewSurface = false;
				}
			}

			if (isNewSurface)
			{
				surfaceId = ++_nextSurfaceId;
			}
			else
			{
				surfaceId = _nextSurfaceId;
			}

			break;
		}
		}

		_previousDrawCallTexture = drawCallTexture;
		_previousDrawCallRect.offset.x = minx;
		_previousDrawCallRect.offset.y = miny;
		_previousDrawCallRect.size.width = maxx - minx;
		_previousDrawCallRect.size.height = maxy - miny;
	}

	_previousSurfaceId = surfaceId;

	for (uint32_t i = 0; i < batch.GetVertexCount(); ++i)
	{
		batchVertices[i].SetSurfaceId(surfaceId);
	}
}
