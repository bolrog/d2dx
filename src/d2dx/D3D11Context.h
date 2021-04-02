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

#include "TextureCache.h"
#include "Types.h"

namespace d2dx
{
	class Vertex;
	class Batch;
	class Simd;

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

		void LoadGammaTable(const uint32_t* paletteAndGamma);
		void BulkWriteVertices(const Vertex* vertices, uint32_t vertexCount);
		int32_t UpdateTexture(const Batch& batch, const uint8_t* tmuData);
		void Draw(const Batch& batch);
		void Present();
		void WriteToScreen(const uint32_t* pixels, int32_t width, int32_t height);
		void SetGamma(float red, float green, float blue);
		void SetPalette(int32_t paletteIndex, const uint32_t* palette);

		const Options& GetOptions() const;

		TextureCache* GetTextureCache(const Batch& batch) const;

		void SetSizes(int32_t gameWidth, int32_t gameHeight, int32_t renderWidth, int32_t renderHeight);
		int32_t GetGameWidth() const;
		int32_t GetGameHeight() const;
		int32_t GetWindowWidth() const;
		int32_t GetWindowHeight() const;
		int32_t GetRenderWidth() const;
		int32_t GetRenderHeight() const;

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
		void UpdateViewport(int32_t width, int32_t height);
		void SetBlendState(AlphaBlend alphaBlend);
		void UpdateConstants();
		void AdjustWindowPlacement(HWND hWnd, bool centerOnCurrentPosition);
		uint32_t UpdateVerticesWithFullScreenQuad(int32_t width, int32_t height);
		bool IsAllowTearingFlagSupported() const;
		void SetVS(ID3D11VertexShader* vs);
		void SetPS(ID3D11PixelShader* ps);
		void SetBlendState(ID3D11BlendState* blendState);
		void SetPSShaderResourceViews(ID3D11ShaderResourceView* srvs[2]);
		void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY pt);

		struct Metrics
		{
			int32_t _desktopWidth;
			int32_t _desktopHeight;
			int32_t _desktopClientMaxHeight;
			int32_t _windowWidth;
			int32_t _windowHeight;
			int32_t _renderWidth;
			int32_t _renderHeight;
			int32_t _gameWidth;
			int32_t _gameHeight;
		};

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

		bool _dxgiAllowTearingFlagSupported;
		D3D_FEATURE_LEVEL _featureLevel;
		Microsoft::WRL::ComPtr<ID3D11Device> _device;
		Microsoft::WRL::ComPtr<ID3D11Device3> _device3;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> _deviceContext;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext1> _deviceContext1;
		Microsoft::WRL::ComPtr<IDXGISwapChain> _swapChain;
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