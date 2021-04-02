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
#include "D3D11Context.h"
#include "GlideHelpers.h"
#include "GammaVS_cso.h"
#include "GammaPS_cso.h"
#include "PixelShader_cso.h"
#include "VertexShader_cso.h"
#include "VideoPS_cso.h"
#include "Utils.h"

using namespace d2dx;
using namespace std;
using namespace Microsoft::WRL;

extern unique_ptr<D2DXContext> g_d2dxContext;

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

D3D11Context::D3D11Context(
	HWND hWnd,
	int32_t renderWidth,
	int32_t renderHeight,
	int32_t gameWidth,
	int32_t gameHeight,
	Options& options,
	shared_ptr<Simd> simd,
	shared_ptr<TextureProcessor> textureProcessor) :
	_hWnd{ hWnd },
	_vbWriteIndex{ 0 },
	_vbCapacity{ 0 },
	_options{ options },
	_constants{ 0 },
	_frameLatencyWaitableObject{ nullptr },
	_simd{ simd },
	_textureProcessor{ textureProcessor }
{
	memset(&_shadowState, 0, sizeof(_shadowState));

	_metrics._desktopWidth = GetSystemMetrics(SM_CXSCREEN);
	_metrics._desktopHeight = GetSystemMetrics(SM_CYSCREEN);
	_metrics._desktopClientMaxHeight = GetSystemMetrics(SM_CYFULLSCREEN);

	_metrics._renderWidth = renderWidth;
	_metrics._renderHeight = renderHeight;
	_metrics._windowWidth = renderWidth;
	_metrics._windowHeight = renderHeight;
	_metrics._gameWidth = gameWidth;
	_metrics._gameHeight = gameHeight;

#ifndef NDEBUG
	ShowCursor_Real(TRUE);
#endif

	RECT clientRect;
	GetClientRect(hWnd, &clientRect);

	SetWindowSubclass((HWND)hWnd, d2dxSubclassWndProc, 1234, (DWORD_PTR)this);

	const int32_t widthFromClientRect = clientRect.right - clientRect.left;
	const int32_t heightFromClientRect = clientRect.bottom - clientRect.top;

	_featureLevel = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL requestFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	_dxgiAllowTearingFlagSupported = IsAllowTearingFlagSupported();

	DWORD swapChainCreateFlags = 0;

	if (_dxgiAllowTearingFlagSupported)
	{
		swapChainCreateFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}
	else
	{
		swapChainCreateFlags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Width = _metrics._desktopWidth;
	swapChainDesc.BufferDesc.Height = _metrics._desktopHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = swapChainCreateFlags;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;

#ifdef NDEBUG
	uint32_t flags = 0;
#else
	uint32_t flags = D3D11_CREATE_DEVICE_DEBUG;
#endif

	D2DX_RELEASE_CHECK_HR(D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		flags,
		&requestFeatureLevel,
		1,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&_swapChain,
		&_device,
		&_featureLevel,
		&_deviceContext));

	ComPtr<IDXGIFactory> dxgiFactory;
	_swapChain->GetParent(IID_PPV_ARGS(&dxgiFactory));
	if (dxgiFactory)
	{
		dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_WINDOW_CHANGES);
	}

	if (SUCCEEDED(_swapChain->QueryInterface(IID_PPV_ARGS(&_swapChain2))))
	{
		ALWAYS_PRINT("Swap chain supports IDXGISwapChain2.");

		if (!_dxgiAllowTearingFlagSupported)
		{
			ALWAYS_PRINT("Will sync using IDXGISwapChain2::GetFrameLatencyWaitableObject.");
			_frameLatencyWaitableObject = _swapChain2->GetFrameLatencyWaitableObject();
		}
	}
	else
	{
		ALWAYS_PRINT("Swap chain does not support IDXGISwapChain2.");
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
		if (SUCCEEDED(_swapChain->GetDevice(IID_PPV_ARGS(&dxgiDevice1))))
		{
			ALWAYS_PRINT("Setting maximum frame latency to 2.");
			D2DX_RELEASE_CHECK_HR(dxgiDevice1->SetMaximumFrameLatency(2));
		}
	}

	// get a pointer directly to the back buffer
	ComPtr<ID3D11Texture2D> backbuffer;
	D2DX_RELEASE_CHECK_HR(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backbuffer));

	// create a render target pointing to the back buffer
	D2DX_RELEASE_CHECK_HR(_device->CreateRenderTargetView(backbuffer.Get(), nullptr, &_backbufferRtv));

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
	CreateRenderTarget();

	_deviceContext->VSSetConstantBuffers(0, 1, _cb.GetAddressOf());
	_deviceContext->PSSetConstantBuffers(0, 1, _cb.GetAddressOf());

	CreateRasterizerState();

	AdjustWindowPlacement(hWnd, false);
}

