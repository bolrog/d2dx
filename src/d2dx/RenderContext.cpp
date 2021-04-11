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
#include "Batch.h"
#include "D2DXContextFactory.h"
#include "RenderContext.h"
#include "DisplayVS_cso.h"
#include "DisplayNonintegerScalePS_cso.h"
#include "DisplayIntegerScalePS_cso.h"
#include "Metrics.h"
#include "GamePS_cso.h"
#include "GameVS_cso.h"
#include "VideoPS_cso.h"
#include "GammaPS_cso.h"
#include "TextureCache.h"
#include "Vertex.h"
#include "Utils.h"

using namespace d2dx;
using namespace std;

extern int (WINAPI* ShowCursor_Real)(
	_In_ BOOL bShow);

extern BOOL(WINAPI* SetWindowPos_Real)(
	_In_ HWND hWnd,
	_In_opt_ HWND hWndInsertAfter,
	_In_ int X,
	_In_ int Y,
	_In_ int cx,
	_In_ int cy,
	_In_ UINT uFlags);

static LRESULT CALLBACK d2dxSubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

RenderContext::RenderContext(
	HWND hWnd,
	Size gameSize,
	Size windowSize,
	Options& options,
	ISimd* simd) :
	_hWnd{ hWnd },
	_vbWriteIndex{ 0 },
	_vbCapacity{ 0 },
	_options{ options },
	_constants{ 0 },
	_frameLatencyWaitableObject{ nullptr },
	_simd{ simd }
{
	memset(&_shadowState, 0, sizeof(_shadowState));

	_desktopSize = { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	_desktopClientMaxHeight = GetSystemMetrics(SM_CYFULLSCREEN);

	_gameSize = gameSize;
	_windowSize = windowSize;
	_renderRect = Metrics::GetRenderRect(
		gameSize,
		_options.screenMode == ScreenMode::FullscreenDefault ? _desktopSize : _windowSize,
		!_options.noWide);

#ifndef NDEBUG
	ShowCursor_Real(TRUE);
#endif

	RECT clientRect;
	GetClientRect(hWnd, &clientRect);

	SetWindowSubclass((HWND)hWnd, d2dxSubclassWndProc, 1234, (DWORD_PTR)this);

	const int32_t widthFromClientRect = clientRect.right - clientRect.left;
	const int32_t heightFromClientRect = clientRect.bottom - clientRect.top;

	_featureLevel = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL requestedFeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1
	};

	_dxgiAllowTearingFlagSupported = IsAllowTearingFlagSupported();
	_frameLatencyWaitableObjectSupported = IsFrameLatencyWaitableObjectSupported();

	_swapChainCreateFlags = 0;

	if (_options.noVSync)
	{
		if (_dxgiAllowTearingFlagSupported)
		{
			_syncStrategy = RenderContextSyncStrategy::AllowTearing;
			_swapChainCreateFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
			ALWAYS_PRINT("Using 'AllowTearing' sync strategy.")
		}
		else
		{
			_syncStrategy = RenderContextSyncStrategy::Interval0;
			ALWAYS_PRINT("Using 'Interval0' sync strategy.")
		}
	}
	else
	{
		if (_frameLatencyWaitableObjectSupported)
		{
			_syncStrategy = RenderContextSyncStrategy::FrameLatencyWaitableObject;
			_swapChainCreateFlags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			ALWAYS_PRINT("Using 'FrameLatencyWaitableObject' sync strategy.")
		}
		else
		{
			_syncStrategy = RenderContextSyncStrategy::Interval1;
			ALWAYS_PRINT("Using 'Interval1' sync strategy.")
		}
	}

	if (GetWindowsVersion().major >= 10)
	{
		_swapStrategy = RenderContextSwapStrategy::FlipDiscard;
		ALWAYS_PRINT("Using 'FlipDiscard' swap strategy.")
	}
	else
	{
		_swapStrategy = RenderContextSwapStrategy::Discard;
		ALWAYS_PRINT("Using 'Discard' swap strategy.")
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Width = _desktopSize.width;
	swapChainDesc.BufferDesc.Height = _desktopSize.height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
	swapChainDesc.SwapEffect = _swapStrategy == RenderContextSwapStrategy::FlipDiscard ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = _swapChainCreateFlags;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;

#ifdef NDEBUG
	uint32_t flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;
#else
	uint32_t flags = D3D11_CREATE_DEVICE_DEBUG;
#endif

	ComPtr<IDXGISwapChain> swapChain;

	D2DX_RELEASE_CHECK_HR(D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		flags,
		requestedFeatureLevels,
		ARRAYSIZE(requestedFeatureLevels),
		D3D11_SDK_VERSION,
		&swapChainDesc,
		swapChain.GetAddressOf(),
		&_device,
		&_featureLevel,
		&_deviceContext));

	ALWAYS_PRINT("Created device with feature level %u.", _featureLevel);

	D2DX_RELEASE_CHECK_HR(swapChain.As(&_swapChain1));

	ComPtr<IDXGIFactory> dxgiFactory;
	_swapChain1->GetParent(IID_PPV_ARGS(&dxgiFactory));
	if (dxgiFactory)
	{
		dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_WINDOW_CHANGES);
	}

	if (SUCCEEDED(_swapChain1.As(&_swapChain2)))
	{
		_backbufferSizingStrategy = RenderContextBackbufferSizingStrategy::SetSourceSize;
		ALWAYS_PRINT("Using 'SetSourceSize' backbuffer sizing strategy.")

		if (!_options.noVSync && _frameLatencyWaitableObjectSupported)
		{
			ALWAYS_PRINT("Will sync using IDXGISwapChain2::GetFrameLatencyWaitableObject.");
			_frameLatencyWaitableObject = _swapChain2->GetFrameLatencyWaitableObject();
		}
	}
	else
	{
		_backbufferSizingStrategy = RenderContextBackbufferSizingStrategy::ResizeBuffers;
		ALWAYS_PRINT("Using 'ResizeBuffers' backbuffer sizing strategy.")
	}

	if (SUCCEEDED(_deviceContext->QueryInterface(IID_PPV_ARGS(&_deviceContext1))))
	{
		ALWAYS_PRINT("Device context supports ID3D11DeviceContext1. Will use this to discard resources and views.");
	}
	else
	{
		ALWAYS_PRINT("Device context does not support ID3D11DeviceContext1.");
	}

	if (_swapChain2)
	{
		ComPtr<IDXGIDevice1> dxgiDevice1;
		if (SUCCEEDED(_swapChain1->GetDevice(IID_PPV_ARGS(&dxgiDevice1))))
		{
			ALWAYS_PRINT("Setting maximum frame latency to 2.");
			D2DX_RELEASE_CHECK_HR(dxgiDevice1->SetMaximumFrameLatency(2));
		}
	}

	// get a pointer directly to the back buffer
	ComPtr<ID3D11Texture2D> backbuffer;
	D2DX_RELEASE_CHECK_HR(_swapChain1->GetBuffer(0, __uuidof(ID3D11Texture2D), &backbuffer));

	// create a render target pointing to the back buffer
	D2DX_RELEASE_CHECK_HR(_device->CreateRenderTargetView(backbuffer.Get(), nullptr, &_backbufferRtv));

	backbuffer = nullptr;

	float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	_deviceContext->ClearRenderTargetView(_backbufferRtv.Get(), color);

	CreateTextureCaches();
	CreateVertexAndIndexBuffers();
	CreateGammaTexture();
	CreatePaletteTexture();
	CreateVideoTextures();
	CreateShadersAndInputLayout();
	CreateSamplerStates();
	CreateConstantBuffers();
	CreateGameTexture();

	_deviceContext->VSSetConstantBuffers(0, 1, _cb.GetAddressOf());
	_deviceContext->PSSetConstantBuffers(0, 1, _cb.GetAddressOf());

	CreateRasterizerState();

	AdjustWindowPlacement(hWnd, false);
}

