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

#include "ITextureCache.h"
#include "Types.h"
#include "Options.h"

namespace d2dx
{
	class Vertex;
	class Batch;

	struct IRenderContext abstract
	{
		virtual ~IRenderContext() noexcept {}

		virtual HWND GetHWnd() const = 0;

		virtual void LoadGammaTable(
			_In_reads_(valueCount) const uint32_t* values,
			_In_ uint32_t valueCount) = 0;

		virtual uint32_t BulkWriteVertices(
			_In_reads_(vertexCount) const Vertex* vertices,
			_In_ uint32_t vertexCount) = 0;

		virtual TextureCacheLocation UpdateTexture(
			_In_ const Batch& batch,
			_In_reads_(tmuDataSize) const uint8_t* tmuData,
			_In_ uint32_t tmuDataSize) = 0;

		virtual void Draw(
			_In_ const Batch& batch,
			_In_ uint32_t startVertexLocation) = 0;

		virtual void Present() = 0;

		virtual void WriteToScreen(
			_In_reads_(width * height) const uint32_t* pixels,
			_In_ int32_t width,
			_In_ int32_t height,
			_In_ bool forCinematic) = 0;

		virtual void SetPalette(
			_In_ int32_t paletteIndex,
			_In_reads_(256) const uint32_t* palette) = 0;

		virtual const Options& GetOptions() const = 0;

		virtual ITextureCache* GetTextureCache(
			_In_ const Batch& batch) const = 0;

		virtual void SetSizes(
			_In_ Size gameSize,
			_In_ Size windowSize,
			_In_ ScreenMode screenMode) = 0;

		virtual void GetCurrentMetrics(
			_Out_opt_ Size* gameSize,
			_Out_opt_ Rect* renderRect,
			_Out_opt_ Size* desktopSize) const = 0;

		virtual void ToggleFullscreen() = 0;

		virtual float GetFrameTime() const = 0;

		virtual int32_t GetFrameTimeFp() const = 0;

		virtual ScreenMode GetScreenMode() const = 0;
	};
}
