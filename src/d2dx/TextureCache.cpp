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
#include "D2DXContext.h"
#include "Utils.h"
#include "TextureCache.h"
#include "TextureCachePolicyBitPmru.h"

using namespace d2dx;
using namespace std;

_Use_decl_annotations_
TextureCache::TextureCache(
	int32_t width,
	int32_t height,
	uint32_t capacity,
	uint32_t texturesPerAtlas,
	ID3D11Device* device,
	const std::shared_ptr<ISimd>& simd)
{
	assert(_atlasCount <= 4);

	_width = width;
	_height = height;
	_capacity = capacity;
	_texturesPerAtlas = texturesPerAtlas;
	_atlasCount = (int32_t)max(1, capacity / texturesPerAtlas);
	_policy = TextureCachePolicyBitPmru(capacity, simd);

#ifndef D2DX_UNITTEST

	CD3D11_TEXTURE2D_DESC desc
	{
		DXGI_FORMAT_R8_UINT,
		(UINT)width,
		(UINT)height,
		_texturesPerAtlas,
		1U,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_DEFAULT
	};

	uint32_t capacityPerPartition = _capacity;

	for (int32_t partition = 0; partition < 4; ++partition)
	{
		D2DX_CHECK_HR(device->CreateTexture2D(&desc, nullptr, &_textures[partition]));
		D2DX_CHECK_HR(device->CreateShaderResourceView(_textures[partition].Get(), NULL, _srvs[partition].GetAddressOf()));
	}

	device->GetImmediateContext(&_deviceContext);
	assert(_deviceContext);
#endif
}

uint32_t TextureCache::GetMemoryFootprint() const
{
	return _width * _height * _texturesPerAtlas * _atlasCount;
}

_Use_decl_annotations_
TextureCacheLocation TextureCache::FindTexture(
	uint32_t contentKey,
	int32_t lastIndex)
{
	const int32_t index = _policy.Find(contentKey, lastIndex);

	if (index < 0)
	{
		return { -1, -1 };
	}

	return { (int16_t)(index / _texturesPerAtlas), (int16_t)(index & (_texturesPerAtlas - 1)) };
}

_Use_decl_annotations_
TextureCacheLocation TextureCache::InsertTexture(
	uint32_t contentKey,
	const Batch& batch,
	const uint8_t* tmuData,
	uint32_t tmuDataSize)
{
	assert(batch.IsValid() && batch.GetTextureWidth() > 0 && batch.GetTextureHeight() > 0);

	bool evicted = false;
	int32_t replacementIndex = _policy.Insert(contentKey, evicted);

	if (evicted)
	{
		D2DX_DEBUG_LOG("Evicted %ix%i texture %i from cache.", batch.GetTextureWidth(), batch.GetTextureHeight(), replacementIndex);
	}

#ifndef D2DX_UNITTEST
	CD3D11_BOX box;
	box.left = 0;
	box.top = 0;
	box.right = batch.GetTextureWidth();
	box.bottom = batch.GetTextureHeight();
	box.front = 0;
	box.back = 1;

	const uint8_t* pData = tmuData + batch.GetTextureStartAddress();

	_deviceContext->UpdateSubresource(_textures[replacementIndex / _texturesPerAtlas].Get(), replacementIndex & (_texturesPerAtlas - 1), &box, pData, batch.GetTextureWidth(), 0);
#endif

	return { (int16_t)(replacementIndex / _texturesPerAtlas), (int16_t)(replacementIndex & (_texturesPerAtlas - 1)) };
}

_Use_decl_annotations_
ID3D11ShaderResourceView* TextureCache::GetSrv(
	uint32_t textureAtlas) const
{
	assert(textureAtlas >= 0 && textureAtlas < (uint32_t)_atlasCount);
	return _srvs[textureAtlas].Get();
}

void TextureCache::OnNewFrame()
{
	_policy.OnNewFrame();
}

_Use_decl_annotations_
void TextureCache::CopyPixels(
	int32_t srcWidth,
	int32_t srcHeight,
	const uint8_t* __restrict srcPixels,
	uint32_t srcPitch,
	uint8_t* __restrict dstPixels,
	uint32_t dstPitch)
{
	const int32_t srcSkip = srcPitch - srcWidth;
	const int32_t dstSkip = dstPitch - srcWidth;

	assert(srcSkip >= 0);
	assert(dstSkip >= 0);

	for (int32_t y = 0; y < srcHeight; ++y)
	{
		for (int32_t x = 0; x < srcWidth; ++x)
		{
			*dstPixels++ = *srcPixels++;
		}
		srcPixels += srcSkip;
		dstPixels += dstSkip;
	}
}

uint32_t TextureCache::GetUsedCount() const
{
	return _policy.GetUsedCount();
}
