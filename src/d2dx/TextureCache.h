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
#include "TextureCachePolicyBitPmru.h"

namespace d2dx
{
	class TextureCache final : public ITextureCache
	{
	public:
		TextureCache(
			_In_ int32_t width,
			_In_ int32_t height,
			_In_ uint32_t capacity,
			_In_ uint32_t texturesPerAtlas,
			_In_ ID3D11Device* device,
			_In_ const std::shared_ptr<ISimd>& simd);

		virtual ~TextureCache() noexcept {}

		virtual void OnNewFrame() override;

		virtual TextureCacheLocation FindTexture(
			_In_ uint64_t contentKey,
			_In_ int32_t lastIndex) override;

		virtual TextureCacheLocation InsertTexture(
			_In_ uint64_t contentKey,
			_In_ const Batch& batch,
			_In_reads_(tmuDataSize) const uint8_t* tmuData,
			_In_ uint32_t tmuDataSize) override;

		virtual ID3D11ShaderResourceView* GetSrv(
			_In_ uint32_t atlasIndex) const override;

		virtual uint32_t GetMemoryFootprint() const override;

		virtual uint32_t GetUsedCount() const override;

	private:
		void CopyPixels(
			_In_ int32_t srcWidth,
			_In_ int32_t srcHeight,
			_In_reads_(srcPitch* srcHeight) const uint8_t* __restrict srcPixels,
			_In_ uint32_t srcPitch,
			_Out_writes_all_(dstPitch* srcHeight) uint8_t* __restrict dstPixels,
			_In_ uint32_t dstPitch);

		int32_t _width = 0;
		int32_t _height = 0;
		uint32_t _capacity = 0;
		uint32_t _texturesPerAtlas = 0;
		int32_t _atlasCount = 0;
		ComPtr<ID3D11DeviceContext> _deviceContext;
		ComPtr<ID3D11Texture2D> _textures[4];
		ComPtr<ID3D11ShaderResourceView> _srvs[4];
		TextureCachePolicyBitPmru _policy;
	};
}
