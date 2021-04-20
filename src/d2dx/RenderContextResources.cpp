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
#include "RenderContextResources.h"
#include "Utils.h"
#include "Types.h"
#include "TextureCache.h"
#include "DisplayVS_cso.h"
#include "DisplayNonintegerScalePS_cso.h"
#include "DisplayIntegerScalePS_cso.h"
#include "GamePS_cso.h"
#include "GameVS_cso.h"
#include "VideoPS_cso.h"
#include "GammaPS_cso.h"
#include "ResolveAA_cso.h"
#include "Metrics.h"

using namespace d2dx;

_Use_decl_annotations_
HRESULT RenderContextResources::RuntimeClassInitialize(
	uint32_t vbSizeBytes,
	uint32_t cbSizeBytes,
	Size framebufferSize,
	ID3D11Device* device,
	ISimd* simd)
{
	HRESULT hr = S_OK;

	D2DX_RETURN_IF_FAILED(
		CreateTexture1Ds(device));

	D2DX_RETURN_IF_FAILED(
		CreateTextureCaches(device, simd));

	D2DX_RETURN_IF_FAILED(
		CreateVideoTextures(device));

	D2DX_RETURN_IF_FAILED(
		CreateShadersAndInputLayout(device));

	D2DX_RETURN_IF_FAILED(
		CreateRasterizerState(device));

	D2DX_RETURN_IF_FAILED(
		CreateSamplerStates(device));

	D2DX_RETURN_IF_FAILED(
		CreateBlendStates(device));

	D2DX_RETURN_IF_FAILED(
		CreateFramebuffers(framebufferSize, device));

	D2DX_RETURN_IF_FAILED(
		CreateVertexBuffer(vbSizeBytes, device));
	
	D2DX_RETURN_IF_FAILED(
		CreateConstantBuffer(cbSizeBytes, device));

	return hr;
}

RenderContextResources::~RenderContextResources()
{
}

void RenderContextResources::OnNewFrame()
{
	for (int32_t i = 0; i < ARRAYSIZE(_textureCaches); ++i)
	{
		_textureCaches[i]->OnNewFrame();
	}
}

ITextureCache* RenderContextResources::GetTextureCache(int32_t textureWidth, int32_t textureHeight) const
{
	const int32_t longest = max(textureWidth, textureHeight);
	assert(longest >= 8);
	uint32_t log2Longest = 0;
	BitScanForward((DWORD*)&log2Longest, (DWORD)longest);
	log2Longest -= 3;
	assert(log2Longest <= 5);
	return _textureCaches[log2Longest].Get();
}

_Use_decl_annotations_
HRESULT RenderContextResources::CreateShadersAndInputLayout(
	ID3D11Device* device)
{
	HRESULT hr;

	D2DX_RETURN_IF_FAILED(
		device->CreateVertexShader(GameVS_cso, ARRAYSIZE(GameVS_cso), NULL, &_vertexShaders[(int32_t)RenderContextVertexShader::Game]));

	D2DX_RETURN_IF_FAILED(
		device->CreatePixelShader(GamePS_cso, ARRAYSIZE(GamePS_cso), NULL, &_pixelShaders[(int32_t)RenderContextPixelShader::Game]));

	D2DX_RETURN_IF_FAILED(
		device->CreatePixelShader(VideoPS_cso, ARRAYSIZE(VideoPS_cso), NULL, &_pixelShaders[(int32_t)RenderContextPixelShader::Video]));

	D2DX_RETURN_IF_FAILED(
		device->CreateVertexShader(DisplayVS_cso, ARRAYSIZE(DisplayVS_cso), NULL, &_vertexShaders[(int32_t)RenderContextVertexShader::Display]));

	D2DX_RETURN_IF_FAILED(
		device->CreatePixelShader(DisplayIntegerScalePS_cso, ARRAYSIZE(DisplayIntegerScalePS_cso), NULL, &_pixelShaders[(int32_t)RenderContextPixelShader::DisplayIntegerScale]));

	D2DX_RETURN_IF_FAILED(
		device->CreatePixelShader(DisplayNonintegerScalePS_cso, ARRAYSIZE(DisplayNonintegerScalePS_cso), NULL, &_pixelShaders[(int32_t)RenderContextPixelShader::DisplayNonintegerScale]));

	D2DX_RETURN_IF_FAILED(
		device->CreatePixelShader(GammaPS_cso, ARRAYSIZE(GammaPS_cso), NULL, &_pixelShaders[(int32_t)RenderContextPixelShader::Gamma]));

	D2DX_RETURN_IF_FAILED(
		device->CreatePixelShader(ResolveAA_cso, ARRAYSIZE(ResolveAA_cso), NULL, &_pixelShaders[(int32_t)RenderContextPixelShader::ResolveAA]));

	D3D11_INPUT_ELEMENT_DESC inputElementDescs[4] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R16G16_SINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_SINT, 0, 4, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R16G16_UINT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D2DX_RETURN_IF_FAILED(
		device->CreateInputLayout(inputElementDescs, ARRAYSIZE(inputElementDescs), GameVS_cso, ARRAYSIZE(GameVS_cso), &_inputLayout));

	return hr;
}

