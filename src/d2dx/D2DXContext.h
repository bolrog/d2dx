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
#include "TextureHasher.h"
#include "TextMotionPredictor.h"
#include "UnitMotionPredictor.h"
#include "WeatherMotionPredictor.h"
#include "Vertex.h"

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
		
		virtual ~D2DXContext() noexcept;

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

		virtual const Options& GetOptions() const override;

		virtual bool IsFeatureEnabled(
			_In_ Feature feature) override;

#pragma endregion ID2DXContext

#pragma region IWin32InterceptionHandler
		
		virtual Offset OnSetCursorPos(
			_In_ Offset pos) override;

		virtual Offset OnMouseMoveMessage(
			_In_ Offset pos) override;

		virtual int32_t OnSleep(
			_In_ int32_t ms) override;

#pragma endregion IWin32InterceptionHandler

#pragma region ID2InterceptionHandler

		virtual Offset BeginDrawText(
			_Inout_z_ wchar_t* str,
			_In_ Offset pos,
			_In_ uint32_t returnAddress,
			_In_ D2Function d2Function) override;

		virtual void EndDrawText() override;

		virtual Offset BeginDrawImage(
			_In_ const D2::CellContextAny* cellContext,
			_In_ uint32_t drawMode,
			_In_ Offset pos,
			_In_ D2Function d2Function) override;

		virtual void EndDrawImage() override;

#pragma endregion ID2InterceptionHandler

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
		
		void EnsureReadVertexStateUpdated(
			_In_ const Batch& batch);

		struct GlideState
		{
			Buffer<uint8_t> tmuMemory{ D2DX_TMU_MEMORY_SIZE };
			Buffer<uint8_t> sideTmuMemory{ D2DX_SIDE_TMU_MEMORY_SIZE };
			Buffer<uint32_t> palettes{ D2DX_MAX_PALETTES * 256 };
			Buffer<uint32_t> gammaTable{ 256 };
			uint32_t constantColor{ 0xFFFFFFFF };
			int32_t stShift{ 0 };
		};

		struct ReadVertexState
		{
			Vertex templateVertex;
			uint32_t constantColorMask{ 0 };
			uint32_t iteratedColorMask{ 0 };
			uint32_t maskedConstantColor{ 0 };
			bool isDirty{ false };
		};

		GlideState _glideState;
		ReadVertexState _readVertexState;

		Batch _scratchBatch;

		int32_t _frame;
		std::shared_ptr<IRenderContext> _renderContext;
		std::shared_ptr<IGameHelper> _gameHelper;
		std::shared_ptr<ISimd> _simd;
		std::unique_ptr<IBuiltinResMod> _builtinResMod;
		std::shared_ptr<CompatibilityModeDisabler> _compatibilityModeDisabler;
		TextureHasher _textureHasher;
		UnitMotionPredictor _unitMotionPredictor;
		TextMotionPredictor _textMotionPredictor;
		WeatherMotionPredictor _weatherMotionPredictor;
		SurfaceIdTracker _surfaceIdTracker;

		MajorGameState _majorGameState;

		Buffer<uint32_t> _paletteKeys;

		uint32_t _batchCount;
		Buffer<Batch> _batches;

		uint32_t _vertexCount;
		Buffer<Vertex> _vertices;

		Options _options;
		Batch _logoTextureBatch;
		
		Size _customGameSize;
		Size _suggestedGameSize;

		uint32_t _lastScreenOpenMode;

		Size _gameSize;

		bool _isDrawingText = false;
		Offset _playerScreenPos = { 0,0 };

		uint32_t _lastWeatherParticleIndex = 0xFFFFFFFF;

		OffsetF _avgDir = { 0.0f, 0.0f };

		bool _areFeatureFlagsInitialized = false;
		uint32_t _featureFlags;

		bool _skipCountingSleep = false;
		int32_t _sleeps = 0;
		uint32_t _threadId = 0;
	};
}
