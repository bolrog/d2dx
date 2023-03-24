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

#include "ISimd.h"
#include "Types.h"

namespace d2dx
{
	class Batch;

	struct TextureCacheLocation final
	{
		int16_t _textureAtlas;
		int16_t _textureIndex;
	};

	static_assert(sizeof(TextureCacheLocation) == 4, "sizeof(TextureCacheLocation) == 4");

	struct ITextureCache abstract
	{
		virtual ~ITextureCache() noexcept {}

		virtual void OnNewFrame() = 0;

		virtual TextureCacheLocation FindTexture(
			_In_ uint64_t contentKey,
			_In_ int32_t lastIndex) = 0;

		virtual TextureCacheLocation InsertTexture(
			_In_ uint64_t contentKey,
			_In_ const Batch& batch,
			_In_reads_(tmuDataSize) const uint8_t* tmuData,
			_In_ uint32_t tmuDataSize) = 0;

		virtual ID3D11ShaderResourceView* GetSrv(
			_In_ uint32_t atlasIndex) const = 0;

		virtual uint32_t GetMemoryFootprint() const = 0;

		virtual uint32_t GetUsedCount() const = 0;
	};
}