D3D11Context::~D3D11Context()
{
	if (_frameLatencyWaitableObject)
	{
		CloseHandle(_frameLatencyWaitableObject);
		_frameLatencyWaitableObject = nullptr;
	}
}

void D3D11Context::CreateRasterizerState()
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

void D3D11Context::CreateVertexAndIndexBuffers()
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

void D3D11Context::CreateShadersAndInputLayout()
{
	D2DX_RELEASE_CHECK_HR(_device->CreateVertexShader(VertexShader_cso, ARRAYSIZE(VertexShader_cso), NULL, &_vs));
	D2DX_RELEASE_CHECK_HR(_device->CreatePixelShader(PixelShader_cso, ARRAYSIZE(PixelShader_cso), NULL, &_ps));
	D2DX_RELEASE_CHECK_HR(_device->CreatePixelShader(VideoPS_cso, ARRAYSIZE(VideoPS_cso), NULL, &_videoPS));
	D2DX_RELEASE_CHECK_HR(_device->CreateVertexShader(GammaVS_cso, ARRAYSIZE(GammaVS_cso), NULL, &_gammaVS));
	D2DX_RELEASE_CHECK_HR(_device->CreatePixelShader(GammaPS_cso, ARRAYSIZE(GammaPS_cso), NULL, &_gammaPS));

	D3D11_INPUT_ELEMENT_DESC inputElementDescs[4] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_SINT, 0, 4, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R16G16_UINT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	D2DX_RELEASE_CHECK_HR(_device->CreateInputLayout(inputElementDescs, ARRAYSIZE(inputElementDescs), VertexShader_cso, ARRAYSIZE(VertexShader_cso), &_inputLayout));

	_deviceContext->IASetInputLayout(_inputLayout.Get());
}

void D3D11Context::CreateConstantBuffers()
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

void D3D11Context::CreateSamplerStates()
{
	CD3D11_SAMPLER_DESC samplerDesc{ CD3D11_DEFAULT() };

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	D2DX_RELEASE_CHECK_HR(_device->CreateSamplerState(&samplerDesc, _samplerState[0].GetAddressOf()));

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	D2DX_RELEASE_CHECK_HR(_device->CreateSamplerState(&samplerDesc, _samplerState[1].GetAddressOf()));

	ID3D11SamplerState* samplerState[2] = { _samplerState[0].Get(), _samplerState[1].Get() };
	_deviceContext->PSSetSamplers(0, 2, samplerState);
}

