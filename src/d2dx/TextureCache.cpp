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

#define USE_BIT_MRU 1

using namespace d2dx;
using namespace std;

TextureCache::TextureCache(int32_t width, int32_t height, uint32_t capacity, ID3D11Device* device, shared_ptr<Simd> simd, shared_ptr<TextureProcessor> textureProcessor) :
	_width{ width },
	_height{ height },
	_capacity{ capacity },
	_convertedData{ 256 * 256 },
	_dilatedData{ 256 * 256 },
	_tempData(256 * 256),
	_tempData2(256 * 256),
	_textureProcessor(textureProcessor),
	_policy(make_unique<TextureCachePolicyBitPmru>(capacity, simd))
{
#ifndef GX_UNITTEST

#ifdef D2DX_TEXTURE_CACHE_IS_ARRAY_BASED
	_atlasWidth = _width;
	_atlasHeight = _height;
	_tileCountX = 1;
	_tileCountY = 1;
	_atlasArraySize = capacity / 512;

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

#else
	_atlasWidth = 4096;
	_atlasHeight = 4096;
	_tileCountX = _atlasWidth / width;
	_tileCountY = _atlasHeight / height;
	_atlasArraySize = (int32_t)ceil((float)capacity / (_tileCountX * _tileCountY));

	while (_atlasArraySize == 1)
	{
		int32_t tempAtlasWidth = _atlasWidth / 2;
		int32_t tempAtlasHeight = _atlasHeight / 2;
		int32_t tempTileCountX = (_atlasWidth / 2) / width;
		int32_t tempTileCountY = (_atlasHeight / 2) / height;
		int32_t tempAtlasArraySize = (int32_t)ceil((float)capacity / (tempTileCountX * tempTileCountY));

		if (tempAtlasArraySize > 1)
		{
			break;
		}

		_atlasWidth = tempAtlasWidth;
		_atlasHeight = tempAtlasHeight;
		_tileCountX = tempTileCountX;
		_tileCountY = tempTileCountY;
		_atlasArraySize = tempAtlasArraySize;
	}

	assert((_tileCountX * _tileCountY * _atlasArraySize) >= _capacity);

	CD3D11_TEXTURE2D_DESC desc
	{
		DXGI_FORMAT_R8_UINT,
		(UINT)_atlasWidth,
		(UINT)_atlasHeight,
		(UINT)_atlasArraySize,
		1U,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_DEFAULT
	};

	D2DX_RELEASE_CHECK_HR(device->CreateTexture2D(&desc, nullptr, &_texture));
	D2DX_RELEASE_CHECK_HR(device->CreateShaderResourceView(_texture.Get(), NULL, _srv.GetAddressOf()));
#endif

	device->GetImmediateContext(&_deviceContext);
	assert(_deviceContext);
#endif
}

uint32_t TextureCache::GetMemoryFootprint() const
{
	return _atlasWidth * _atlasHeight * _atlasArraySize * sizeof(uint8_t);
}

TextureCacheLocation TextureCache::FindTexture(uint32_t contentKey, int32_t lastIndex)
{
	int32_t index = _policy->Find(contentKey, lastIndex);

	if (index < 0)
	{
		return { 0, 0, -1 };
	}

#ifdef D2DX_TEXTURE_CACHE_IS_ARRAY_BASED
	return { 0, 0, index };
#else
	const int32_t arrayIndex = index / (_tileCountX * _tileCountY);
	const int32_t subIndex = index % (_tileCountX * _tileCountY);
	const int32_t tileX = subIndex % _tileCountX;
	const int32_t tileY = subIndex / _tileCountX;
	assert(arrayIndex < _atlasArraySize);

	TextureCacheLocation tcl;
	tcl.OffsetS = tileX * _width;
	tcl.OffsetT = tileY * _height;
	tcl.ArrayIndex = arrayIndex;
	return tcl;
#endif
}

TextureCacheLocation TextureCache::InsertTexture(uint32_t contentKey, Batch& batch, const uint8_t* tmuData, const uint32_t* palette)
{
	assert(batch.IsValid() && batch.GetWidth() > 0 && batch.GetHeight() > 0);

	//auto time = timeStart();

//	ConvertTexture(batch, tmuData, palette);

	/*auto elapsed = timeEndMs(time);
	static char ss[256];
	sprintf(ss, "ConvertTexture took %f ms.\n", elapsed);
	OutputDebugStringA(ss);*/

	bool evicted = false;
	int32_t replacementIndex = _policy->Insert(contentKey, evicted);

	if (evicted)
	{
		DEBUG_PRINT("Evicted %ix%i texture %i from cache.", batch.GetWidth(), batch.GetHeight(), replacementIndex);
	}

#ifndef GX_UNITTEST
	CD3D11_BOX box;
#ifdef D2DX_TEXTURE_CACHE_IS_ARRAY_BASED
	box.left = 0;
	box.top = 0;
	box.right = batch.GetWidth();
	box.bottom = batch.GetHeight();
	assert((replacementIndex / 512) < _atlasArraySize);
#else
	int32_t arrayIndex = replacementIndex / (_tileCountX * _tileCountY);
	int32_t subIndex = replacementIndex % (_tileCountX * _tileCountY);
	int32_t tileX = subIndex % _tileCountX;
	int32_t tileY = subIndex / _tileCountX;
	box.left = tileX * _width;
	box.top = tileY * _height;
	box.right = box.left + batch.GetWidth();
	box.bottom = box.top + batch.GetHeight();
	assert(box.left >= 0 && box.left <= _atlasWidth);
	assert(box.right >= 0 && box.right <= _atlasWidth);
	assert(box.top >= 0 && box.top <= _atlasHeight);
	assert(box.bottom >= 0 && box.bottom <= _atlasHeight);
	assert(arrayIndex < _atlasArraySize);
#endif
	box.front = 0;
	box.back = 1;

	const uint8_t* pData = tmuData + batch.GetTextureStartAddress();

	if (batch.IsStFlipped())
	{
		_textureProcessor->Transpose(batch.GetWidth(), batch.GetHeight(), pData, _tempData.items);
		_textureProcessor->CopyPixels(batch.GetWidth(), batch.GetHeight(), _tempData.items, batch.GetWidth(), _tempData2.items, _width);
		pData = _tempData2.items;
	}
#ifdef D2DX_TEXTURE_CACHE_IS_ARRAY_BASED
	_deviceContext->UpdateSubresource(_textures[replacementIndex / 512].Get(), replacementIndex & 511, &box, pData, batch.GetWidth(), 0);
#else
	_deviceContext->UpdateSubresource(_texture.Get(), arrayIndex, &box, pData, batch.GetWidth(), 0);
#endif

#ifdef D2DX_TEXTURE_CACHE_IS_ARRAY_BASED
	return { 0, 0, replacementIndex };
#else
	return { tileX * _width, tileY * _height, arrayIndex };
#endif
#else
	return { 0,0,0 };
#endif
}

uint32_t TextureCache::GetCapacity() const
{
	return _capacity;
}

ID3D11Texture2D* TextureCache::GetTexture(uint32_t atlasIndex) const
{
#ifdef D2DX_TEXTURE_CACHE_IS_ARRAY_BASED
	return _textures[atlasIndex / 512].Get();
#else
	return _texture.Get();
#endif
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
