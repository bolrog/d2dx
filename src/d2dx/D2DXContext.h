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

#include "D3D11Context.h"
#include "GameHelper.h"
#include "Types.h"
#include "Batch.h"
#include "Vertex.h"

namespace d2dx
{
	class D2DXContext final
	{
	public:
		static D2DXContext* Instance();
		static void Destroy();

		D2DXContext();
		~D2DXContext();

		bool IsCapturingFrame() const;
		
		bool IsDrawingDisabled() const;

		void OnGlideInit();

		void OnGlideShutdown();

		const char* OnGetString(
			uint32_t pname);
		
		uint32_t OnGet(
			uint32_t pname,
			uint32_t plength,
			_Out_writes_(plength) int32_t* params);
		
		void OnSstWinOpen(
			uint32_t hWnd,
			int32_t width,
			int32_t height);
		
		void OnVertexLayout(
			uint32_t param,
			int32_t offset);
		
		void OnTexDownload(
			uint32_t tmu,
			_In_reads_(width * height) const uint8_t* sourceAddress,
			uint32_t startAddress,
			int32_t width,
			int32_t height);
		
		void OnTexSource(
			uint32_t tmu,
			uint32_t startAddress,
			int32_t width,
			int32_t height);
		
		void OnConstantColorValue(
			uint32_t color);
		
		void OnAlphaBlendFunction(
			GrAlphaBlendFnc_t rgb_sf,
			GrAlphaBlendFnc_t rgb_df,
			GrAlphaBlendFnc_t alpha_sf,
			GrAlphaBlendFnc_t alpha_df);
		
		void OnColorCombine(
			GrCombineFunction_t function,
			GrCombineFactor_t factor,
			GrCombineLocal_t local,
			GrCombineOther_t other,
			bool invert);
		
		void OnAlphaCombine(
			GrCombineFunction_t function,
			GrCombineFactor_t factor,
			GrCombineLocal_t local,
			GrCombineOther_t other,
			bool invert);
		
		void OnDrawLine(
			_In_ const void* v1,
			_In_ const void* v2,
			uint32_t gameContext);
		
		void OnDrawVertexArray(
			uint32_t mode,
			uint32_t count,
			_In_reads_(count) uint8_t** pointers,
			uint32_t gameContext);
		
		void OnDrawVertexArrayContiguous(
			uint32_t mode,
			uint32_t count,
			_In_reads_(count * stride) uint8_t* vertex,
			uint32_t stride,
			uint32_t gameContext);
		
		void OnTexDownloadTable(
			GrTexTable_t type,
			_In_reads_bytes_(256 * 4) void* data);
		
		void OnLoadGammaTable(
			uint32_t nentries, 
			_In_reads_(nentries) uint32_t* red, 
			_In_reads_(nentries) uint32_t* green,
			_In_reads_(nentries) uint32_t* blue);
		
		void OnChromakeyMode(
			GrChromakeyMode_t mode);
		
		void OnLfbUnlock(
			_In_reads_bytes_(strideInBytes * 480) const uint32_t* lfbPtr,
			uint32_t strideInBytes);
		
		void OnGammaCorrectionRGB(
			float red,
			float green,
			float blue);
		
		void OnBufferSwap();
		
		void OnMousePosChanged(
			int32_t x,
			int32_t y);

		void SetCustomResolution(
			int32_t width,
			int32_t height);
		
		void GetSuggestedCustomResolution(
			_Out_ int32_t* width,
			_Out_ int32_t* height);

		void LogGlideCall(
			_In_z_ const char* s);

		GameVersion GetGameVersion() const;

	private:
		void CheckMajorGameState();
		void PrepareLogoTextureBatch();
		void InsertLogoOnTitleScreen();
		void DrawBatches();
		const Batch PrepareBatchForSubmit(
			Batch batch,
			PrimitiveType primitiveType,
			uint32_t vertexCount,
			uint32_t gameContext) const;
		
		Vertex ReadVertex(
			const uint8_t* vertex,
			uint32_t vertexLayout,
			const Batch& batch);
		
		void FixIngameMousePosition();

		uint32_t _renderFilter;
		bool _capturingFrame;

		Batch _scratchBatch;

		int32_t _frame;
		std::unique_ptr<D3D11Context> _d3d11Context;
		MajorGameState _majorGameState;

		Buffer<uint32_t> _paletteKeys;

		float _gamma[3];
		Buffer<uint32_t> _gammaTable;

		uint32_t _constantColor;

		uint32_t _vertexLayout;

		int32_t _batchCount;
		Buffer<Batch> _batches;

		int32_t _vertexCount;
		Buffer<Vertex> _vertices;

		GameHelper _gameHelper;
		Options _options;
		Batch _logoTextureBatch;

		Buffer<uint8_t> _tmuMemory;
		Buffer<uint8_t> _sideTmuMemory;

		int32_t _mouseX;
		int32_t _mouseY;
		int32_t _customWidth;
		int32_t _customHeight;
	};
}