_Use_decl_annotations_
HRESULT RenderContextResources::CreateTexture1Ds(
	ID3D11Device* device)
{
	HRESULT hr = S_OK;

	CD3D11_TEXTURE1D_DESC desc{
		DXGI_FORMAT_B8G8R8A8_UNORM,
		(UINT)256,
		D2DX_MAX_PALETTES,
		1U,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_DEFAULT
	};

	uint32_t initialPalettes[D2DX_MAX_PALETTES * 256];
	memset(initialPalettes, 0, D2DX_MAX_PALETTES * 256 * sizeof(uint32_t));
	for (int32_t i = 0; i < 256; ++i)
	{
		initialPalettes[D2DX_WHITE_PALETTE_INDEX * 256 + i] = 0xFFFFFFFF;
	}

	D3D11_SUBRESOURCE_DATA subResourceData[D2DX_MAX_PALETTES];
	for (int32_t i = 0; i < D2DX_MAX_PALETTES; ++i)
	{
		subResourceData[i].pSysMem = initialPalettes + 256 * i;
		subResourceData[i].SysMemPitch = 0;
		subResourceData[i].SysMemSlicePitch = 0;
	}

	D2DX_RETURN_IF_FAILED(
		device->CreateTexture1D(
			&desc,
			&subResourceData[0],
			&_texture1Ds[(int32_t)RenderContextTexture1D::Palette].texture));

	D2DX_RETURN_IF_FAILED(
		device->CreateShaderResourceView(
			_texture1Ds[(int32_t)RenderContextTexture1D::Palette].texture.Get(),
			nullptr,
			&_texture1Ds[(int32_t)RenderContextTexture1D::Palette].srv));

	desc.ArraySize = 1;

	D2DX_RETURN_IF_FAILED(
		device->CreateTexture1D(
			&desc,
			nullptr,
			&_texture1Ds[(int32_t)RenderContextTexture1D::GammaTable].texture));

	D2DX_RETURN_IF_FAILED(
		device->CreateShaderResourceView(
			_texture1Ds[(int32_t)RenderContextTexture1D::GammaTable].texture.Get(),
			nullptr,
			&_texture1Ds[(int32_t)RenderContextTexture1D::GammaTable].srv));

	return hr;
}

_Use_decl_annotations_
HRESULT RenderContextResources::CreateTextureCaches(
	ID3D11Device* device,
	ISimd* simd)
{
	HRESULT hr = S_OK;

	static const uint32_t capacities[6] = { 2048, 2048, 2048, 2048, 2048, 2048 };

	const uint32_t texturesPerAtlas = DetermineMaxTextureArraySize(device);
	ALWAYS_PRINT("The device supports %u textures per atlas.", texturesPerAtlas);

	uint32_t totalSize = 0;
	for (int32_t i = 0; i < ARRAYSIZE(_textureCaches); ++i)
	{
		int32_t width = 1U << (i + 3);

		hr = MakeAndInitialize<TextureCache, ITextureCache>(&_textureCaches[i],
			width, width, capacities[i], texturesPerAtlas, device, simd);

		if (FAILED(hr))
		{
			return hr;
		}

		DEBUG_PRINT("Creating texture cache for %i x %i with capacity %u (%u kB).", width, width, capacities[i], _textureCaches[i]->GetMemoryFootprint() / 1024);

		totalSize += _textureCaches[i]->GetMemoryFootprint();
	}

	ALWAYS_PRINT("Total size of texture caches is %u kB.", totalSize / 1024);
	return hr;
}

