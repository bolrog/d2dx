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
#include "Types.h"

namespace d2dx
{
	enum class RenderContextSamplerState
	{
		Point = 0,
		Bilinear = 1,
		Count = 2
	};

	enum class RenderContextVertexShader
	{
		Game = 0,
		Display = 1,
		Count = 2
	};

	enum class RenderContextPixelShader
	{
		Game = 0,
		Gamma = 1,
		Video = 2,
		DisplayIntegerScale = 3,
		DisplayNonintegerScale = 4,
		DisplayBilinearScale = 5,
		DisplayCatmullRomScale = 6,
		ResolveAA = 7,
		Count = 8
	};

	enum class RenderContextTexture1D
	{
		Palette = 0,
		GammaTable = 1,
		Count = 2,
	};

	enum class RenderContextFramebuffer
	{
		Game = 0,
		GammaCorrected = 1,
		SurfaceId = 2,
		Count = 3,
	};

	class RenderContextResources final
	{
	public:
		RenderContextResources(
			_In_ uint32_t vbSizeBytes,
			_In_ uint32_t cbSizeBytes,
			_In_ Size framebufferSize,
			_In_ ID3D11Device* device,
			_In_ const std::shared_ptr<ISimd>& simd);
		
		virtual ~RenderContextResources() noexcept {}

		void OnNewFrame();

		void SetFramebufferSize(Size framebufferSize, ID3D11Device* device);

		ID3D11InputLayout* GetInputLayout() const { return _inputLayout.Get(); }

		ID3D11VertexShader* GetVertexShader(RenderContextVertexShader vertexShader) const
		{
			return _vertexShaders[(int32_t)vertexShader].Get();
		}

		ID3D11PixelShader* GetPixelShader(RenderContextPixelShader pixelShader) const 
		{
			return _pixelShaders[(int32_t)pixelShader].Get();
		}

		ITextureCache* GetTextureCache(
			int32_t textureWidth, 
			int32_t textureHeight) const;

		ID3D11Texture1D* GetTexture1D(RenderContextTexture1D texture1d) const
		{ 
			return _texture1Ds[(int32_t)texture1d].texture.Get();
		}
		
		ID3D11ShaderResourceView* GetTexture1DSrv(RenderContextTexture1D texture1d) const
		{
			return _texture1Ds[(int32_t)texture1d].srv.Get();
		}

		Size GetVideoTextureSize() const
		{
			return _videoTextureSize;
		}
		
		ID3D11Texture2D* GetVideoTexture() const
		{
			return _videoTexture.Get();
		}
		
		ID3D11ShaderResourceView* GetVideoSrv() const
		{
			return _videoTextureSrv.Get();
		}

		Size GetCinematicTextureSize() const
		{
			return _cinematicTextureSize;
		}

		ID3D11Texture2D* GetCinematicTexture() const
		{
			return _cinematicTexture.Get();
		}

		ID3D11ShaderResourceView* GetCinematicSrv() const
		{
			return _cinematicTextureSrv.Get();
		}

		ID3D11RasterizerState* GetRasterizerState(bool enableScissor) const
		{
			return enableScissor ? _rasterizerState.Get() : _rasterizerStateNoScissor.Get();
		}

		ID3D11SamplerState* GetSamplerState(RenderContextSamplerState samplerState) const
		{
			return _samplerState[(int32_t)samplerState].Get();
		}

		ID3D11BlendState* GetBlendState(AlphaBlend alphaBlend) const
		{
			return _blendStates[(int32_t)alphaBlend].Get();
		}

		Size GetFramebufferSize() const
		{
			return _framebufferSize;
		}

		ID3D11Texture2D* GetFramebufferTexture(RenderContextFramebuffer fb) const
		{
			return _framebuffers[(int32_t)fb].texture.Get();
		}

		ID3D11ShaderResourceView* GetFramebufferSrv(RenderContextFramebuffer fb) const
		{
			return _framebuffers[(int32_t)fb].srv.Get();
		}

		ID3D11RenderTargetView* GetFramebufferRtv(RenderContextFramebuffer fb) const
		{
			return _framebuffers[(int32_t)fb].rtv.Get();
		}

		ID3D11Buffer* GetVertexBuffer() const
		{
			return _vb.Get();
		}

		ID3D11Buffer* GetConstantBuffer() const
		{
			return _cb.Get();
		}

	private:
		void CreateRasterizerState(
			_In_ ID3D11Device* device);

		void CreateShadersAndInputLayout(
			_In_ ID3D11Device* device);

		void CreateTexture1Ds(
			_In_ ID3D11Device* device);

		uint32_t DetermineMaxTextureArraySize(
			_In_ ID3D11Device* device);

		void CreateTextureCaches(
			_In_ ID3D11Device* device,
			_In_ const std::shared_ptr<ISimd>& simd);
	
		void CreateVideoTextures(
			_In_ ID3D11Device* device);

		void CreateSamplerStates(
			_In_ ID3D11Device* device);

		void CreateBlendStates(
			_In_ ID3D11Device* device);

		void CreateFramebuffers(
			_In_ Size framebufferSize,
			_In_ ID3D11Device* device);

		void CreateVertexBuffer(
			_In_ uint32_t vbSizeBytes,
			_In_ ID3D11Device* device);

		void CreateConstantBuffer(
			_In_ uint32_t cbSizeBytes,
			_In_ ID3D11Device* device);

		ComPtr<ID3D11InputLayout> _inputLayout;
		ComPtr<ID3D11VertexShader> _vertexShaders[(int32_t)RenderContextVertexShader::Count];
		ComPtr<ID3D11PixelShader> _pixelShaders[(int32_t)RenderContextPixelShader::Count];
		ComPtr<ID3D11PixelShader> _gammaPS;
		ComPtr<ID3D11PixelShader> _resolveAAPS;

		struct
		{
			ComPtr<ID3D11Texture1D> texture;
			ComPtr<ID3D11ShaderResourceView> srv;
		
		} _texture1Ds[(int32_t)RenderContextTexture1D::Count];

		Size _videoTextureSize;
		ComPtr<ID3D11Texture2D> _videoTexture;
		ComPtr<ID3D11ShaderResourceView> _videoTextureSrv;

		Size _cinematicTextureSize;
		ComPtr<ID3D11Texture2D> _cinematicTexture;
		ComPtr<ID3D11ShaderResourceView> _cinematicTextureSrv;

		std::unique_ptr<ITextureCache> _textureCaches[7];

		ComPtr<ID3D11RasterizerState> _rasterizerStateNoScissor;
		ComPtr<ID3D11RasterizerState> _rasterizerState;

		ComPtr<ID3D11SamplerState> _samplerState[2];
	
		ComPtr<ID3D11BlendState> _blendStates[(int32_t)AlphaBlend::Count];

		struct {
			ComPtr<ID3D11Texture2D> texture;
			ComPtr<ID3D11RenderTargetView> rtv;
			ComPtr<ID3D11ShaderResourceView> srv;
		
		} _framebuffers[(int32_t)RenderContextFramebuffer::Count];

		Size _framebufferSize;

		ComPtr<ID3D11Buffer> _vb;
		ComPtr<ID3D11Buffer> _cb;
	};
}