void D3D11Context::Draw(const Batch& batch)
{
	SetVS(_vs.Get());
	SetPS(_ps.Get());

	SetBlendState(batch.GetAlphaBlend());

	TextureCache* atlas = GetTextureCache(batch);
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

void D3D11Context::Present()
{
	_deviceContext->RSSetState(_rasterizerStateNoScissor.Get());

	_deviceContext->OMSetRenderTargets(1, _backbufferRtv.GetAddressOf(), NULL);
	UpdateViewport(_metrics._renderWidth, _metrics._renderHeight);

	ID3D11ShaderResourceView* srvs[2] = { _renderTargetTextureSrv.Get(), _gammaTextureSrv.Get() };
	SetPSShaderResourceViews(srvs);

	SetVS(_gammaVS.Get());
	SetPS(_gammaPS.Get());

	SetBlendState(AlphaBlend::Opaque);

	SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto startVertexLocation = _vbWriteIndex;
	const uint32_t vertexCount = UpdateVerticesWithFullScreenQuad(_metrics._renderWidth, _metrics._renderHeight);

	_deviceContext->Draw(vertexCount, startVertexLocation);

	srvs[0] = nullptr;
	srvs[1] = nullptr;
	SetPSShaderResourceViews(srvs);

	if (_dxgiAllowTearingFlagSupported)
	{
		D2DX_RELEASE_CHECK_HR(_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	}
	else if (_frameLatencyWaitableObject)
	{
		D2DX_RELEASE_CHECK_HR(_swapChain->Present(0, 0));

		DWORD result = WaitForSingleObjectEx(
			_frameLatencyWaitableObject,
			1000, // 1 second timeout (shouldn't ever occur)
			true
		);
	}
	else
	{
		D2DX_RELEASE_CHECK_HR(_swapChain->Present(0, 0));
	}

	if (_deviceContext1)
	{
		_deviceContext1->DiscardView(_renderTargetTextureSrv.Get());
		_deviceContext1->DiscardView(_renderTargetTextureRtv.Get());
		_deviceContext1->DiscardView(_backbufferRtv.Get());
	}

	for (int32_t i = 0; i < ARRAYSIZE(_textureCaches); ++i)
	{
		_textureCaches[i]->OnNewFrame();
	}

	_deviceContext->OMSetRenderTargets(1, _renderTargetTextureRtv.GetAddressOf(), NULL);
	UpdateViewport(_metrics._renderWidth, _metrics._renderHeight);

	float color[] = { .0f, .0f, .0f, 1.0f };
	_deviceContext->ClearRenderTargetView(_renderTargetTextureRtv.Get(), color);

	_deviceContext->RSSetState(_rasterizerState.Get());

	SetVS(_vs.Get());
	SetPS(_ps.Get());
}

void D3D11Context::CreateRenderTarget()
{
	DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	UINT formatSupport;
	D2DX_RELEASE_CHECK_HR(_device->CheckFormatSupport(DXGI_FORMAT_R10G10B10A2_UNORM, &formatSupport));
	if ((formatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D) &&
		(formatSupport & D3D11_FORMAT_SUPPORT_SHADER_LOAD) &&
		(formatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET))
	{
		ALWAYS_PRINT("Using DXGI_FORMAT_R10G10B10A2_UNORM for the render buffer.");
		renderTargetFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	}
	else
	{
		ALWAYS_PRINT("Using DXGI_FORMAT_R8G8B8A8_UNORM for the render buffer.");
	}

	CD3D11_TEXTURE2D_DESC desc{
		renderTargetFormat,
		(UINT)_metrics._desktopWidth,
		(UINT)_metrics._desktopHeight,
		1U,
		1U,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		D3D11_USAGE_DEFAULT
	};

	D2DX_RELEASE_CHECK_HR(_device->CreateTexture2D(&desc, NULL, &_renderTargetTexture));
	D2DX_RELEASE_CHECK_HR(_device->CreateShaderResourceView(_renderTargetTexture.Get(), NULL, &_renderTargetTextureSrv));

	CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc{
		D3D11_RTV_DIMENSION_TEXTURE2D,
		renderTargetFormat
	};

	D2DX_RELEASE_CHECK_HR(_device->CreateRenderTargetView(_renderTargetTexture.Get(), &rtvDesc, &_renderTargetTextureRtv));

	_deviceContext->OMSetRenderTargets(1, _renderTargetTextureRtv.GetAddressOf(), NULL);
}

void D3D11Context::CreateGammaTexture()
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

void D3D11Context::CreatePaletteTexture()
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

void D3D11Context::LoadGammaTable(const uint32_t* gammaTable)
{
	_deviceContext->UpdateSubresource(_gammaTexture.Get(), 0, nullptr, gammaTable, 1024, 0);
}

void D3D11Context::CreateVideoTextures()
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

void D3D11Context::WriteToScreen(const uint32_t* pixels, int32_t width, int32_t height)
{
	D3D11_MAPPED_SUBRESOURCE ms;
	D2DX_RELEASE_CHECK_HR(_deviceContext->Map(_videoTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms));
	memcpy(ms.pData, pixels, width * height * 4);
	_deviceContext->Unmap(_videoTexture.Get(), 0);

	SetVS(_gammaVS.Get());
	SetPS(_videoPS.Get());

	SetBlendState(AlphaBlend::Opaque);

	ID3D11ShaderResourceView* srvs[2] = { _videoTextureSrv.Get(), nullptr };
	SetPSShaderResourceViews(srvs);

	SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	uint32_t startVertexLocation = _vbWriteIndex;
	uint32_t vertexCount = UpdateVerticesWithFullScreenQuad(_metrics._renderWidth, _metrics._renderHeight);

	_deviceContext->Draw(vertexCount, startVertexLocation);

	Present();
}

void D3D11Context::SetBlendState(AlphaBlend alphaBlend)
{
	ComPtr<ID3D11BlendState> blendState = _blendStates[(int32_t)alphaBlend];

	if (!blendState)
	{
		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(D3D11_BLEND_DESC));

		if (alphaBlend == AlphaBlend::Opaque)
		{
			blendDesc.RenderTarget[0].BlendEnable = FALSE;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
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
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
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
		}
		else
		{
			assert(false && "Unhandled alphablend.");
		}

		D2DX_RELEASE_CHECK_HR(_device->CreateBlendState(&blendDesc, &blendState));

		_blendStates[(int32_t)alphaBlend] = blendState;
	}

	SetBlendState(blendState.Get());
}

void D3D11Context::UpdateConstants()
{
	_constants.screenSize[0] = (float)_metrics._renderWidth;
	_constants.screenSize[1] = (float)_metrics._renderHeight;

	if (_options.screenMode == ScreenMode::FullscreenDefault)
	{
		float scale = (float)_metrics._renderHeight / _metrics._gameHeight;
		const uint32_t scaledWidth = (uint32_t)(scale * _metrics._gameWidth);
		_constants.vertexOffset[0] = (float)(_metrics._desktopWidth / 2 - scaledWidth / 2);
		_constants.vertexOffset[1] = 0.0f;
		_constants.vertexScale[0] = scale;
		_constants.vertexScale[1] = scale;
	}
	else
	{
		float scale = (float)_metrics._renderHeight / _metrics._gameHeight;
		_constants.vertexOffset[0] = 0.0f;
		_constants.vertexOffset[1] = 0.0f;
		_constants.vertexScale[0] = scale;
		_constants.vertexScale[1] = scale;
	}

	if (memcmp(&_constants, &_shadowState._constants, sizeof(Constants)) != 0)
	{
		D3D11_MAPPED_SUBRESOURCE mappedSubResource = { 0 };
		D2DX_RELEASE_CHECK_HR(_deviceContext->Map(_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource));
		memcpy(mappedSubResource.pData, &_constants, sizeof(Constants));
		_deviceContext->Unmap(_cb.Get(), 0);
		_shadowState._constants = _constants;
	}
}

void D3D11Context::BulkWriteVertices(const Vertex* vertices, uint32_t vertexCount)
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

uint32_t D3D11Context::UpdateVerticesWithFullScreenQuad(int32_t width, int32_t height)
{
	Vertex vertices[6] = {
		Vertex{ 0.0f, 0.0f, 0, 0, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 },
		Vertex{ (float)width, 0.0f, (int16_t)width, 0, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 },
		Vertex{ (float)width, (float)height, (int16_t)width, (int16_t)height, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 },

		Vertex{ 0.0f, 0.0f, 0, 0, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 },
		Vertex{ (float)width, (float)height, (int16_t)width, (int16_t)height, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 },
		Vertex{ 0.0f, (float)height, 0, (int16_t)height, 0xFFFFFFFF, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, false, 0, 0 }
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

TextureCacheLocation D3D11Context::UpdateTexture(const Batch& batch, const uint8_t* tmuData)
{
	assert(batch.IsValid() && "Batch has no texture set.");
	const uint32_t contentKey = batch.GetHash() ^ batch.GetPaletteIndex();

	TextureCache* atlas = GetTextureCache(batch);
	auto tcl = atlas->FindTexture(contentKey, -1);

	if (tcl._textureAtlas < 0)
	{
		tcl = atlas->InsertTexture(contentKey, batch, tmuData);
	}

	return tcl;
}

void D3D11Context::UpdateViewport(int32_t width, int32_t height)
{
	CD3D11_VIEWPORT viewport{ 0.0f, 0.0f, (float)width, (float)height };
	_deviceContext->RSSetViewports(1, &viewport);
}

void D3D11Context::SetGamma(float red, float green, float blue)
{
	uint32_t gammaTable[256];

	for (int32_t i = 0; i < 256; ++i)
	{
		float v = i / 255.0f;
		float r = powf(v, 1.0f / red);
		float g = powf(v, 1.0f / green);
		float b = powf(v, 1.0f / blue);
		uint32_t ri = (uint32_t)(r * 255.0f);
		uint32_t gi = (uint32_t)(g * 255.0f);
		uint32_t bi = (uint32_t)(b * 255.0f);
		gammaTable[i] = (ri << 16) | (gi << 8) | bi;
	}

	LoadGammaTable(gammaTable);
}

void D3D11Context::SetPalette(int32_t paletteIndex, const uint32_t* palette)
{
	_deviceContext->UpdateSubresource(_paletteTexture.Get(), paletteIndex, nullptr, palette, 1024, 0);
}

const Options& D3D11Context::GetOptions() const
{
	return _options;
}

LRESULT CALLBACK d2dxSubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	D3D11Context* d3d11Context = (D3D11Context*)dwRefData;

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
		g_d2dxContext = nullptr;
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
		uint32_t x = LOWORD(lParam);
		uint32_t y = HIWORD(lParam);

		const D3D11Context::Metrics& metrics = d3d11Context->GetMetrics();

		const bool isFullscreen = d3d11Context->GetOptions().screenMode == ScreenMode::FullscreenDefault;
		const float scale = (float)metrics._renderHeight / metrics._gameHeight;
		const uint32_t scaledWidth = (uint32_t)(scale * metrics._gameWidth);
		const float mouseOffsetX = isFullscreen ? (float)(metrics._desktopWidth / 2 - scaledWidth / 2) : 0.0f;

		x = (uint32_t)(max(0, x - mouseOffsetX) / scale);
		y = (uint32_t)(y / scale);

		g_d2dxContext->OnMousePosChanged(x, y);

		lParam = x;
		lParam |= y << 16;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

uint32_t D3D11Context::DetermineMaxTextureArraySize()
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

	} while (FAILED(hr));

	return 512;
}

void D3D11Context::CreateTextureCaches()
{
	static const uint32_t capacities[6] = { 2048, 2048, 2048, 2048, 2048, 2048 };

	const uint32_t texturesPerAtlas = DetermineMaxTextureArraySize();
	ALWAYS_PRINT("The device supports %u textures per atlas.", texturesPerAtlas);

	uint32_t totalSize = 0;
	for (int32_t i = 0; i < ARRAYSIZE(_textureCaches); ++i)
	{
		int32_t width = 1U << (i + 3);

		_textureCaches[i] = make_unique<TextureCache>(width, width, capacities[i], texturesPerAtlas, _device.Get(), _simd, _textureProcessor);

		DEBUG_PRINT("Creating texture cache for %i x %i with capacity %u (%u kB).", width, width, capacities[i], _textureCaches[i]->GetMemoryFootprint() / 1024);

		totalSize += _textureCaches[i]->GetMemoryFootprint();
	}

	ALWAYS_PRINT("Total size of texture caches is %u kB.", totalSize / 1024);
}

void D3D11Context::SetVS(ID3D11VertexShader* vs)
{
	if (vs != _shadowState._lastVS)
	{
		_deviceContext->VSSetShader(vs, NULL, 0);
		_shadowState._lastVS = vs;
	}
}

void D3D11Context::SetPS(ID3D11PixelShader* ps)
{
	if (ps != _shadowState._lastPS)
	{
		_deviceContext->PSSetShader(ps, NULL, 0);
		_shadowState._lastPS = ps;
	}
}

void D3D11Context::SetBlendState(ID3D11BlendState* blendState)
{
	if (blendState != _shadowState._lastBlendState)
	{
		float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		UINT sampleMask = 0xffffffff;
		_deviceContext->OMSetBlendState(blendState, blendFactor, sampleMask);
		_shadowState._lastBlendState = blendState;
	}
}

void D3D11Context::SetPSShaderResourceViews(ID3D11ShaderResourceView* srvs[2])
{
	if (srvs[0] != _shadowState._psSrvs[0] ||
		srvs[1] != _shadowState._psSrvs[1])
	{
		_deviceContext->PSSetShaderResources(0, 2, srvs);
		_shadowState._psSrvs[0] = srvs[0];
		_shadowState._psSrvs[1] = srvs[1];
	}
}

void D3D11Context::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY pt)
{
	if (pt != _shadowState._primitiveTopology)
	{
		_deviceContext->IASetPrimitiveTopology(pt);
		_shadowState._primitiveTopology = pt;
	}
}

