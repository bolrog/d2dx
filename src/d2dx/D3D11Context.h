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

#include "TextureCache.h"
#include "Types.h"

namespace d2dx
{
	class Vertex;
	class Batch;
	class Simd;

	enum class D3D11SyncStrategy
	{
		AllowTearing = 0,
		Interval0 = 1,
		FrameLatencyWaitableObject = 2,
		Interval1 = 3,
	};

	enum class D3D11SwapStrategy
	{
		FlipDiscard = 0,
		Discard = 1,
	};

	enum class D3D11BackbufferSizingStrategy
	{
		SetSourceSize = 0,
		ResizeBuffers = 1,
	};

	class D3D11Context final
	{
	public:
		D3D11Context(
			HWND hWnd, 
			int32_t windowWidth,
			int32_t windowHeight,
			int32_t gameWidth,
			int32_t gameHeight,
			Options& options,
			std::shared_ptr<Simd> simd,
			std::shared_ptr<TextureProcessor> textureProcessor);
		
		~D3D11Context();

		void LoadGammaTable(
			_In_reads_(256) const uint32_t* gammaTable);
		
		void BulkWriteVertices(
			_In_reads_(vertexCount) const Vertex* vertices,
			uint32_t vertexCount);
		
		TextureCacheLocation UpdateTexture(
			const Batch& batch,
			_In_reads_(tmuDataSize) const uint8_t* tmuData,
			uint32_t tmuDataSize);
		
		void Draw(
			const Batch& batch);
		
		void Present();
		
		void WriteToScreen(
			_In_reads_(width * height) const uint32_t* pixels,
			int32_t width,
			int32_t height);
		
		void SetGamma(
			float red, 
			float green, 
			float blue);
		
		void SetPalette(
			int32_t paletteIndex, 
			_In_reads_(256) const uint32_t* palette);

		const Options& GetOptions() const;

		TextureCache* GetTextureCache(const Batch& batch) const;

		void SetSizes(int32_t gameWidth, int32_t gameHeight, int32_t renderWidth, int32_t renderHeight);

		struct Metrics
		{
			int32_t desktopWidth;
			int32_t desktopHeight;
			int32_t desktopClientMaxHeight;
			int32_t windowWidth;
			int32_t windowHeight;
			int32_t renderWidth;
			int32_t renderHeight;
			int32_t gameWidth;
			int32_t gameHeight;
		};
		
		const Metrics& GetMetrics() const;

		void ToggleFullscreen();

	private:
		void CreateRasterizerState();
		void CreateVertexAndIndexBuffers();
		void CreateShadersAndInputLayout();
		void CreateConstantBuffers();
		void CreateGammaTexture();
		void CreatePaletteTexture();
		void CreateVideoTextures();
		void CreateRenderTarget();
		void CreateSamplerStates();
		void CreateTextureCaches();
		uint32_t DetermineMaxTextureArraySize();
		
		void UpdateViewport(
			int32_t width, 
			int32_t height);
		
		void SetBlendState(AlphaBlend alphaBlend);
		
		void UpdateConstants();
		
		void AdjustWindowPlacement(
			_In_ HWND hWnd,
			bool centerOnCurrentPosition);
		
		uint32_t UpdateVerticesWithFullScreenQuad(
			int32_t width,
			int32_t height);
		
		bool IsFrameLatencyWaitableObjectSupported() const;
		
		bool IsAllowTearingFlagSupported() const;

		void ResizeBackbuffer();

		void SetVS(
			_In_ ID3D11VertexShader* vs);
		void SetPS(
			_In_ ID3D11PixelShader* ps);
		
		void SetBlendState(
			_In_ ID3D11BlendState* blendState);
		
		void SetPSShaderResourceViews(
			_In_ ID3D11ShaderResourceView* srvs[2]);
		
		void SetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY pt);

		Metrics _metrics;

		struct Constants
		{
			float vertexOffset[2];
			float vertexScale[2];
			float screenSize[2];
			float dummy[2];
		};

		static_assert(sizeof(Constants) == 8 * 4, "size of Constants");

		uint32_t _vbWriteIndex;
		uint32_t _vbCapacity;

		Constants _constants;

		D3D11SyncStrategy _syncStrategy;
		D3D11SwapStrategy _swapStrategy;
		D3D11BackbufferSizingStrategy _backbufferSizingStrategy;

		DWORD _swapChainCreateFlags;
		bool _dxgiAllowTearingFlagSupported;
		bool _frameLatencyWaitableObjectSupported;
		D3D_FEATURE_LEVEL _featureLevel;
		Microsoft::WRL::ComPtr<ID3D11Device> _device;
		Microsoft::WRL::ComPtr<ID3D11Device3> _device3;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> _deviceContext;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext1> _deviceContext1;
		Microsoft::WRL::ComPtr<IDXGISwapChain1> _swapChain1;
		Microsoft::WRL::ComPtr<IDXGISwapChain2> _swapChain2;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> _rasterizerStateNoScissor;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState> _rasterizerState;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> _inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> _vb;
		Microsoft::WRL::ComPtr<ID3D11Buffer> _cb;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> _vs;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> _ps;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> _videoPS;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> _gammaVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> _gammaPS;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _backbufferRtv;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> _samplerState[2];

		Microsoft::WRL::ComPtr<ID3D11Texture2D> _videoTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _videoTextureSrv;

		Microsoft::WRL::ComPtr<ID3D11BlendState> _blendStates[(int32_t)AlphaBlend::Count];

		Microsoft::WRL::ComPtr<ID3D11Texture1D> _gammaTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _gammaTextureSrv;

		Microsoft::WRL::ComPtr<ID3D11Texture1D> _paletteTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _paletteTextureSrv;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> _renderTargetTexture;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _renderTargetTextureRtv;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _renderTargetTextureSrv;

		std::unique_ptr<TextureCache> _textureCaches[6];

		HWND _hWnd;
		Options& _options;

		struct
		{
			ID3D11VertexShader* _lastVS;
			ID3D11PixelShader* _lastPS;
			ID3D11BlendState* _lastBlendState;
			ID3D11ShaderResourceView* _psSrvs[2];
			D3D11_PRIMITIVE_TOPOLOGY _primitiveTopology;
			Constants _constants;
		} _shadowState;

		HANDLE _frameLatencyWaitableObject;
		std::shared_ptr<Simd> _simd;
		std::shared_ptr<TextureProcessor> _textureProcessor;
	};
}