RenderContext::~RenderContext()
{
	if (_frameLatencyWaitableObject)
	{
		CloseHandle(_frameLatencyWaitableObject);
		_frameLatencyWaitableObject = nullptr;
	}
}

void RenderContext::CreateRasterizerState()
{
	CD3D11_RASTERIZER_DESC rasterizerDesc{ CD3D11_DEFAULT() };

	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.DepthClipEnable = false;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.ScissorEnable = FALSE;
	D2DX_RELEASE_CHECK_HR(_device->CreateRasterizerState(&rasterizerDesc, &_rasterizerStateNoScissor));

	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.DepthClipEnable = false;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.ScissorEnable = TRUE;
	D2DX_RELEASE_CHECK_HR(_device->CreateRasterizerState(&rasterizerDesc, &_rasterizerState));

	_deviceContext->RSSetState(_rasterizerState.Get());
}

void RenderContext::CreateVertexAndIndexBuffers()
{
	_vbCapacity = 1024 * 1024;

	const CD3D11_BUFFER_DESC vbDesc
	{
		sizeof(Vertex) * _vbCapacity,
		D3D11_BIND_VERTEX_BUFFER,
		D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE
	};

	D2DX_RELEASE_CHECK_HR(_device->CreateBuffer(&vbDesc, NULL, &_vb));

	uint32_t stride = sizeof(Vertex);
	uint32_t offset = 0;
	_deviceContext->IASetVertexBuffers(0, 1, _vb.GetAddressOf(), &stride, &offset);
}