#define MAX_DIMS 6

TextureCache* D3D11Context::GetTextureCache(const Batch& batch) const
{
	const int32_t longest = max(batch.GetWidth(), batch.GetHeight());
	assert(longest >= 8);
	uint32_t log2Longest = 0;
	BitScanForward((DWORD*)&log2Longest, (DWORD)longest);
	log2Longest -= 3;
	assert(log2Longest <= 5);
	return _textureCaches[log2Longest].get();
}

void D3D11Context::AdjustWindowPlacement(HWND hWnd, bool centerOnCurrentPosition)
{
	const int32_t desktopCenterX = _metrics._desktopWidth / 2;
	const int32_t desktopCenterY = _metrics._desktopHeight / 2;

	if (_options.screenMode == ScreenMode::Windowed)
	{
		RECT oldWindowRect;
		RECT oldWindowClientRect;
		GetWindowRect(hWnd, &oldWindowRect);
		GetClientRect(hWnd, &oldWindowClientRect);
		const int32_t oldWindowWidth = oldWindowRect.right - oldWindowRect.left;
		const int32_t oldWindowHeight = oldWindowRect.bottom - oldWindowRect.top;
		const int32_t oldWindowClientWidth = oldWindowClientRect.right - oldWindowClientRect.left;
		const int32_t oldWindowClientHeight = oldWindowClientRect.bottom - oldWindowClientRect.top;
		const int32_t oldWindowCenterX = (oldWindowRect.left + oldWindowRect.right) / 2;
		const int32_t oldWindowCenterY = (oldWindowRect.top + oldWindowRect.bottom) / 2;

		_metrics._windowWidth = _metrics._renderWidth;
		_metrics._windowHeight = _metrics._renderHeight;

		if (_metrics._windowHeight > _metrics._desktopClientMaxHeight)
		{
			_metrics._windowWidth *= (int32_t)((float)_metrics._desktopClientMaxHeight / _metrics._windowHeight);
			_metrics._windowHeight = _metrics._desktopClientMaxHeight;
			_metrics._renderWidth = _metrics._windowWidth;
			_metrics._renderHeight = _metrics._windowHeight;
		}

		const int32_t newWindowWidth = (oldWindowWidth - oldWindowClientWidth) + _metrics._windowWidth;
		const int32_t newWindowHeight = (oldWindowHeight - oldWindowClientHeight) + _metrics._windowHeight;
		const int32_t newWindowCenterX = centerOnCurrentPosition ? oldWindowCenterX : desktopCenterX;
		const int32_t newWindowCenterY = centerOnCurrentPosition ? oldWindowCenterY : desktopCenterY;
		const int32_t newWindowX = newWindowCenterX - newWindowWidth / 2;
		const int32_t newWindowY = newWindowCenterY - newWindowHeight / 2;

		SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPEDWINDOW);
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
		SetWindowPos_Real(hWnd, HWND_TOP, 0, 0, _metrics._desktopWidth, _metrics._desktopHeight, SWP_SHOWWINDOW | SWP_NOSENDCHANGING | SWP_FRAMECHANGED);

		_metrics._renderWidth = _metrics._desktopWidth;
		_metrics._renderHeight = _metrics._desktopHeight;
	}

	char newWindowText[256];
	sprintf_s(newWindowText, "Diablo II DX [%ix%i, scale %i%%]",
		_metrics._gameWidth, _metrics._gameHeight, (int)(((float)_metrics._renderHeight / _metrics._gameHeight) * 100.0f));
	SetWindowTextA(hWnd, newWindowText);

	ALWAYS_PRINT("Desktop size %ix%i", _metrics._desktopWidth, _metrics._desktopHeight);
	ALWAYS_PRINT("Window size %ix%i", _metrics._windowWidth, _metrics._windowHeight);
	ALWAYS_PRINT("Game size %ix%i", _metrics._gameWidth, _metrics._gameHeight);
	ALWAYS_PRINT("Render size %ix%i", _metrics._renderWidth, _metrics._renderHeight);

	UpdateConstants();

	if (_swapChain2)
	{
		_swapChain2->SetSourceSize(_metrics._renderWidth, _metrics._renderHeight);
	}

	CD3D11_RECT scissorRect{ (LONG)_constants.vertexOffset[0], 0, _metrics._renderWidth - (LONG)_constants.vertexOffset[0], _metrics._renderHeight };
	_deviceContext->RSSetScissorRects(1, &scissorRect);

	UpdateViewport(_metrics._renderWidth, _metrics._renderHeight);

	Present();
}

