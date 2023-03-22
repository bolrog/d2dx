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

#include "IRenderContext.h"
#include "ISimd.h"
#include "ITextureCache.h"
#include "RenderContextResources.h"
#include "Types.h"

namespace d2dx
{
	class Vertex;
	class Batch;

	enum class RenderContextSyncStrategy
	{
		AllowTearing = 0,
		Interval0 = 1,
		FrameLatencyWaitableObject = 2,
		Interval1 = 3,
	};

	enum class RenderContextSwapStrategy
	{
		FlipDiscard = 0,
		Discard = 1,
	};

	enum class RenderContextBackbufferSizingStrategy
	{
		SetSourceSize = 0,
		ResizeBuffers = 1,
	};

	class RenderContext final : public IRenderContext
	{
	public:
		RenderContext(
			_In_ HWND hWnd,
			_In_ Size gameSize,
			_In_ Size windowSize,
			_In_ ScreenMode initialScreenMode,
			_In_ ID2DXContext* d2dxContext,
			_In_ const std::shared_ptr<ISimd>& simd);
		
		virtual ~RenderContext() noexcept {}

		virtual HWND GetHWnd() const override;

		virtual void LoadGammaTable(
			_In_reads_(valueCount) const uint32_t* values,
			_In_ uint32_t valueCount) override;

		virtual uint32_t BulkWriteVertices(
			_In_reads_(vertexCount) const Vertex* vertices,
			_In_ uint32_t vertexCount) override;

		virtual TextureCacheLocation UpdateTexture(
			_In_ const Batch& batch,
			_In_reads_(tmuDataSize) const uint8_t* tmuData,
			_In_ uint32_t tmuDataSize) override;

		virtual void Draw(
			_In_ const Batch& batch,
			_In_ uint32_t startVertexLocation) override;

		virtual void Present() override;

		virtual void WriteToScreen(
			_In_reads_(width* height) const uint32_t* pixels,
			_In_ int32_t width,
			_In_ int32_t height,
			_In_ bool forCinematic) override;

		virtual void SetPalette(
			_In_ int32_t paletteIndex,
			_In_reads_(256) const uint32_t* palette) override;

		virtual const Options& GetOptions() const override;

		virtual ITextureCache* GetTextureCache(
			_In_ const Batch& batch) const override;

		virtual void SetSizes(
			_In_ Size gameSize,
			_In_ Size windowSize,
			_In_ ScreenMode screenMode) override;

		virtual void GetCurrentMetrics(
			_Out_opt_ Size* gameSize,
			_Out_opt_ Rect* renderRect,
			_Out_opt_ Size* desktopSize) const override;

		virtual void ToggleFullscreen() override;

		virtual float GetFrameTime() const override;
		virtual int32_t GetFrameTimeFp() const override;

		virtual ScreenMode GetScreenMode() const override;

		void ClipCursor();
		void UnclipCursor();

	private:
		bool IsIntegerScale() const;

		void UpdateViewport(
			_In_ Rect rect);

		void SetRenderTargets(
			_In_opt_ ID3D11RenderTargetView* rtv0,
			_In_opt_ ID3D11RenderTargetView* rtv1);

		void SetRasterizerState(
			_In_ ID3D11RasterizerState* rasterizerState);

		void SetBlendState(
			_In_ AlphaBlend alphaBlend);

		void AdjustWindowPlacement(
			_In_ HWND hWnd);

		uint32_t UpdateVerticesWithFullScreenTriangle(
			_In_ Size srcSize,
			_In_ Size srcTextureSize,
			_In_ Rect dstRect);

		bool IsFrameLatencyWaitableObjectSupported() const;

		bool IsAllowTearingFlagSupported() const;

		void ResizeBackbuffer();

		void SetShaderState(
			_In_opt_ ID3D11VertexShader* vs,
			_In_opt_ ID3D11PixelShader* ps,
			_In_opt_ ID3D11ShaderResourceView* psSrv0,
			_In_opt_ ID3D11ShaderResourceView* psSrv1);

		void SetBlendState(
			_In_ ID3D11BlendState* blendState);

		struct Constants final
		{
			float screenSize[2] = { 0.0f, 0.0f };
			float invScreenSize[2] = { 0.0f, 0.0f };
			uint32_t flags[4] = { 0, 0, 0, 0 };
		};

		static_assert(sizeof(Constants) == 8 * 4, "size of Constants");

		struct DeviceContextState final
		{
			Constants constants;
			ID3D11RasterizerState* rs = nullptr;
			ID3D11VertexShader* vs = nullptr;
			ID3D11PixelShader* ps = nullptr;
			ID3D11BlendState* bs = nullptr;
			ID3D11ShaderResourceView* psSrv0 = nullptr;
			ID3D11ShaderResourceView* psSrv1 = nullptr;
			ID3D11RenderTargetView* rtv0 = nullptr;
			ID3D11RenderTargetView* rtv1 = nullptr;
		};

		ScreenMode _screenMode = ScreenMode::Windowed;
		ComPtr<ID3D11Device> _device;
		ComPtr<ID3D11Device3> _device3;
		ComPtr<ID3D11DeviceContext> _deviceContext;
		ComPtr<ID3D11DeviceContext1> _deviceContext1;
		ComPtr<IDXGISwapChain1> _swapChain1;
		ComPtr<IDXGISwapChain2> _swapChain2;
		ComPtr<ID3D11RenderTargetView> _backbufferRtv;
		std::unique_ptr<RenderContextResources> _resources;
		std::shared_ptr<ISimd> _simd;

		uint32_t _frameCount = 0;
		Size _gameSize = { 0, 0 };
		Rect _renderRect = { 0,0,0,0 };
		Size _windowSize = { 0,0 };
		Size _desktopSize = { 0,0 };
		int32_t _desktopClientMaxHeight = 0;
		uint32_t _vbWriteIndex = 0;
		uint32_t _vbCapacity = 0;
		Constants _constants;
		RenderContextSyncStrategy _syncStrategy = RenderContextSyncStrategy::AllowTearing;
		RenderContextSwapStrategy _swapStrategy = RenderContextSwapStrategy::FlipDiscard;
		RenderContextBackbufferSizingStrategy _backbufferSizingStrategy = RenderContextBackbufferSizingStrategy::SetSourceSize;
		DWORD _swapChainCreateFlags = 0;
		bool _dxgiAllowTearingFlagSupported = false;
		bool _frameLatencyWaitableObjectSupported = false;
		D3D_FEATURE_LEVEL _featureLevel = D3D_FEATURE_LEVEL_11_0;
		HWND _hWnd = nullptr;
		ID2DXContext* _d2dxContext = nullptr;
		DeviceContextState _shadowState;
		EventHandle _frameLatencyWaitableObject;
		int64_t _timeStart;
		bool _hasAdjustedWindowPlacement = false;

		double _prevTime;
		double _frameTimeMs;
	};
}