void RenderContext::CreateShadersAndInputLayout()
{
	D2DX_RELEASE_CHECK_HR(_device->CreateVertexShader(GameVS_cso, ARRAYSIZE(GameVS_cso), NULL, &_gameVS));
	D2DX_RELEASE_CHECK_HR(_device->CreatePixelShader(GamePS_cso, ARRAYSIZE(GamePS_cso), NULL, &_gamePS));
	D2DX_RELEASE_CHECK_HR(_device->CreatePixelShader(VideoPS_cso, ARRAYSIZE(VideoPS_cso), NULL, &_videoPS));
	D2DX_RELEASE_CHECK_HR(_device->CreateVertexShader(DisplayVS_cso, ARRAYSIZE(DisplayVS_cso), NULL, &_displayVS));
	D2DX_RELEASE_CHECK_HR(_device->CreatePixelShader(DisplayIntegerScalePS_cso, ARRAYSIZE(DisplayIntegerScalePS_cso), NULL, &_displayIntegerScalePS));
	D2DX_RELEASE_CHECK_HR(_device->CreatePixelShader(DisplayNonintegerScalePS_cso, ARRAYSIZE(DisplayNonintegerScalePS_cso), NULL, &_displayNonintegerScalePS));
	D2DX_RELEASE_CHECK_HR(_device->CreatePixelShader(GammaPS_cso, ARRAYSIZE(GammaPS_cso), NULL, &_gammaPS));

	D3D11_INPUT_ELEMENT_DESC inputElementDescs[4] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_SINT, 0, 4, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R16G16_UINT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D2DX_RELEASE_CHECK_HR(_device->CreateInputLayout(inputElementDescs, ARRAYSIZE(inputElementDescs), GameVS_cso, ARRAYSIZE(GameVS_cso), &_inputLayout));

	_deviceContext->IASetInputLayout(_inputLayout.Get());
}

void RenderContext::CreateConstantBuffers()
{
	CD3D11_BUFFER_DESC cb1Desc
	{
		sizeof(Constants),
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_USAGE_DYNAMIC,
		D3D11_CPU_ACCESS_WRITE
	};

	D2DX_RELEASE_CHECK_HR(_device->CreateBuffer(&cb1Desc, NULL, _cb.GetAddressOf()));
}

void RenderContext::CreateSamplerStates()
{
	CD3D11_SAMPLER_DESC samplerDesc{ CD3D11_DEFAULT() };

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	D2DX_RELEASE_CHECK_HR(_device->CreateSamplerState(&samplerDesc, _samplerState[0].GetAddressOf()));

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	D2DX_RELEASE_CHECK_HR(_device->CreateSamplerState(&samplerDesc, _samplerState[1].GetAddressOf()));

	ID3D11SamplerState* samplerState[2] = { _samplerState[0].Get(), _samplerState[1].Get() };
	_deviceContext->PSSetSamplers(0, 2, samplerState);
}

_Use_decl_annotations_
void RenderContext::Draw(
	const Batch& batch)
{
	SetVS(_gameVS.Get());
	SetPS(_gamePS.Get());

	SetBlendState(batch.GetAlphaBlend(),
		(batch.GetTextureCategory() == TextureCategory::UserInterface ||
 		 batch.GetTextureCategory() == TextureCategory::MousePointer ||
		 batch.GetTextureCategory() == TextureCategory::Font ||
			batch.GetAlphaBlend() == AlphaBlend::Additive)
		? 1.0f : 
		32.0f/255.0f);

	ITextureCache* atlas = GetTextureCache(batch);
	ID3D11ShaderResourceView* srvs[2] = { atlas->GetSrv(batch.GetTextureAtlas()), _paletteTextureSrv.Get() };
	SetPSShaderResourceViews(srvs);

	switch (batch.GetPrimitiveType())
	{
	case PrimitiveType::Triangles:
	{
		SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_deviceContext->Draw(batch.GetVertexCount(), batch.GetStartVertex());
		break;
	}
	case PrimitiveType::Lines:
	{
		SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		_deviceContext->Draw(batch.GetVertexCount(), batch.GetStartVertex());
		break;
	}
	default:
		assert(false && "Unhandled primitive type.");
		break;
	}
}

bool RenderContext::IsIntegerScale() const
{
	float scaleX = ((float)_renderRect.size.width / _gameSize.width);
	float scaleY = ((float)_renderRect.size.height / _gameSize.height);

	return fabs(scaleX - floor(scaleX)) < 0.01 && fabs(scaleY - floor(scaleY)) < 0.01;
}

