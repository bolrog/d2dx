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

#include "Batch.h"
#include "Buffer.h"
#include "IBuiltinResMod.h"
#include "ID2DXContext.h"
#include "IGameHelper.h"
#include "IGlide3x.h"
#include "IRenderContext.h"
#include "IWin32InterceptionHandler.h"
#include "CompatibilityModeDisabler.h"
#include "SurfaceIdTracker.h"
#include "UnitMotionPredictor.h"
#include "WeatherMotionPredictor.h"

namespace d2dx
{
	class Vertex;

	class D2DXContext final :
		public ID2DXContext
	{
	public:
		D2DXContext(
			_In_ const std::shared_ptr<IGameHelper>& gameHelper,
			_In_ const std::shared_ptr<ISimd>& simd,
			_In_ const std::shared_ptr<CompatibilityModeDisabler>& compatibilityModeDisabler);
		
		virtual ~D2DXContext() noexcept {}

#pragma region IGlide3x

		virtual const char* OnGetString(
			_In_ uint32_t pname);
		
		virtual uint32_t OnGet(
			_In_ uint32_t pname,
			_In_ uint32_t plength,
			_Out_writes_(plength) int32_t* params);
		
		virtual void OnSstWinOpen(
			_In_ uint32_t hWnd,
			_In_ int32_t width,
			_In_ int32_t height);
		
		virtual void OnVertexLayout(
			_In_ uint32_t param,
			_In_ int32_t offset);
		
		virtual void OnTexDownload(
			_In_ uint32_t tmu,
			_In_reads_(width * height) const uint8_t* sourceAddress,
			_In_ uint32_t startAddress,
			_In_ int32_t width,
			_In_ int32_t height);
		
		virtual void OnTexSource(
			_In_ uint32_t tmu,
			_In_ uint32_t startAddress,
			_In_ int32_t width,
			_In_ int32_t height);
		
		virtual void OnConstantColorValue(
			_In_ uint32_t color);
		
		virtual void OnAlphaBlendFunction(
			_In_ GrAlphaBlendFnc_t rgb_sf,
			_In_ GrAlphaBlendFnc_t rgb_df,
			_In_ GrAlphaBlendFnc_t alpha_sf,
			_In_ GrAlphaBlendFnc_t alpha_df);
		
		virtual void OnColorCombine(
			_In_ GrCombineFunction_t function,
			_In_ GrCombineFactor_t factor,
			_In_ GrCombineLocal_t local,
			_In_ GrCombineOther_t other,
			_In_ bool invert);
		
		virtual void OnAlphaCombine(
			_In_ GrCombineFunction_t function,
			_In_ GrCombineFactor_t factor,
			_In_ GrCombineLocal_t local,
			_In_ GrCombineOther_t other,
			_In_ bool invert);

		virtual void OnDrawPoint(
			_In_ const void* pt,
			_In_ uint32_t gameContext);

		virtual void OnDrawLine(
			_In_ const void* v1,
			_In_ const void* v2,
			_In_ uint32_t gameContext);
		
		virtual void OnDrawVertexArray(
			_In_ uint32_t mode,
			_In_ uint32_t count,
			_In_reads_(count) uint8_t** pointers,
			_In_ uint32_t gameContext);
		
		virtual void OnDrawVertexArrayContiguous(
			_In_ uint32_t mode,
			_In_ uint32_t count,
			_In_reads_(count * stride) uint8_t* vertex,
			_In_ uint32_t stride,
			_In_ uint32_t gameContext);
		
		virtual void OnTexDownloadTable(
			_In_ GrTexTable_t type,
			_In_reads_bytes_(256 * 4) void* data);
		
		virtual void OnLoadGammaTable(
			_In_ uint32_t nentries,
			_In_reads_(nentries) uint32_t* red, 
			_In_reads_(nentries) uint32_t* green,
			_In_reads_(nentries) uint32_t* blue);
		
		virtual void OnChromakeyMode(
			_In_ GrChromakeyMode_t mode);
		
		virtual void OnLfbUnlock(
			_In_reads_bytes_(strideInBytes * 480) const uint32_t* lfbPtr,
			_In_ uint32_t strideInBytes);
		
		virtual void OnGammaCorrectionRGB(
			_In_ float red,
			_In_ float green,
			_In_ float blue);
		
		virtual void OnBufferSwap();

		virtual void OnBufferClear();

#pragma endregion IGlide3x

#pragma region ID2DXContext

		virtual void SetCustomResolution(
			_In_ Size size) override;
		
		virtual Size GetSuggestedCustomResolution() override;

		virtual GameVersion GetGameVersion() const override;

		virtual void DisableBuiltinResMod() override;

		virtual Options& GetOptions() override;

#pragma endregion ID2DXContext

#pragma region IWin32InterceptionHandler
		
		virtual Offset OnSetCursorPos(
			_In_ Offset pos) override;

		virtual Offset OnMouseMoveMessage(
			_In_ Offset pos) override;

#pragma endregion IWin32InterceptionHandler

#pragma region ID2GfxInterceptionHandler

		virtual void BeginDrawText() override;

		virtual void EndDrawText() override;

		virtual void BeginDrawImage(
			_In_ const D2::CellContext* cellContext,
			_In_ Offset pos) override;

		virtual void EndDrawImage() override;

#pragma endregion IWin32InterceptionHandler

	private:
		void CheckMajorGameState();

		void PrepareLogoTextureBatch();

		void InsertLogoOnTitleScreen();

		void DrawBatches(
			_In_ uint32_t startVertexLocation);

		const Batch PrepareBatchForSubmit(
			_In_ Batch batch,
			_In_ PrimitiveType primitiveType,
			_In_ uint32_t vertexCount,
			_In_ uint32_t gameContext) const;
		
		Vertex ReadVertex(
			_In_ const uint8_t* vertex,
			_In_ uint32_t vertexLayout,
			_In_ const Batch& batch,
			_In_ int32_t batchIndex);

		Batch _scratchBatch;

		int32_t _frame;
		std::shared_ptr<IRenderContext> _renderContext;
		MajorGameState _majorGameState;

		Buffer<uint32_t> _paletteKeys;

		Buffer<uint32_t> _gammaTable;

		uint32_t _constantColor;

		uint32_t _vertexLayout;

		Buffer<uint32_t> _textureHashCache;
		uint32_t _textureHashCacheHits;
		uint32_t _textureHashCacheMisses;

		int32_t _batchCount;
		Buffer<Batch> _batches;

		int32_t _vertexCount;
		Buffer<Vertex> _vertices;

		std::shared_ptr<IGameHelper> _gameHelper;
		std::shared_ptr<ISimd> _simd;
		std::unique_ptr<IBuiltinResMod> _builtinResMod;
		std::shared_ptr<CompatibilityModeDisabler> _compatibilityModeDisabler;

		Options _options;
		Batch _logoTextureBatch;

		Buffer<uint8_t> _tmuMemory;
		Buffer<uint8_t> _sideTmuMemory;

		Size _customGameSize;
		Size _suggestedGameSize;

		uint32_t _lastScreenOpenMode;

		SurfaceIdTracker _surfaceIdTracker;

		Buffer<uint32_t> _palettes;

		Size _gameSize;

		UnitMotionPredictor _unitMotionPredictor;

		bool _isDrawingText = false;
		Offset _playerScreenPos = { 0,0 };

		uint32_t _lastWeatherParticleIndex = 0xFFFFFFFF;

		WeatherMotionPredictor _weatherMotionPredictor;

		OffsetF _avgDir = { 0.0f, 0.0f };
	};
}
