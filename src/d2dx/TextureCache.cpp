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
#include "pch.h"
#include "D2DXContext.h"
#include "Utils.h"
#include "TextureCache.h"
#include "TextureCachePolicyBitPmru.h"

using namespace d2dx;
using namespace std;

TextureCache::TextureCache(int32_t width, int32_t height, uint32_t capacity, ID3D11Device* device, shared_ptr<Simd> simd, shared_ptr<TextureProcessor> textureProcessor) :
	_width{ width },
	_height{ height },
	_capacity{ capacity },
	_textureProcessor(textureProcessor),
	_policy(make_unique<TextureCachePolicyBitPmru>(capacity, simd))
{
#ifndef D2DX_UNITTEST

	_atlasWidth = _width;
	_atlasHeight = _height;
	_tileCountX = 1;
	_tileCountY = 1;
	_atlasArraySize = capacity;

	CD3D11_TEXTURE2D_DESC desc
	{
		DXGI_FORMAT_R8_UINT,
		(UINT)width,
		(UINT)height,
		512U,
		1U,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_DEFAULT
	};

	HRESULT hr = S_OK;
	uint32_t capacityPerPartition = _capacity;

	for (int32_t partition = 0; partition < 4; ++partition)
	{
		D2DX_RELEASE_CHECK_HR(device->CreateTexture2D(&desc, nullptr, &_textures[partition]));
		D2DX_RELEASE_CHECK_HR(device->CreateShaderResourceView(_textures[partition].Get(), NULL, _srvs[partition].GetAddressOf()));
	}

	device->GetImmediateContext(&_deviceContext);
	assert(_deviceContext);
#endif
}

uint32_t TextureCache::GetMemoryFootprint() const
{
	return _atlasWidth * _atlasHeight * _atlasArraySize * sizeof(uint8_t);
}

int32_t TextureCache::FindTexture(uint32_t contentKey, int32_t lastIndex)
{
	return _policy->Find(contentKey, lastIndex);
}

int32_t TextureCache::InsertTexture(uint32_t contentKey, const Batch& batch, const uint8_t* tmuData)
{
	assert(batch.IsValid() && batch.GetWidth() > 0 && batch.GetHeight() > 0);

	bool evicted = false;
	int32_t replacementIndex = _policy->Insert(contentKey, evicted);

	if (evicted)
	{
		DEBUG_PRINT("Evicted %ix%i texture %i from cache.", batch.GetWidth(), batch.GetHeight(), replacementIndex);
	}

#ifndef D2DX_UNITTEST
	CD3D11_BOX box;
	box.left = 0;
	box.top = 0;
	box.right = batch.GetWidth();
	box.bottom = batch.GetHeight();
	assert(replacementIndex < _atlasArraySize);
	box.front = 0;
	box.back = 1;

	const uint8_t* pData = tmuData + batch.GetTextureStartAddress();

	_deviceContext->UpdateSubresource(_textures[replacementIndex / 512].Get(), replacementIndex & 511, &box, pData, batch.GetWidth(), 0);

	return replacementIndex;
#else
	return replacementIndex;
#endif
}

uint32_t TextureCache::GetCapacity() const
{
	return _capacity;
}

ID3D11Texture2D* TextureCache::GetTexture(uint32_t atlasIndex) const
{
	return _textures[atlasIndex / 512].Get();
}

ID3D11ShaderResourceView* TextureCache::GetSrv(uint32_t atlasIndex) const
{
#ifdef D2DX_TEXTURE_CACHE_IS_ARRAY_BASED
	return _srvs[atlasIndex / 512].Get();
#else
	return _srv.Get();
#endif
}

void TextureCache::OnNewFrame()
{
	_policy->OnNewFrame();
}