void RenderContext::Present()
{
	float color[] = { .0f, .0f, .0f, .0f };
	ID3D11ShaderResourceView* srvs[2];

	SetBlendState(AlphaBlend::Opaque, true);
	SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	_deviceContext->OMSetRenderTargets(1, _gammaCorrectedTextureRtv.GetAddressOf(), NULL);
	_deviceContext->ClearRenderTargetView(_gammaCorrectedTextureRtv.Get(), color);
	UpdateViewport({ 0,0,_gameSize.width, _gameSize.height });

	srvs[0] = _gameTextureSrv.Get();
	srvs[1] = _gammaTextureSrv.Get();
	SetPSShaderResourceViews(srvs);

	SetVS(_displayVS.Get());
	SetPS(_gammaPS.Get());

	auto startVertexLocation = _vbWriteIndex;
	uint32_t vertexCount = UpdateVerticesWithFullScreenQuad(_gameSize, { 0,0,_gameSize.width, _gameSize.height });

	_deviceContext->Draw(vertexCount, startVertexLocation);

	_deviceContext->RSSetState(_rasterizerStateNoScissor.Get());

	_deviceContext->OMSetRenderTargets(1, _backbufferRtv.GetAddressOf(), NULL);
	_deviceContext->ClearRenderTargetView(_backbufferRtv.Get(), color);
	UpdateViewport(_renderRect);

	const bool isIntegerScale = IsIntegerScale();

	srvs[0] = _gammaCorrectedTextureSrv.Get();
	srvs[1] = nullptr;
	SetPSShaderResourceViews(srvs);

	SetVS(_displayVS.Get());
	SetPS(isIntegerScale ? _displayIntegerScalePS.Get() : _displayNonintegerScalePS.Get());

	startVertexLocation = _vbWriteIndex;
	vertexCount = UpdateVerticesWithFullScreenQuad(_gameSize, _renderRect);

	_deviceContext->Draw(vertexCount, startVertexLocation);

	srvs[0] = nullptr;
	srvs[1] = nullptr;
	SetPSShaderResourceViews(srvs);

	switch (_syncStrategy)
	{
	case RenderContextSyncStrategy::AllowTearing:
		D2DX_RELEASE_CHECK_HR(_swapChain1->Present(0, DXGI_PRESENT_ALLOW_TEARING));
		break;
	case RenderContextSyncStrategy::Interval0:
		D2DX_RELEASE_CHECK_HR(_swapChain1->Present(0, 0));
		break;
	case RenderContextSyncStrategy::FrameLatencyWaitableObject:
		D2DX_RELEASE_CHECK_HR(_swapChain1->Present(0, 0));
		::WaitForSingleObjectEx(_frameLatencyWaitableObject, 1000, true);
		break;
	case RenderContextSyncStrategy::Interval1:
		D2DX_RELEASE_CHECK_HR(_swapChain1->Present(1, 0));
		break;
	}

	if (_deviceContext1)
	{
		_deviceContext1->DiscardView(_gameTextureSrv.Get());
		_deviceContext1->DiscardView(_gameTextureRtv.Get());
		_deviceContext1->DiscardView(_backbufferRtv.Get());
	}

	for (int32_t i = 0; i < ARRAYSIZE(_textureCaches); ++i)
	{
		_textureCaches[i]->OnNewFrame();
	}

	_deviceContext->OMSetRenderTargets(1, _gameTextureRtv.GetAddressOf(), NULL);
	UpdateViewport({ 0,0,_gameSize.width, _gameSize.height });

	_deviceContext->ClearRenderTargetView(_gameTextureRtv.Get(), color);

	_deviceContext->RSSetState(_rasterizerState.Get());

	SetVS(_gameVS.Get());
	SetPS(_gamePS.Get());
}

	void RenderContext::CreateGameTexture()
	{
		DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		UINT formatSupport;
		D2DX_RELEASE_CHECK_HR(_device->CheckFormatSupport(DXGI_FORMAT_R10G10B10A2_UNORM, &formatSupport));
		//if ((formatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D) &&
		//	(formatSupport & D3D11_FORMAT_SUPPORT_SHADER_LOAD) &&
		//	(formatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET))
		//{
		//	ALWAYS_PRINT("Using DXGI_FORMAT_R10G10B10A2_UNORM for the render buffer.");
		//	renderTargetFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
		//}
		//else
		{
			ALWAYS_PRINT("Using DXGI_FORMAT_R8G8B8A8_UNORM for the render buffer.");
		}

		CD3D11_TEXTURE2D_DESC desc{
			renderTargetFormat,
			(UINT)_desktopSize.width,
			(UINT)_desktopSize.height,
			1U,
			1U,
			D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
			D3D11_USAGE_DEFAULT
		};

		D2DX_RELEASE_CHECK_HR(_device->CreateTexture2D(&desc, NULL, &_gameTexture));
		D2DX_RELEASE_CHECK_HR(_device->CreateShaderResourceView(_gameTexture.Get(), NULL, &_gameTextureSrv));

		CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc{
			D3D11_RTV_DIMENSION_TEXTURE2D,
			renderTargetFormat
		};

		D2DX_RELEASE_CHECK_HR(_device->CreateRenderTargetView(_gameTexture.Get(), &rtvDesc, &_gameTextureRtv));

		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		D2DX_RELEASE_CHECK_HR(_device->CreateTexture2D(&desc, NULL, &_gammaCorrectedTexture));
		D2DX_RELEASE_CHECK_HR(_device->CreateShaderResourceView(_gammaCorrectedTexture.Get(), NULL, &_gammaCorrectedTextureSrv));

		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		D2DX_RELEASE_CHECK_HR(_device->CreateRenderTargetView(_gammaCorrectedTexture.Get(), &rtvDesc, &_gammaCorrectedTextureRtv));


		_deviceContext->OMSetRenderTargets(1, _gameTextureRtv.GetAddressOf(), NULL);
	}

	void RenderContext::CreateGammaTexture()
	{
		CD3D11_TEXTURE1D_DESC desc{
			DXGI_FORMAT_B8G8R8A8_UNORM,
			(UINT)256,
			1U,
			1U,
			D3D11_BIND_SHADER_RESOURCE,
			D3D11_USAGE_DEFAULT
		};

		D2DX_RELEASE_CHECK_HR(_device->CreateTexture1D(&desc, nullptr, &_gammaTexture));
		D2DX_RELEASE_CHECK_HR(_device->CreateShaderResourceView(_gammaTexture.Get(), nullptr, &_gammaTextureSrv));
	}

	void RenderContext::CreatePaletteTexture()
	{
		CD3D11_TEXTURE1D_DESC desc{
			DXGI_FORMAT_B8G8R8A8_UNORM,
			(UINT)256,
			32U,
			1U,
			D3D11_BIND_SHADER_RESOURCE,
			D3D11_USAGE_DEFAULT
		};

		D2DX_RELEASE_CHECK_HR(_device->CreateTexture1D(&desc, nullptr, &_paletteTexture));
		D2DX_RELEASE_CHECK_HR(_device->CreateShaderResourceView(_paletteTexture.Get(), nullptr, &_paletteTextureSrv));
	}

	_Use_decl_annotations_
		void RenderContext::LoadGammaTable(
			_In_reads_(valueCount) const uint32_t* values,
			_In_ uint32_t valueCount)
	{
		_deviceContext->UpdateSubresource(_gammaTexture.Get(), 0, nullptr, values, valueCount * sizeof(uint32_t), 0);
	}

	void RenderContext::CreateVideoTextures()
	{
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

		D2DX_RELEASE_CHECK_HR(_device->CreateTexture2D(&desc, NULL, &_videoTexture));
		D2DX_RELEASE_CHECK_HR(_device->CreateShaderResourceView(_videoTexture.Get(), NULL, &_videoTextureSrv));
	}

	_Use_decl_annotations_
		void RenderContext::WriteToScreen(
			const uint32_t* pixels,
			int32_t width,
			int32_t height)
	{
		D3D11_MAPPED_SUBRESOURCE ms;
		D2DX_RELEASE_CHECK_HR(_deviceContext->Map(_videoTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
		memcpy(ms.pData, pixels, width * height * 4);
		_deviceContext->Unmap(_videoTexture.Get(), 0);

		SetVS(_displayVS.Get());
		SetPS(_videoPS.Get());

		SetBlendState(AlphaBlend::Opaque, true);

		ID3D11ShaderResourceView* srvs[2] = { _videoTextureSrv.Get(), nullptr };
		SetPSShaderResourceViews(srvs);

		SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		uint32_t startVertexLocation = _vbWriteIndex;
		uint32_t vertexCount = UpdateVerticesWithFullScreenQuad(_gameSize, { 0,0,_gameSize.width, _gameSize.height });
		UpdateViewport({ 0,0,_gameSize.width, _gameSize.height });
		_deviceContext->Draw(vertexCount, startVertexLocation);

		Present();
	}

	void RenderContext::SetBlendState(
		AlphaBlend alphaBlend,
		float blendFactorAlpha)
	{
		ComPtr<ID3D11BlendState> blendState = _blendStates[(int32_t)alphaBlend];

		if (!blendState)
		{
			D3D11_BLEND_DESC blendDesc;
			ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));

			if (alphaBlend == AlphaBlend::Opaque)
			{
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_BLEND_FACTOR;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
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
			}
			else if (alphaBlend == AlphaBlend::Multiplicative)
			{
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_BLEND_FACTOR;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			}
			else if (alphaBlend == AlphaBlend::SrcAlphaInvSrcAlpha)
			{
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_BLEND_FACTOR;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			}
			else
			{
				assert(false && "Unhandled alphablend.");
			}

			D2DX_RELEASE_CHECK_HR(_device->CreateBlendState(&blendDesc, &blendState));

			_blendStates[(int32_t)alphaBlend] = blendState;
		}

		SetBlendState(blendState.Get(), blendFactorAlpha);
	}

	_Use_decl_annotations_
		void RenderContext::BulkWriteVertices(
			const Vertex* vertices,
			uint32_t vertexCount)
	{
		assert(vertexCount <= _vbCapacity);
		vertexCount = min(vertexCount, _vbCapacity);

		D3D11_MAP mapType = D3D11_MAP_WRITE_DISCARD;
		_vbWriteIndex = 0;

		D3D11_MAPPED_SUBRESOURCE mappedSubResource = { 0 };
		D2DX_RELEASE_CHECK_HR(_deviceContext->Map(_vb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource));
		Vertex* pMappedVertices = (Vertex*)mappedSubResource.pData + _vbWriteIndex;

		memcpy(pMappedVertices, vertices, sizeof(Vertex) * vertexCount);

		_deviceContext->Unmap(_vb.Get(), 0);

		_vbWriteIndex += vertexCount;
	}

	uint32_t RenderContext::UpdateVerticesWithFullScreenQuad(
		Size srcSize,
		Rect dstRect)
	{
		Vertex vertices[6] = {
			Vertex{ 0.0f, 0.0f, 0, 0, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 },
			Vertex{ (float)dstRect.size.width, 0.0f, (int16_t)srcSize.width, 0, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 },
			Vertex{ (float)dstRect.size.width, (float)dstRect.size.height, (int16_t)srcSize.width, (int16_t)srcSize.height, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 },

			Vertex{ 0.0f, 0.0f, 0, 0, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 },
			Vertex{ (float)dstRect.size.width, (float)dstRect.size.height, (int16_t)srcSize.width, (int16_t)srcSize.height, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 },
			Vertex{ 0.0f, (float)dstRect.size.height, 0, (int16_t)srcSize.height, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 }
		};

		D3D11_MAP mapType = D3D11_MAP_WRITE_NO_OVERWRITE;
		if ((_vbWriteIndex + ARRAYSIZE(vertices)) > _vbCapacity)
		{
			_vbWriteIndex = 0;
			mapType = D3D11_MAP_WRITE_DISCARD;
		}

		D3D11_MAPPED_SUBRESOURCE mappedSubResource = { 0 };
		D2DX_RELEASE_CHECK_HR(_deviceContext->Map(_vb.Get(), 0, mapType, 0, &mappedSubResource));
		Vertex* pMappedVertices = (Vertex*)mappedSubResource.pData + _vbWriteIndex;

		memcpy(pMappedVertices, vertices, sizeof(Vertex) * ARRAYSIZE(vertices));

		_deviceContext->Unmap(_vb.Get(), 0);

		_vbWriteIndex += ARRAYSIZE(vertices);

		return 6;
	}

	_Use_decl_annotations_
		TextureCacheLocation RenderContext::UpdateTexture(
			const Batch& batch,
			const uint8_t* tmuData,
			uint32_t tmuDataSize)
	{
		assert(batch.IsValid() && "Batch has no texture set.");
		const uint32_t contentKey = batch.GetHash();

		ITextureCache* atlas = GetTextureCache(batch);

		auto tcl = atlas->FindTexture(contentKey, -1);

		if (tcl._textureAtlas < 0)
		{
			tcl = atlas->InsertTexture(contentKey, batch, tmuData, tmuDataSize);
		}

		return tcl;
	}

	void RenderContext::UpdateViewport(Rect rect)
	{
		CD3D11_VIEWPORT viewport{ (float)rect.offset.x, (float)rect.offset.y, (float)rect.size.width, (float)rect.size.height };
		_deviceContext->RSSetViewports(1, &viewport);

		CD3D11_RECT scissorRect{ rect.offset.x, rect.offset.y, rect.size.width, rect.size.height };
		_deviceContext->RSSetScissorRects(1, &scissorRect);

		_constants.screenSize[0] = (float)rect.size.width;
		_constants.screenSize[1] = (float)rect.size.height;
		_constants.flags[0] = _options.noAA ? 0 : 1;
		_constants.flags[1] = 0;
		if (memcmp(&_constants, &_shadowState._constants, sizeof(Constants)) != 0)
		{
			D3D11_MAPPED_SUBRESOURCE mappedSubResource = { 0 };
			D2DX_RELEASE_CHECK_HR(_deviceContext->Map(_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource));
			memcpy(mappedSubResource.pData, &_constants, sizeof(Constants));
			_deviceContext->Unmap(_cb.Get(), 0);
			_shadowState._constants = _constants;
		}
	}

	_Use_decl_annotations_
		void RenderContext::SetPalette(
			int32_t paletteIndex,
			const uint32_t* palette)
	{
		_deviceContext->UpdateSubresource(_paletteTexture.Get(), paletteIndex, nullptr, palette, 1024, 0);
	}

	const Options& RenderContext::GetOptions() const
	{
		return _options;
	}

	LRESULT CALLBACK d2dxSubclassWndProc(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData)
	{
		RenderContext* d3d11Context = (RenderContext*)dwRefData;

		if (uMsg == WM_SYSKEYDOWN || uMsg == WM_KEYDOWN)
		{
			if (wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN))
			{
				d3d11Context->ToggleFullscreen();
				return 0;
			}
		}
		else if (uMsg == WM_DESTROY)
		{
			RemoveWindowSubclass(hWnd, d2dxSubclassWndProc, 1234);
			D2DXContextFactory::DestroyInstance();
		}
		else if (uMsg == WM_NCMOUSEMOVE)
		{
			ShowCursor_Real(TRUE);
			return 0;
		}
		else if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
		{
#ifdef NDEBUG
			ShowCursor_Real(FALSE);
#endif
			int32_t x = LOWORD(lParam);
			int32_t y = HIWORD(lParam);

			Size gameSize;
			Rect renderRect;
			Size desktopSize;

			d3d11Context->GetCurrentMetrics(&gameSize, &renderRect, &desktopSize);

			const bool isFullscreen = d3d11Context->GetOptions().screenMode == ScreenMode::FullscreenDefault;
			const float scale = (float)renderRect.size.height / gameSize.height;
			const uint32_t scaledWidth = (uint32_t)(scale * gameSize.width);
			const float mouseOffsetX = isFullscreen ? (float)(desktopSize.width / 2 - scaledWidth / 2) : 0.0f;

			x = (int32_t)(max(0, x - mouseOffsetX) / scale);
			y = (int32_t)(y / scale);

			D2DXContextFactory::GetInstance()->OnMousePosChanged({ x, y });

			lParam = x;
			lParam |= y << 16;
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	uint32_t RenderContext::DetermineMaxTextureArraySize()
	{
		HRESULT hr = E_FAIL;

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
			hr = _device->CreateTexture2D(&desc, nullptr, &tempTexture);

			if (SUCCEEDED(hr))
			{
				return arraySize;
			}

		}

		return 512;
	}

	void RenderContext::CreateTextureCaches()
	{
		static const uint32_t capacities[6] = { 2048, 2048, 2048, 2048, 2048, 2048 };

		const uint32_t texturesPerAtlas = DetermineMaxTextureArraySize();
		ALWAYS_PRINT("The device supports %u textures per atlas.", texturesPerAtlas);

		uint32_t totalSize = 0;
		for (int32_t i = 0; i < ARRAYSIZE(_textureCaches); ++i)
		{
			int32_t width = 1U << (i + 3);

			MakeAndInitialize<TextureCache, ITextureCache>(&_textureCaches[i],
				width, width, capacities[i], texturesPerAtlas, _device.Get(), _simd.Get());

			DEBUG_PRINT("Creating texture cache for %i x %i with capacity %u (%u kB).", width, width, capacities[i], _textureCaches[i]->GetMemoryFootprint() / 1024);

			totalSize += _textureCaches[i]->GetMemoryFootprint();
		}

		ALWAYS_PRINT("Total size of texture caches is %u kB.", totalSize / 1024);
	}

	_Use_decl_annotations_
		void RenderContext::SetVS(
			ID3D11VertexShader* vs)
	{
		if (vs != _shadowState._lastVS)
		{
			_deviceContext->VSSetShader(vs, NULL, 0);
			_shadowState._lastVS = vs;
		}
	}

	_Use_decl_annotations_
		void RenderContext::SetPS(
			ID3D11PixelShader* ps)
	{
		if (ps != _shadowState._lastPS)
		{
			_deviceContext->PSSetShader(ps, NULL, 0);
			_shadowState._lastPS = ps;
		}
	}

	_Use_decl_annotations_
		void RenderContext::SetBlendState(
			ID3D11BlendState* blendState,
			float blendFactorAlpha)
	{
		if (blendState != _shadowState._lastBlendState || blendFactorAlpha != _shadowState._lastBlendFactorAlpha)
		{
			float blendFactor[4] = { 0.0f, 0.0f, 0.0f, blendFactorAlpha };
			UINT sampleMask = 0xffffffff;
			_deviceContext->OMSetBlendState(blendState, blendFactor, sampleMask);
			_shadowState._lastBlendState = blendState;
		}
	}

	_Use_decl_annotations_
		void RenderContext::SetPSShaderResourceViews(
			ID3D11ShaderResourceView* srvs[2])
	{
		if (srvs[0] != _shadowState._psSrvs[0] ||
			srvs[1] != _shadowState._psSrvs[1])
		{
			_deviceContext->PSSetShaderResources(0, 2, srvs);
			_shadowState._psSrvs[0] = srvs[0];
			_shadowState._psSrvs[1] = srvs[1];
		}
	}

	void RenderContext::SetPrimitiveTopology(
		D3D11_PRIMITIVE_TOPOLOGY pt)
	{
		if (pt != _shadowState._primitiveTopology)
		{
			_deviceContext->IASetPrimitiveTopology(pt);
			_shadowState._primitiveTopology = pt;
		}
	}

