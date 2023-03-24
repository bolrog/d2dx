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
#include "D2DXContextFactory.h"
#include "Utils.h"

using namespace d2dx;
using namespace std;

static void GetWidthHeightFromTexInfo(const GrTexInfo* info, FxU32* w, FxU32* h);

static GrLfbInfo_t lfbInfo = { 0 };
static char tempString[2048];
static bool initialized = false;

extern "C" {

FX_ENTRY void FX_CALL
	grDrawPoint(const void* pt)
{
	try
	{
		const auto returnAddress = (uintptr_t)_ReturnAddress();
		D2DXContextFactory::GetInstance()->OnDrawPoint(pt, returnAddress);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grDrawLine(const void* v1, const void* v2)
{
	try
	{
		const auto returnAddress = (uintptr_t)_ReturnAddress();
		D2DXContextFactory::GetInstance()->OnDrawLine(v1, v2, returnAddress);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grVertexLayout(FxU32 param, FxI32 offset, FxU32 mode)
{
	try
	{ 
		D2DXContextFactory::GetInstance()->OnVertexLayout(param, mode ? offset : 0xFF);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grDrawVertexArray(FxU32 mode, FxU32 Count, void* pointers)
{
	const auto returnAddress = (uintptr_t)_ReturnAddress();
	try
	{
		D2DXContextFactory::GetInstance()->OnDrawVertexArray(mode, Count, (uint8_t**)pointers, returnAddress);	
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grDrawVertexArrayContiguous(FxU32 mode, FxU32 Count, void* vertex, FxU32 stride)
{
	const auto returnAddress = (uintptr_t)_ReturnAddress();

	try
	{
		D2DXContextFactory::GetInstance()->OnDrawVertexArrayContiguous(mode, Count, (uint8_t*)vertex, stride, returnAddress);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grBufferClear(GrColor_t color, GrAlpha_t alpha, FxU32 depth)
{
	try
	{
		D2DXContextFactory::GetInstance()->OnBufferClear();
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grBufferSwap(FxU32 swap_interval)
{
	try
	{
		D2DXContextFactory::GetInstance()->OnBufferSwap();
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grRenderBuffer(GrBuffer_t buffer)
{
}

FX_ENTRY void FX_CALL
	grErrorSetCallback(GrErrorCallbackFnc_t fnc)
{
}

FX_ENTRY void FX_CALL
	grFinish(void)
{
}

FX_ENTRY void FX_CALL
	grFlush(void)
{
}

FX_ENTRY GrContext_t FX_CALL
	grSstWinOpen(
		FxU32                hWnd,
		GrScreenResolution_t screen_resolution,
		GrScreenRefresh_t    refresh_rate,
		GrColorFormat_t      color_format,
		GrOriginLocation_t   origin_location,
		int                  nColBuffers,
		int                  nAuxBuffers)
{
	assert(color_format == GR_COLORFORMAT_RGBA);

	if (lfbInfo.lfbPtr)
	{
		memset(lfbInfo.lfbPtr, 0, 640 * 480 * 4);
	}

	int32_t width, height;
	switch (screen_resolution)
	{
	case GR_RESOLUTION_640x480:
		width = 640;
		height = 480;
		break;
	case GR_RESOLUTION_800x600:
		width = 800;
		height = 600;
		break;
	case GR_RESOLUTION_1024x768:
		width = 1024;
		height = 768;
		break;
	default:
		width = -1;
		height = -1;
		break;
	}

	try
	{
		D2DXContextFactory::GetInstance()->OnSstWinOpen(hWnd, width, height);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}

	return 1;
}

FX_ENTRY FxBool FX_CALL
	grSstWinClose(GrContext_t context)
{
	return FXTRUE;
}

FX_ENTRY FxBool FX_CALL
	grSelectContext(GrContext_t context)
{
	return FXTRUE;
}

FX_ENTRY void FX_CALL
	grSstSelect(int which_sst)
{
}

FX_ENTRY void FX_CALL
	grAlphaBlendFunction(
		GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df,
		GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df
	)
{
	try
	{
		D2DXContextFactory::GetInstance()->OnAlphaBlendFunction(rgb_sf, rgb_df, alpha_sf, alpha_df);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grAlphaCombine(
		GrCombineFunction_t function, GrCombineFactor_t factor,
		GrCombineLocal_t local, GrCombineOther_t other,
		FxBool invert
	)
{
	assert(function != GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA);

	//sprintf(tempString, "grAlphaCombine function=%s factor=%s local=%s other=%s invert=%i\n",
	//	GetCombineFunctionString(function),
	//	GetCombineFactorString(factor),
	//	GetCombineLocalString(local),
	//	GetCombineOtherString(other),
	//	invert ? 1 : 0);
	//OutputDebugString(tempString);

	assert(	(function == GR_COMBINE_FUNCTION_ZERO && factor == GR_COMBINE_FACTOR_ZERO && local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT && !invert) ||
			(function == GR_COMBINE_FUNCTION_LOCAL && factor == GR_COMBINE_FACTOR_ZERO && local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT && !invert));

	try
	{
		D2DXContextFactory::GetInstance()->OnAlphaCombine(function, factor, local, other, invert);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grChromakeyMode(GrChromakeyMode_t mode)
{
	try
	{
		D2DXContextFactory::GetInstance()->OnChromakeyMode(mode);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grChromakeyValue(GrColor_t value)
{
	assert(value == 255);
}

FX_ENTRY void FX_CALL
	grColorCombine(
		GrCombineFunction_t function, GrCombineFactor_t factor,
		GrCombineLocal_t local, GrCombineOther_t other,
		FxBool invert)
{
	assert(function != GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA);

	//sprintf(tempString, "grColorCombine function=%s factor=%s local=%s other=%s invert=%i\n",
	//	GetCombineFunctionString(function),
	//	GetCombineFactorString(factor),
	//	GetCombineLocalString(local),
	//	GetCombineOtherString(other),
	//	invert ? 1 : 0);
	//OutputDebugString(tempString);

	assert(
		/* menus+ingame */	(function == GR_COMBINE_FUNCTION_SCALE_OTHER && factor == GR_COMBINE_FACTOR_LOCAL &&	local == GR_COMBINE_LOCAL_ITERATED && other == GR_COMBINE_OTHER_TEXTURE &&	!invert) ||
		/* ingame */		(function == GR_COMBINE_FUNCTION_LOCAL &&		factor == GR_COMBINE_FACTOR_ZERO &&		local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT && !invert)
		);
				
	try
	{
		D2DXContextFactory::GetInstance()->OnColorCombine(function, factor, local, other, invert);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grColorMask(FxBool rgb, FxBool a)
{
}

FX_ENTRY void FX_CALL
	grConstantColorValue(GrColor_t value)
{
	try
	{
		D2DXContextFactory::GetInstance()->OnConstantColorValue((uint32_t)value);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grDepthMask(FxBool mask)
{
}

FX_ENTRY void FX_CALL
	grDitherMode(GrDitherMode_t mode)
{
}

FX_ENTRY void FX_CALL
	grLoadGammaTable(FxU32 nentries, FxU32* red, FxU32* green, FxU32* blue)
{
	try
	{
		D2DXContextFactory::GetInstance()->OnLoadGammaTable(nentries, (uint32_t *)red, (uint32_t*)green, (uint32_t*)blue);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY FxU32 FX_CALL
	grGet(FxU32 pname, FxU32 plength, FxI32* params)
{
	try
	{
		return D2DXContextFactory::GetInstance()->OnGet(pname, plength, (int32_t*)params);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grCoordinateSpace(GrCoordinateSpaceMode_t mode)
{
}

FX_ENTRY void FX_CALL
	grViewport(FxI32 x, FxI32 y, FxI32 width, FxI32 height)
{
	assert(x == 0 && y == 0);
}

FX_ENTRY FxU32 FX_CALL
	grTexMinAddress(GrChipID_t tmu)
{
	return 0;
}

FX_ENTRY FxU32 FX_CALL
	grTexMaxAddress(GrChipID_t tmu)
{
	return tmu == 0 ? D2DX_TMU_MEMORY_SIZE - (2 * D2DX_TMU_ADDRESS_ALIGNMENT) : 0;
}

FX_ENTRY void FX_CALL
	grTexSource(GrChipID_t tmu,
		FxU32      startAddress,
		FxU32      evenOdd,
		GrTexInfo* info)
{
	FxU32 w, h;
	GetWidthHeightFromTexInfo(info, &w, &h);

	try
	{
		D2DXContextFactory::GetInstance()->OnTexSource(tmu, startAddress, w, h, info->largeLodLog2, info->aspectRatioLog2);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grTexClampMode(
		GrChipID_t tmu,
		GrTextureClampMode_t s_clampmode,
		GrTextureClampMode_t t_clampmode
	)
{
}

FX_ENTRY void FX_CALL
	grTexCombine(
		GrChipID_t tmu,
		GrCombineFunction_t rgb_function,
		GrCombineFactor_t rgb_factor,
		GrCombineFunction_t alpha_function,
		GrCombineFactor_t alpha_factor,
		FxBool rgb_invert,
		FxBool alpha_invert
	)
{
	assert(tmu == 0 && rgb_function == GR_COMBINE_FUNCTION_LOCAL && rgb_factor == GR_COMBINE_FACTOR_ZERO && alpha_function == GR_COMBINE_FUNCTION_LOCAL && alpha_factor == GR_COMBINE_FACTOR_ZERO && !rgb_invert && !alpha_invert);
}

FX_ENTRY void FX_CALL
	grTexFilterMode(GrChipID_t tmu, GrTextureFilterMode_t minfilter_mode, GrTextureFilterMode_t magfilter_mode)
{
}

FX_ENTRY void FX_CALL
	grTexDownloadMipMap(GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo* info)
{
	FxU32 width, height;
	GetWidthHeightFromTexInfo(info, &width, &height);

	try
	{
		D2DXContextFactory::GetInstance()->OnTexDownload(tmu, (const uint8_t*)info->data, startAddress, (int32_t)width, (int32_t)height);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grTexDownloadTable(GrTexTable_t type,
		void* data)
{
	try
	{
		D2DXContextFactory::GetInstance()->OnTexDownloadTable(type, data);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grTexMipMapMode(GrChipID_t     tmu,
		GrMipMapMode_t mode,
		FxBool         lodBlend)
{
}

FX_ENTRY FxBool FX_CALL
	grLfbLock(GrLock_t type, GrBuffer_t buffer, GrLfbWriteMode_t writeMode,
		GrOriginLocation_t origin, FxBool pixelPipeline,
		GrLfbInfo_t* info)
{
	try
	{
		if (type == GR_LFB_WRITE_ONLY && buffer == GR_BUFFER_FRONTBUFFER && writeMode == GR_LFBWRITEMODE_8888 &&
			origin == GR_ORIGIN_UPPER_LEFT && !pixelPipeline && info && info->size == 20)
		{
			if (!lfbInfo.lfbPtr)
			{
				lfbInfo.lfbPtr = (uint8_t*)malloc(640 * 480 * 4);
				lfbInfo.strideInBytes = 640 * 4;
			}

			lfbInfo.writeMode = writeMode;
			lfbInfo.origin = origin;
			*info = lfbInfo;

			return FXTRUE;
		}
		else
		{
			return FXFALSE;
		}
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY FxBool FX_CALL
	grLfbUnlock(GrLock_t type, GrBuffer_t buffer)
{
	try
	{
		if (type == GR_LFB_WRITE_ONLY && buffer == GR_BUFFER_FRONTBUFFER)
		{
			D2DXContextFactory::GetInstance()->OnLfbUnlock((const uint32_t*)lfbInfo.lfbPtr, lfbInfo.strideInBytes);
			return FXTRUE;
		}
		else
		{
			return FXFALSE;
		}
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

FX_ENTRY void FX_CALL
	grGlideInit(void)
{
}

FX_ENTRY void FX_CALL
	grGlideShutdown(void)
{
}

FX_ENTRY void FX_CALL
	guGammaCorrectionRGB(FxFloat red, FxFloat green, FxFloat blue)
{
	try
	{
		D2DXContextFactory::GetInstance()->OnGammaCorrectionRGB(red, green, blue);
	}
	catch (...)
	{
		D2DX_FATAL_EXCEPTION;
	}
}

/*
	UNUSED GLIDE FUNCTIONS 
*/

FX_ENTRY FxI32 FX_CALL
	grQueryResolutions(const GrResolution* resTemplate, GrResolution* output)
{
	assert(false && "grQueryResolutions: Unsupported");
	return 0;
}

FX_ENTRY FxBool FX_CALL
	grReset(FxU32 what)
{
	assert(false && "grReset: Unsupported");
	return FXTRUE;
}

FX_ENTRY void FX_CALL
	grEnable(GrEnableMode_t mode)
{
	assert(false && "grEnable: Unsupported");
}

FX_ENTRY void FX_CALL
	grDisable(GrEnableMode_t mode)
{
	assert(false && "grDisable: Unsupported");
}

FX_ENTRY void FX_CALL
	grGlideGetState(void* state)
{
	assert(false && "grGlideGetState: Unsupported");
}

FX_ENTRY void FX_CALL
	grGlideSetState(const void* state)
{
	assert(false && "grGlideSetState: Unsupported");
}

FX_ENTRY void FX_CALL
	grGlideGetVertexLayout(void* layout)
{
	assert(false && "grGlideGetVertexLayout: Unsupported");
}

FX_ENTRY void FX_CALL
	grGlideSetVertexLayout(const void* layout)
{
	assert(false && "grGlideSetVertexLayout: Unsupported");
}

FX_ENTRY float FX_CALL
	guFogTableIndexToW(int i)
{
	assert(false && "guFogTableIndexToW: Unsupported");
	//float r = (*pguFogTableIndexToW)(i);
	return 0.0f;
}

FX_ENTRY void FX_CALL
	guFogGenerateExp(GrFog_t* fogtable, float density)
{
	assert(false && "guFogGenerateExp: Unsupported");
	//(*pguFogGenerateExp)(fogtable, density);
}

FX_ENTRY void FX_CALL
	guFogGenerateExp2(GrFog_t* fogtable, float density)
{
	assert(false && "guFogGenerateExp2: Unsupported");
	//(*pguFogGenerateExp2)(fogtable, density);
}

FX_ENTRY void FX_CALL
	guFogGenerateLinear(GrFog_t* fogtable,
		float nearZ, float farZ)
{
	assert(false && "guFogGenerateLinear: Unsupported");
	//(*pguFogGenerateLinear)(fogtable, nearZ, farZ);
}

FX_ENTRY FxBool FX_CALL
	gu3dfGetInfo(const char* filename, Gu3dfInfo* info)
{
	assert(false && "gu3dfGetInfo: Unsupported");
	//FxBool r = (*pgu3dfGetInfo)(filename, info);
	return FXFALSE;
}

FX_ENTRY FxBool FX_CALL
	gu3dfLoad(const char* filename, Gu3dfInfo* data)
{
	assert(false && "gu3dfLoad: Unsupported");
	//FxBool r = (*pgu3dfLoad)(filename, data);
	return FXFALSE;
}

FX_ENTRY void FX_CALL
	grCheckForRoom(FxI32 n)
{
	assert(false && "grCheckForRoom: Unsupported");
}

FX_ENTRY int FX_CALL
	guEncodeRLE16(void* dst,
		void* src,
		FxU32 width,
		FxU32 height)
{
	assert(false && "guEncodeRLE16: Unsupported");
	return 0;
}

FX_ENTRY void FX_CALL
	grSstVidMode(FxU32 whichSst, void* vidTimings)
{
	assert(false && "grSstVidMode: Unsupported");
	//(*pgrSstVidMode)(whichSst, vidTimings);
}


FX_ENTRY void FX_CALL
	grAlphaControlsITRGBLighting(FxBool enable)
{
	assert(false && "grAlphaControlsITRGBLighting: Unsupported");
}

FX_ENTRY void FX_CALL
	grAlphaTestFunction(GrCmpFnc_t function)
{
	assert(false && "grAlphaTestFunction: Unsupported");
}

FX_ENTRY void FX_CALL
	grAlphaTestReferenceValue(GrAlpha_t value)
{
	assert(false && "grAlphaTestReferenceValue: Unsupported");
}

FX_ENTRY void FX_CALL
	grLfbConstantAlpha(GrAlpha_t alpha)
{
	assert(false && "grLfbConstantAlpha: Unsupported");
}

FX_ENTRY void FX_CALL
	grLfbConstantDepth(FxU32 depth)
{
	assert(false && "grLfbConstantDepth: Unsupported");
}

FX_ENTRY void FX_CALL
	grLfbWriteColorSwizzle(FxBool swizzleBytes, FxBool swapWords)
{
	assert(false && "grLfbWriteColorSwizzle: Unsupported");
}

FX_ENTRY void FX_CALL
	grLfbWriteColorFormat(GrColorFormat_t colorFormat)
{
	assert(false && "grLfbWriteColorFormat: Unsupported");
}

FX_ENTRY FxBool FX_CALL
	grLfbWriteRegion(GrBuffer_t dst_buffer,
		FxU32 dst_x, FxU32 dst_y,
		GrLfbSrcFmt_t src_format,
		FxU32 src_width, FxU32 src_height,
		FxBool pixelPipeline,
		FxI32 src_stride, void* src_data)
{
	assert(false && "grLfbWriteRegion: Unsupported");
	return FXFALSE;
}

FX_ENTRY FxBool FX_CALL
	grLfbReadRegion(GrBuffer_t src_buffer,
		FxU32 src_x, FxU32 src_y,
		FxU32 src_width, FxU32 src_height,
		FxU32 dst_stride, void* dst_data)
{
	assert(false && "grLfbReadRegion: Unsupported");
	return FXFALSE;
}

FX_ENTRY void FX_CALL
	grTexMultibase(GrChipID_t tmu,
		FxBool     enable)
{
	assert(false && "grTexMultibase: Unsupported");
}

FX_ENTRY void FX_CALL
	grTexMultibaseAddress(GrChipID_t       tmu,
		GrTexBaseRange_t range,
		FxU32            startAddress,
		FxU32            evenOdd,
		GrTexInfo* info)
{
	assert(false && "grTexMultibaseAddress: Unsupported");
}

FX_ENTRY void FX_CALL
	grDrawTriangle(const void* a, const void* b, const void* c)
{
	assert(false && "grDrawTriangle: Unsupported");
}

FX_ENTRY void FX_CALL
	grAADrawTriangle(
		const void* a, const void* b, const void* c,
		FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias
	)
{
	assert(false && "grAADrawTriangle: Unsupported");
}

FX_ENTRY void FX_CALL
	grClipWindow(FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy)
{
	assert(false && "grClipWindow: Unsupported");
}

FX_ENTRY void FX_CALL
	grSstOrigin(GrOriginLocation_t origin)
{
	assert(false && "grSstOrigin: Unsupported");
}

FX_ENTRY void FX_CALL
	grCullMode(GrCullMode_t mode)
{
	assert(false && "grCullMode: Unsupported");
}

FX_ENTRY void FX_CALL
	grDepthBiasLevel(FxI32 level)
{
	assert(false && "grDepthBiasLevel: Unsupported");
}

FX_ENTRY void FX_CALL
	grDepthBufferFunction(GrCmpFnc_t function)
{
	assert(false && "grDepthBufferFunction: Unsupported");
}

FX_ENTRY void FX_CALL
	grDepthBufferMode(GrDepthBufferMode_t mode)
{
	assert(false && "grDepthBufferMode: Unsupported");
}

FX_ENTRY void FX_CALL
	grFogColorValue(GrColor_t fogcolor)
{
	assert(false && "grFogColorValue: Unsupported");
}

FX_ENTRY void FX_CALL
	grFogMode(GrFogMode_t mode)
{
	assert(false && "grFogMode: Unsupported");
}

FX_ENTRY void FX_CALL
	grFogTable(const GrFog_t ft[])
{
	assert(false && "grFogTable: Unsupported");
}

FX_ENTRY void FX_CALL
	grSplash(float x, float y, float width, float height, FxU32 frame)
{
	assert(false && "grSplash: Unsupported");
}

FX_ENTRY GrProc FX_CALL
	grGetProcAddress(char* procName)
{
	assert(false && "grGetProcAddress: Unsupported");
	return NULL;
}

FX_ENTRY void FX_CALL
	grDepthRange(FxFloat n, FxFloat f)
{
	assert(false && "grDepthRange: Unsupported");
}

FX_ENTRY void FX_CALL
	grTexNCCTable(GrNCCTable_t table)
{
	assert(false && "grTexNCCTable: Unsupported");
}

FX_ENTRY void FX_CALL
	grTexDetailControl(
		GrChipID_t tmu,
		int lod_bias,
		FxU8 detail_scale,
		float detail_max
	)
{
	assert(false && "grTexDetailControl: Unsupported");
}

FX_ENTRY void FX_CALL
	grTexLodBiasValue(GrChipID_t tmu, float bias)
{
	assert(false && "grTexLodBiasValue: Unsupported");
}

FX_ENTRY void FX_CALL
	grTexDownloadMipMapLevel(GrChipID_t        tmu,
		FxU32             startAddress,
		GrLOD_t           thisLod,
		GrLOD_t           largeLod,
		GrAspectRatio_t   aspectRatio,
		GrTextureFormat_t format,
		FxU32             evenOdd,
		void* data)
{
	assert(false && "grTexDownloadMipMapLevel: Unsupported");
}

FX_ENTRY FxBool FX_CALL
	grTexDownloadMipMapLevelPartial(GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod, GrLOD_t largeLod, GrAspectRatio_t aspectRatio, GrTextureFormat_t format,
		FxU32 evenOdd, void* data, int start, int end)
{
	assert(false && "grTexDownloadMipMapLevelPartial: Unsupported");
	return FXFALSE;
}

FX_ENTRY FxU32 FX_CALL
	grTexCalcMemRequired(
		GrLOD_t lodmin, GrLOD_t lodmax,
		GrAspectRatio_t aspect, GrTextureFormat_t fmt)
{
	assert(false && "grTexCalcMemRequired: Unsupported");
	return 0;
}

FX_ENTRY FxU32 FX_CALL
	grTexTextureMemRequired(FxU32     evenOdd,
		GrTexInfo* info)
{
	assert(false && "grTexTextureMemRequired: Unsupported");
	return 0;
}

FX_ENTRY void FX_CALL
	grDisableAllEffects(void)
{
	assert(false && "grDisableAllEffects: Unsupported");
}

FX_ENTRY const char* FX_CALL
	grGetString(FxU32 pname)
{
	return D2DXContextFactory::GetInstance()->OnGetString(pname);
}

FX_ENTRY void FX_CALL
	grTexDownloadTablePartial(GrTexTable_t type,
		void* data,
		int          start,
		int          end)
{
	assert(false);
}

}

static void GetWidthHeightFromTexInfo(const GrTexInfo* info, FxU32* w, FxU32* h)
{
	FxU32 ww = 1 << info->largeLodLog2;
	switch (info->aspectRatioLog2)
	{
	case GR_ASPECT_LOG2_1x1:
		*w = ww;
		*h = ww;
		break;
	case GR_ASPECT_LOG2_1x2:
		*w = ww / 2;
		*h = ww;
		break;
	case GR_ASPECT_LOG2_2x1:
		*w = ww;
		*h = ww / 2;
		break;
	case GR_ASPECT_LOG2_1x4:
		*w = ww / 4;
		*h = ww;
		break;
	case GR_ASPECT_LOG2_4x1:
		*w = ww;
		*h = ww / 4;
		break;
	case GR_ASPECT_LOG2_1x8:
		*w = ww / 8;
		*h = ww;
		break;
	case GR_ASPECT_LOG2_8x1:
		*w = ww;
		*h = ww / 8;
		break;
	default:
		*w = 0;
		*h = 0;
		break;
	}
}
