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

#include "Types.h"

namespace d2dx
{
	struct IGlide3x abstract
	{
		virtual ~IGlide3x() noexcept {}

		virtual const char* OnGetString(
			_In_ uint32_t pname) = 0;

		virtual uint32_t OnGet(
			_In_ uint32_t pname,
			_In_ uint32_t plength,
			_Out_writes_(plength) int32_t* params) = 0;

		virtual void OnSstWinOpen(
			_In_ uint32_t hWnd,
			_In_ int32_t width,
			_In_ int32_t height) = 0;

		virtual void OnVertexLayout(
			_In_ uint32_t param,
			_In_ int32_t offset) = 0;

		virtual void OnTexDownload(
			_In_ uint32_t tmu,
			_In_reads_(width* height) const uint8_t* sourceAddress,
			_In_ uint32_t startAddress,
			_In_ int32_t width,
			_In_ int32_t height) = 0;

		virtual void OnTexSource(
			_In_ uint32_t tmu,
			_In_ uint32_t startAddress,
			_In_ int32_t width,
			_In_ int32_t height) = 0;

		virtual void OnConstantColorValue(
			_In_ uint32_t color) = 0;

		virtual void OnAlphaBlendFunction(
			_In_ GrAlphaBlendFnc_t rgb_sf,
			_In_ GrAlphaBlendFnc_t rgb_df,
			_In_ GrAlphaBlendFnc_t alpha_sf,
			_In_ GrAlphaBlendFnc_t alpha_df) = 0;

		virtual void OnColorCombine(
			_In_ GrCombineFunction_t function,
			_In_ GrCombineFactor_t factor,
			_In_ GrCombineLocal_t local,
			_In_ GrCombineOther_t other,
			_In_ bool invert) = 0;

		virtual void OnAlphaCombine(
			_In_ GrCombineFunction_t function,
			_In_ GrCombineFactor_t factor,
			_In_ GrCombineLocal_t local,
			_In_ GrCombineOther_t other,
			_In_ bool invert) = 0;

		virtual void OnDrawLine(
			_In_ const void* v1,
			_In_ const void* v2,
			_In_ uint32_t gameContext) = 0;

		virtual void OnDrawVertexArray(
			_In_ uint32_t mode,
			_In_ uint32_t count,
			_In_reads_(count) uint8_t** pointers,
			_In_ uint32_t gameContext) = 0;

		virtual void OnDrawVertexArrayContiguous(
			_In_ uint32_t mode,
			_In_ uint32_t count,
			_In_reads_(count* stride) uint8_t* vertex,
			_In_ uint32_t stride,
			_In_ uint32_t gameContext) = 0;

		virtual void OnTexDownloadTable(
			_In_ GrTexTable_t type,
			_In_reads_bytes_(256 * 4) void* data) = 0;

		virtual void OnLoadGammaTable(
			_In_ uint32_t nentries,
			_In_reads_(nentries) uint32_t* red,
			_In_reads_(nentries) uint32_t* green,
			_In_reads_(nentries) uint32_t* blue) = 0;

		virtual void OnChromakeyMode(
			_In_ GrChromakeyMode_t mode) = 0;

		virtual void OnLfbUnlock(
			_In_reads_bytes_(strideInBytes * 480) const uint32_t* lfbPtr,
			_In_ uint32_t strideInBytes) = 0;

		virtual void OnGammaCorrectionRGB(
			_In_ float red,
			_In_ float green,
			_In_ float blue) = 0;

		virtual void OnBufferSwap() = 0;
	};
}