#define MAX_DIMS 6

	ITextureCache* RenderContext::GetTextureCache(const Batch& batch) const
	{
		const int32_t longest = max(batch.GetWidth(), batch.GetHeight());
		assert(longest >= 8);
		uint32_t log2Longest = 0;
		BitScanForward((DWORD*)&log2Longest, (DWORD)longest);
		log2Longest -= 3;
		assert(log2Longest <= 5);
		return _textureCaches[log2Longest].Get();
	}

	_Use_decl_annotations_
		void RenderContext::AdjustWindowPlacement(
			HWND hWnd,
			bool centerOnCurrentPosition)
	{
		const int32_t desktopCenterX = _desktopSize.width / 2;
		const int32_t desktopCenterY = _desktopSize.height / 2;

		if (_options.screenMode == ScreenMode::Windowed)
		{
			RECT oldWindowRect;
			GetWindowRect(hWnd, &oldWindowRect);
			const int32_t oldWindowWidth = oldWindowRect.right - oldWindowRect.left;
			const int32_t oldWindowHeight = oldWindowRect.bottom - oldWindowRect.top;
			const int32_t oldWindowCenterX = (oldWindowRect.left + oldWindowRect.right) / 2;
			const int32_t oldWindowCenterY = (oldWindowRect.top + oldWindowRect.bottom) / 2;

			//_windowSize.width = _renderRect.size.width;
			//_windowSize.height = _renderRect.size.height;

			//if (_windowSize.height > _desktopClientMaxHeight)
			//{
			//	_windowSize.width *= (int32_t)((float)_desktopClientMaxHeight / _windowSize.height);
			//	_windowSize.height = _desktopClientMaxHeight;
			//	_renderRect.size.width = _windowSize.width;
			//	_renderRect.size.height = _windowSize.height;
			//}

			const DWORD windowStyle = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
			RECT windowRect = { 0, 0, _windowSize.width, _windowSize.height };
			AdjustWindowRect(&windowRect, windowStyle, FALSE);
			const int32_t newWindowWidth = windowRect.right - windowRect.left;
			const int32_t newWindowHeight = windowRect.bottom - windowRect.top;
			const int32_t newWindowCenterX = centerOnCurrentPosition ? oldWindowCenterX : desktopCenterX;
			const int32_t newWindowCenterY = centerOnCurrentPosition ? oldWindowCenterY : desktopCenterY;
			const int32_t newWindowX = newWindowCenterX - newWindowWidth / 2;
			const int32_t newWindowY = newWindowCenterY - newWindowHeight / 2;

			SetWindowLongPtr(hWnd, GWL_STYLE, windowStyle);
			SetWindowPos_Real(hWnd, HWND_TOP, newWindowX, newWindowY, newWindowWidth, newWindowHeight, SWP_SHOWWINDOW | SWP_NOSENDCHANGING | SWP_FRAMECHANGED);

#ifndef NDEBUG
			RECT newWindowRect;
			GetWindowRect(hWnd, &newWindowRect);
			assert(newWindowWidth == (newWindowRect.right - newWindowRect.left));
			assert(newWindowHeight == (newWindowRect.bottom - newWindowRect.top));
#endif
		}
		else if (_options.screenMode == ScreenMode::FullscreenDefault)
		{
			SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
			SetWindowPos_Real(hWnd, HWND_TOP, 0, 0, _desktopSize.width, _desktopSize.height, SWP_SHOWWINDOW | SWP_NOSENDCHANGING | SWP_FRAMECHANGED);
		}

		char newWindowText[256];
		sprintf_s(newWindowText, "Diablo II DX [%ix%i, scale %i%%]",
			_gameSize.width,
			_gameSize.height,
			(int)(((float)_renderRect.size.height / _gameSize.height) * 100.0f));
		SetWindowTextA(hWnd, newWindowText);

		ALWAYS_PRINT("Desktop size %ix%i", _desktopSize.width, _desktopSize.height);
		ALWAYS_PRINT("Window size %ix%i", _windowSize.width, _windowSize.height);
		ALWAYS_PRINT("Game size %ix%i", _gameSize.width, _gameSize.height);
		ALWAYS_PRINT("Render size %ix%i", _renderRect.size.width, _renderRect.size.height);

		ResizeBackbuffer();

		UpdateViewport({ 0,0,_gameSize.width, _gameSize.height });

		Present();
	}

	void RenderContext::ResizeBackbuffer()
	{
		if (_backbufferSizingStrategy == RenderContextBackbufferSizingStrategy::SetSourceSize)
		{
			_swapChain2->SetSourceSize(
				_renderRect.offset.x * 2 + _renderRect.size.width,
				_renderRect.offset.y * 2 + _renderRect.size.height);
		}
		else if (_backbufferSizingStrategy == RenderContextBackbufferSizingStrategy::ResizeBuffers)
		{
			ID3D11RenderTargetView* nullRtv = nullptr;
			_deviceContext->OMSetRenderTargets(1, &nullRtv, nullptr);
			_backbufferRtv = nullptr;

			D2DX_RELEASE_CHECK_HR(_swapChain1->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, _swapChainCreateFlags));

			ComPtr<ID3D11Texture2D> backbuffer;
			D2DX_RELEASE_CHECK_HR(_swapChain1->GetBuffer(0, __uuidof(ID3D11Texture2D), &backbuffer));
			D2DX_RELEASE_CHECK_HR(_device->CreateRenderTargetView(backbuffer.Get(), nullptr, &_backbufferRtv));
		}
	}

	_Use_decl_annotations_
		void RenderContext::SetSizes(
			Size gameSize,
			Size windowSize)
	{
		_gameSize = gameSize;
		_windowSize = windowSize;

		auto displaySize = _options.screenMode == ScreenMode::FullscreenDefault ? _desktopSize : _windowSize;

		_renderRect = Metrics::GetRenderRect(
			_gameSize,
			displaySize,
			!_options.noWide);

		AdjustWindowPlacement(_hWnd, true);
	}

	bool RenderContext::IsFrameLatencyWaitableObjectSupported() const
	{
		auto windowsVersion = GetWindowsVersion();
		return (windowsVersion.major == 6 && windowsVersion.minor >= 3) || windowsVersion.major > 6;
	}

	bool RenderContext::IsAllowTearingFlagSupported() const
	{
		ComPtr<IDXGIFactory4> dxgiFactory4;

		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory4))))
		{
			return false;
		}

		ComPtr<IDXGIFactory5> dxgiFactory5;

		if (FAILED(dxgiFactory4.As(&dxgiFactory5)))
		{
			return false;
		}

		BOOL allowTearing = FALSE;

		if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
		{
			return allowTearing;
		}

		return false;
	}

	void RenderContext::ToggleFullscreen()
	{
		if (_options.screenMode == ScreenMode::FullscreenDefault)
		{
			_options.screenMode = ScreenMode::Windowed;
			SetSizes(_gameSize, _windowSize);
		}
		else
		{
			_options.screenMode = ScreenMode::FullscreenDefault;
			SetSizes(_gameSize, _windowSize);
		}
	}

	_Use_decl_annotations_
		void RenderContext::GetCurrentMetrics(
			Size* gameSize,
			Rect* renderRect,
			Size* desktopSize) const
	{
		assert(gameSize && renderRect && desktopSize);
		*gameSize = _gameSize;
		*renderRect = _renderRect;
		*desktopSize = _desktopSize;
	}