_Use_decl_annotations_
uint32_t RenderContextResources::DetermineMaxTextureArraySize(
	ID3D11Device* device)
{
	for (uint32_t arraySize = 2048; arraySize > 512; arraySize /= 2)
	{
		CD3D11_TEXTURE2D_DESC desc{
			DXGI_FORMAT_R8G8B8A8_UNORM,
			16U,
			16U,
			arraySize,
			1U,
			D3D11_BIND_SHADER_RESOURCE,
			D3D11_USAGE_DEFAULT
		};

		ComPtr<ID3D11Texture2D> tempTexture;
		HRESULT hr = device->CreateTexture2D(&desc, nullptr, &tempTexture);

		if (SUCCEEDED(hr))
		{
			return arraySize;
		}
	}

	return 512;
}

_Use_decl_annotations_
HRESULT RenderContextResources::CreateVideoTextures(
	ID3D11Device* device)
{
	HRESULT hr = S_OK;

	CD3D11_TEXTURE2D_DESC desc
	{
		DXGI_FORMAT_B8G8R8A8_UNORM,
		640,
		480,
		1U,
		1U,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE
	};

	_videoTextureSize = { 640, 480 };

	D2DX_RETURN_IF_FAILED(
		device->CreateTexture2D(&desc, NULL, &_videoTexture));

	D2DX_RETURN_IF_FAILED(
		device->CreateShaderResourceView(_videoTexture.Get(), NULL, &_videoTextureSrv));

	return hr;
}

_Use_decl_annotations_
HRESULT RenderContextResources::CreateFramebuffers(
	Size framebufferSize,
	ID3D11Device* device)
{
	HRESULT hr;

	DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	UINT formatSupport = 0;
	hr = device->CheckFormatSupport(DXGI_FORMAT_R10G10B10A2_UNORM, &formatSupport);
	if (SUCCEEDED(hr) &&
		(formatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D) &&
		(formatSupport & D3D11_FORMAT_SUPPORT_SHADER_LOAD) &&
		(formatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET))
	{
		ALWAYS_PRINT("Using DXGI_FORMAT_R10G10B10A2_UNORM for the render buffer.");
		renderTargetFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	}
	else
	{
		ALWAYS_PRINT("Using DXGI_FORMAT_R8G8B8A8_UNORM for the render buffer.");
		hr = S_OK;
	}

	_framebufferSize = framebufferSize;

	CD3D11_TEXTURE2D_DESC desc
	{
		renderTargetFormat,
		(UINT)framebufferSize.width,
		(UINT)framebufferSize.height,
		1U,
		1U,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		D3D11_USAGE_DEFAULT
	};

	CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc
	{
		D3D11_SRV_DIMENSION_TEXTURE2D,
		renderTargetFormat
	};

	D2DX_RETURN_IF_FAILED(
		device->CreateTexture2D(
			&desc,
			NULL,
			&_framebuffers[(int32_t)RenderContextFramebuffer::Game].texture));

	D2DX_RETURN_IF_FAILED(
		device->CreateShaderResourceView(
			_framebuffers[(int32_t)RenderContextFramebuffer::Game].texture.Get(),
			&srvDesc,
			&_framebuffers[(int32_t)RenderContextFramebuffer::Game].srv));

	CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc{
		D3D11_RTV_DIMENSION_TEXTURE2D,
		renderTargetFormat
	};

	D2DX_RETURN_IF_FAILED(
		device->CreateRenderTargetView(
			_framebuffers[(int32_t)RenderContextFramebuffer::Game].texture.Get(),
			&rtvDesc,
			&_framebuffers[(int32_t)RenderContextFramebuffer::Game].rtv));

	desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
	D2DX_RETURN_IF_FAILED(
		device->CreateTexture2D(
			&desc,
			NULL,
			&_framebuffers[(int32_t)RenderContextFramebuffer::GammaCorrected].texture));

	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D2DX_RETURN_IF_FAILED(
		device->CreateShaderResourceView(
			_framebuffers[(int32_t)RenderContextFramebuffer::GammaCorrected].texture.Get(),
			&srvDesc,
			&_framebuffers[(int32_t)RenderContextFramebuffer::GammaCorrected].srv));

	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D2DX_RETURN_IF_FAILED(
		device->CreateRenderTargetView(
			_framebuffers[(int32_t)RenderContextFramebuffer::GammaCorrected].texture.Get(),
			&rtvDesc,
			&_framebuffers[(int32_t)RenderContextFramebuffer::GammaCorrected].rtv));

	desc.Format = DXGI_FORMAT_R16_TYPELESS;
	D2DX_RETURN_IF_FAILED(
		device->CreateTexture2D(
			&desc,
			NULL,
			&_framebuffers[(int32_t)RenderContextFramebuffer::SurfaceId].texture));

	srvDesc.Format = DXGI_FORMAT_R16_UNORM;
	D2DX_RETURN_IF_FAILED(
		device->CreateShaderResourceView(
			_framebuffers[(int32_t)RenderContextFramebuffer::SurfaceId].texture.Get(),
			&srvDesc,
			&_framebuffers[(int32_t)RenderContextFramebuffer::SurfaceId].srv));

	rtvDesc.Format = DXGI_FORMAT_R16_UNORM;
	D2DX_RETURN_IF_FAILED(
		device->CreateRenderTargetView(
			_framebuffers[(int32_t)RenderContextFramebuffer::SurfaceId].texture.Get(),
			&rtvDesc,
			&_framebuffers[(int32_t)RenderContextFramebuffer::SurfaceId].rtv));

	return hr;
}