void D3D11Context::SetSizes(int32_t gameWidth, int32_t gameHeight, int32_t renderWidth, int32_t renderHeight)
{
	_metrics._gameWidth = gameWidth;
	_metrics._gameHeight = gameHeight;

	if (renderWidth > 0 && renderHeight > 0)
	{
		_metrics._renderWidth = renderWidth;
		_metrics._renderHeight = renderHeight;
	}

	if (_metrics._gameWidth > _metrics._renderWidth || _metrics._gameHeight > _metrics._renderHeight)
	{
		_metrics._renderWidth = _metrics._gameWidth;
		_metrics._renderHeight = _metrics._gameHeight;
	}

	AdjustWindowPlacement(_hWnd, true);
}

bool D3D11Context::IsAllowTearingFlagSupported() const
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

void D3D11Context::ToggleFullscreen()
{
	if (_options.screenMode == ScreenMode::FullscreenDefault)
	{
		_options.screenMode = ScreenMode::Windowed;
		SetSizes(_metrics._gameWidth, _metrics._gameHeight, _metrics._windowWidth, _metrics._windowHeight);
	}
	else
	{
		_options.screenMode = ScreenMode::FullscreenDefault;
		SetSizes(_metrics._gameWidth, _metrics._gameHeight, _metrics._desktopWidth, _metrics._desktopHeight);
	}
}

const D3D11Context::Metrics& D3D11Context::GetMetrics() const
{
	return _metrics;
}
