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

#include "Utils.h"
#include "TextureProcessor.h"
#include "Simd.h"
#include "TextureCachePolicy.h"

#define D2DX_TEXTURE_CACHE_IS_ARRAY_BASED

namespace d2dx
{
	class Batch;

	struct TextureCacheLocation
	{
		int32_t OffsetS;
		int32_t OffsetT;
		int32_t ArrayIndex;
	};

	class TextureCache final
	{
	public:
		TextureCache(
			int32_t width,
			int32_t height,
			uint32_t capacity,
			ID3D11Device* device,
			std::shared_ptr<Simd> simd,
			std::shared_ptr<TextureProcessor> textureProcessor);

		void OnNewFrame();

		TextureCacheLocation FindTexture(
			uint32_t contentKey,
			int32_t lastIndex);

		TextureCacheLocation InsertTexture(
			uint32_t contentKey,
			Batch& batch,
			const uint8_t* tmuData,
			const uint32_t* palette);

		uint32_t GetCapacity() const;

		ID3D11Texture2D* GetTexture(
			uint32_t atlasIndex) const;

		ID3D11ShaderResourceView* GetSrv(
			uint32_t atlasIndex) const;

		uint32_t GetMemoryFootprint() const;

	private:
		void ConvertTexture(const Batch& batch, const uint8_t* tmuData, const uint32_t* palette);

		int32_t _atlasWidth;
		int32_t _atlasHeight;
		int32_t _atlasArraySize;

		int32_t _width;
		int32_t _height;
		uint32_t _capacity;

		int32_t _tileCountX;
		int32_t _tileCountY;

		Microsoft::WRL::ComPtr<ID3D11DeviceContext> _deviceContext;
#ifdef D2DX_TEXTURE_CACHE_IS_ARRAY_BASED
		Microsoft::WRL::ComPtr<ID3D11Texture2D> _textures[4];
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _srvs[4];
#else
		Microsoft::WRL::ComPtr<ID3D11Texture2D> _texture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _srv;
#endif
		std::shared_ptr<TextureProcessor> _textureProcessor;
		Buffer<uint8_t> _tempData;
		Buffer<uint8_t> _tempData2;
		Buffer<uint32_t> _convertedData;
		Buffer<uint32_t> _dilatedData;
		std::unique_ptr<TextureCachePolicy> _policy;
	};
}