_Use_decl_annotations_
HRESULT RenderContextResources::CreateRasterizerState(
	ID3D11Device* device)
{
	HRESULT hr;

	CD3D11_RASTERIZER_DESC rasterizerDesc{ CD3D11_DEFAULT() };

	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.DepthClipEnable = false;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.ScissorEnable = FALSE;
	D2DX_RETURN_IF_FAILED(
		device->CreateRasterizerState(&rasterizerDesc, &_rasterizerStateNoScissor));

	rasterizerDesc.ScissorEnable = TRUE;
	D2DX_RETURN_IF_FAILED(
		device->CreateRasterizerState(&rasterizerDesc, &_rasterizerState));

	return hr;
}

_Use_decl_annotations_
HRESULT RenderContextResources::CreateSamplerStates(
	ID3D11Device* device)
{
	HRESULT hr;

	CD3D11_SAMPLER_DESC samplerDesc{ CD3D11_DEFAULT() };

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	D2DX_RETURN_IF_FAILED(
		device->CreateSamplerState(&samplerDesc, _samplerState[0].GetAddressOf()));

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	D2DX_RETURN_IF_FAILED(
		device->CreateSamplerState(&samplerDesc, _samplerState[1].GetAddressOf()));

	return hr;
}

_Use_decl_annotations_
HRESULT RenderContextResources::CreateBlendStates(
	ID3D11Device* device)
{
	HRESULT hr;

	for (int32_t i = 0; i < (int32_t)AlphaBlend::Count; ++i)
	{
		AlphaBlend alphaBlend = (AlphaBlend)i;

		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));
		blendDesc.IndependentBlendEnable = TRUE;

		if (alphaBlend == AlphaBlend::Opaque)
		{
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

			blendDesc.RenderTarget[1].BlendEnable = TRUE;
			blendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[1].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[1].DestBlend = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_MAX;
			blendDesc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		}
		else if (alphaBlend == AlphaBlend::Additive)
		{
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

			blendDesc.RenderTarget[1].BlendEnable = TRUE;
			blendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[1].SrcBlend = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].DestBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		}
		else if (alphaBlend == AlphaBlend::Multiplicative)
		{
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

			blendDesc.RenderTarget[1].BlendEnable = TRUE;
			blendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[1].SrcBlend = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].DestBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		}
		else if (alphaBlend == AlphaBlend::SrcAlphaInvSrcAlpha)
		{
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

			blendDesc.RenderTarget[1].BlendEnable = TRUE;
			blendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[1].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[1].DestBlend = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_MAX;
			blendDesc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		}
		else
		{
			assert(false && "Unhandled alphablend.");
		}

		D2DX_RETURN_IF_FAILED(
			device->CreateBlendState(&blendDesc, &_blendStates[i]));
	}

	return hr;
}

_Use_decl_annotations_
HRESULT RenderContextResources::CreateVertexBuffer(
	uint32_t vbSizeBytes,
	ID3D11Device* device)
{
	HRESULT hr;

	const CD3D11_BUFFER_DESC vbDesc
	{
		vbSizeBytes,
		D3D11_BIND_VERTEX_BUFFER,
		D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE
	};

	D2DX_RETURN_IF_FAILED(
		device->CreateBuffer(&vbDesc, NULL, &_vb));

	return hr;
}

_Use_decl_annotations_
HRESULT RenderContextResources::CreateConstantBuffer(
	uint32_t cbSizeBytes,
	ID3D11Device* device)
{
	HRESULT hr;

	CD3D11_BUFFER_DESC desc
	{
		cbSizeBytes,
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE
	};

	D2DX_RETURN_IF_FAILED(
		device->CreateBuffer(&desc, NULL, _cb.GetAddressOf()));
	
	return hr;
}

