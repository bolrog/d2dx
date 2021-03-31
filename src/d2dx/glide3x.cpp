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
#include "GlideHelpers.h"
#include "D2DXContext.h"

using namespace d2dx;
using namespace std;

unique_ptr<D2DXContext> g_d2dxContext;

static GrLfbInfo_t lfbInfo = { 0 };
static char tempString[2048];
static bool initialized = false;

#define D2DX_LOG(s, ...) \
	if (g_d2dxContext && g_d2dxContext->IsCapturingFrame()) { \
		sprintf_s(tempString, s, __VA_ARGS__); \
		g_d2dxContext->LogGlideCall(tempString); \
	}

static void EnsureInitialized()
{
	/* The game is single threaded and there's no worry about synchronization. */

	if (initialized)
	{
		return;
	}

	initialized = true;

	g_d2dxContext = std::make_unique<D2DXContext>();
}

extern "C" {

FX_ENTRY void FX_CALL
	grDrawPoint(const void* pt)
{
	EnsureInitialized();
	D2DX_LOG("grDrawPoint pt=%p\n", pt);
}

FX_ENTRY void FX_CALL
	grDrawLine(const void* v1, const void* v2)
{
	EnsureInitialized();

	D2DX_LOG("grDrawLine v1=%p v2=%p\n", v1, v2);

	if (!g_d2dxContext)
	{
		return;
	}

	const auto returnAddress = (uintptr_t)_ReturnAddress();
	g_d2dxContext->OnDrawLine(v1, v2, returnAddress);
}

FX_ENTRY void FX_CALL
	grVertexLayout(FxU32 param, FxI32 offset, FxU32 mode)
{
	EnsureInitialized();

	if (!g_d2dxContext)
	{
		return;
	}

	D2DX_LOG("grVertexLayout param=%s offset=%i mode=%u\n", GetVertexLayoutParamString(param), offset, mode);
	g_d2dxContext->OnVertexLayout(param, mode ? offset : 0xFF);		
}

FX_ENTRY void FX_CALL
	grDrawVertexArray(FxU32 mode, FxU32 Count, void* pointers)
{
	EnsureInitialized();
	D2DX_LOG("grDrawVertexArray mode=%s count=%u pointers=%p\n", GetPrimitiveTypeString(mode), Count, pointers);
		
	if (!g_d2dxContext)
	{
		return;
	}

	const auto returnAddress = (uintptr_t)_ReturnAddress();
	g_d2dxContext->OnDrawVertexArray(mode, Count, (uint8_t**)pointers, returnAddress);	
}

FX_ENTRY void FX_CALL
	grDrawVertexArrayContiguous(FxU32 mode, FxU32 Count, void* vertex, FxU32 stride)
{
	EnsureInitialized();
	D2DX_LOG("grDrawVertexArrayContiguous mode=%s count=%u vertex=%p stride=%u\n", GetPrimitiveTypeString(mode), Count, vertex, stride);

	if (!g_d2dxContext)
	{
		return;
	}

	const auto returnAddress = (uintptr_t)_ReturnAddress();
	g_d2dxContext->OnDrawVertexArrayContiguous(mode, Count, (uint8_t*)vertex, stride, returnAddress);
}

FX_ENTRY void FX_CALL
	grBufferClear(GrColor_t color, GrAlpha_t alpha, FxU32 depth)
{
	EnsureInitialized();
	D2DX_LOG("grBufferClear color=%u alpha=%u depth=%u\n", color, alpha, depth);
}

FX_ENTRY void FX_CALL
	grBufferSwap(FxU32 swap_interval)
{
	EnsureInitialized();
	D2DX_LOG("grBufferSwap swap_interval=%u\n", swap_interval);

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnBufferSwap();
}

FX_ENTRY void FX_CALL
	grRenderBuffer(GrBuffer_t buffer)
{
	EnsureInitialized();
	D2DX_LOG("grRenderBuffer buffer=%u\n", buffer);
}

FX_ENTRY void FX_CALL
	grErrorSetCallback(GrErrorCallbackFnc_t fnc)
{
	EnsureInitialized();
	D2DX_LOG("grErrorSetCallback fnc=%p\n", fnc);
}

FX_ENTRY void FX_CALL
	grFinish(void)
{
	EnsureInitialized();
	D2DX_LOG("grFinish\n");
}

FX_ENTRY void FX_CALL
	grFlush(void)
{
	EnsureInitialized();
	D2DX_LOG("grFlush\n");
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
	EnsureInitialized();
	assert(color_format == GR_COLORFORMAT_RGBA);
	D2DX_LOG("grSstWinOpen hWnd=%u screen_resolution=%u refresh_rate=%u color_format=%u origin_location=%u nColBuffers=%i nAuxBuffers=%i\n",
		hWnd, screen_resolution, refresh_rate, color_format, origin_location, nColBuffers, nAuxBuffers);

	if (!g_d2dxContext)
	{
		return 0;
	}

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

	g_d2dxContext->OnSstWinOpen(hWnd, width, height);
	return 1;
}

FX_ENTRY FxBool FX_CALL
	grSstWinClose(GrContext_t context)
{
	EnsureInitialized();
	D2DX_LOG("grSstWinClose context=%u\n", context);
	return FXTRUE;
}

FX_ENTRY FxBool FX_CALL
	grSelectContext(GrContext_t context)
{
	EnsureInitialized();
	D2DX_LOG("grSelectContext context=%u\n", context);
	return FXTRUE;
}

FX_ENTRY void FX_CALL
	grSstSelect(int which_sst)
{
	EnsureInitialized();
	D2DX_LOG("grSstSelect which_sst=%i\n", which_sst);
}

FX_ENTRY void FX_CALL
	grAlphaBlendFunction(
		GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df,
		GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df
	)
{
	EnsureInitialized();
	D2DX_LOG("grAlphaBlendFunction rgb_sf=%s rgb_df=%s alpha_sf=%s alpha_df=%s\n",
		GetAlphaBlendFncString(rgb_sf),
		GetAlphaBlendFncString(rgb_df),
		GetAlphaBlendFncString(alpha_sf),
		GetAlphaBlendFncString(alpha_df));

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnAlphaBlendFunction(rgb_sf, rgb_df, alpha_sf, alpha_df);
}

FX_ENTRY void FX_CALL
	grAlphaCombine(
		GrCombineFunction_t function, GrCombineFactor_t factor,
		GrCombineLocal_t local, GrCombineOther_t other,
		FxBool invert
	)
{
	EnsureInitialized();
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

	D2DX_LOG("grAlphaCombine function=%s factor=%s local=%s other=%s invert=%i\n",
		GetCombineFunctionString(function),
		GetCombineFactorString(factor),
		GetCombineLocalString(local),
		GetCombineOtherString(other),
		invert ? 1 : 0);

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnAlphaCombine(function, factor, local, other, invert);
}

FX_ENTRY void FX_CALL
	grChromakeyMode(GrChromakeyMode_t mode)
{
	EnsureInitialized();
	D2DX_LOG("grChromakeyMode mode=%i\n", mode);

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnChromakeyMode(mode);
}

FX_ENTRY void FX_CALL
	grChromakeyValue(GrColor_t value)
{
	EnsureInitialized();
	D2DX_LOG("grChromakeyValue value=%u\n", value);
	assert(value == 255);
}

FX_ENTRY void FX_CALL
	grColorCombine(
		GrCombineFunction_t function, GrCombineFactor_t factor,
		GrCombineLocal_t local, GrCombineOther_t other,
		FxBool invert)
{
	EnsureInitialized();
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
				
	D2DX_LOG("grColorCombine function=%s factor=%s local=%s other=%s invert=%i\n", 
		GetCombineFunctionString(function),
		GetCombineFactorString(factor),
		GetCombineLocalString(local),
		GetCombineOtherString(other), 
		invert ? 1 : 0);

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnColorCombine(function, factor, local, other, invert);
}

FX_ENTRY void FX_CALL
	grColorMask(FxBool rgb, FxBool a)
{
	EnsureInitialized();
	D2DX_LOG("grColorMask rgb=%i a=%i\n", rgb ? 1 : 0, a ? 1 : 0);
}

FX_ENTRY void FX_CALL
	grConstantColorValue(GrColor_t value)
{
	EnsureInitialized();
	D2DX_LOG("grConstantColorValue value=%u\n", value);

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnConstantColorValue((uint32_t)value);
}

FX_ENTRY void FX_CALL
	grDepthMask(FxBool mask)
{
	EnsureInitialized();
	D2DX_LOG("grDepthMask mask=%i\n", mask ? 1 : 0);
}

FX_ENTRY void FX_CALL
	grDitherMode(GrDitherMode_t mode)
{
	EnsureInitialized();
	D2DX_LOG("grDitherMode mode=%i\n", mode);
}

FX_ENTRY void FX_CALL
	grLoadGammaTable(FxU32 nentries, FxU32* red, FxU32* green, FxU32* blue)
{
	EnsureInitialized();
	D2DX_LOG("grLoadGammaTable nentries=%u red=%p green=%p blue=%p\n", nentries, red, green, blue);

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnLoadGammaTable(nentries, (uint32_t *)red, (uint32_t*)green, (uint32_t*)blue);
}

FX_ENTRY FxU32 FX_CALL
	grGet(FxU32 pname, FxU32 plength, FxI32* params)
{
	EnsureInitialized();

	D2DX_LOG("grGet pname=%u plength=%u params=%p\n", pname, plength, params);

	if (!g_d2dxContext)
	{
		return 0;
	}

	return g_d2dxContext->OnGet(pname, plength, (int32_t *)params);
}

FX_ENTRY void FX_CALL
	grCoordinateSpace(GrCoordinateSpaceMode_t mode)
{
	EnsureInitialized();
	D2DX_LOG("grCoordinateSpace mode=%s\n", GetCoordinateSpaceModeString(mode));
}

FX_ENTRY void FX_CALL
	grViewport(FxI32 x, FxI32 y, FxI32 width, FxI32 height)
{
	EnsureInitialized();
	assert(x == 0 && y == 0);
	D2DX_LOG("grViewport x=%i y=%i width=%i height=%i\n", x, y, width, height);
}

FX_ENTRY FxU32 FX_CALL
	grTexMinAddress(GrChipID_t tmu)
{
	EnsureInitialized();
	return 0;
}

FX_ENTRY FxU32 FX_CALL
	grTexMaxAddress(GrChipID_t tmu)
{
	EnsureInitialized();
	return tmu == 0 ? D2DX_TMU_MEMORY_SIZE - 512 : 0;
}

FX_ENTRY void FX_CALL
	grTexSource(GrChipID_t tmu,
		FxU32      startAddress,
		FxU32      evenOdd,
		GrTexInfo* info)
{
	EnsureInitialized();
	FxU32 w, h;
	GetWidthHeightFromTexInfo(info, &w, &h);
	D2DX_LOG("grTexSource tmu=%i startAddress=%08x evenOdd=%u fmt=%s w=%u h=%u\n",
		tmu, startAddress, evenOdd, GetTextureFormatString(info->format), w, h);

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnTexSource(tmu, startAddress, w, h);
}

FX_ENTRY void FX_CALL
	grTexClampMode(
		GrChipID_t tmu,
		GrTextureClampMode_t s_clampmode,
		GrTextureClampMode_t t_clampmode
	)
{
	EnsureInitialized();
	D2DX_LOG("grTexClampMode tmu=%i s_clampmode=%i t_clampmode=%i\n",
		tmu, s_clampmode, t_clampmode);
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
	EnsureInitialized();
	assert(tmu == 0 && rgb_function == GR_COMBINE_FUNCTION_LOCAL && rgb_factor == GR_COMBINE_FACTOR_ZERO && alpha_function == GR_COMBINE_FUNCTION_LOCAL && alpha_factor == GR_COMBINE_FACTOR_ZERO && !rgb_invert && !alpha_invert);

	D2DX_LOG("grTexCombine tmu=%i rgb_function=%s rgb_factor=%s alpha_function=%s alpha_factor=%s rgb_invert=%i alpha_invert=%i\n",
		tmu, 
		GetCombineFunctionString(rgb_function),
		GetCombineFactorString(rgb_factor),
		GetCombineFunctionString(alpha_function),
		GetCombineFactorString(alpha_factor),
		rgb_invert,
		alpha_invert);
}

FX_ENTRY void FX_CALL
	grTexFilterMode(GrChipID_t tmu, GrTextureFilterMode_t minfilter_mode, GrTextureFilterMode_t magfilter_mode)
{
	EnsureInitialized();
	D2DX_LOG("grTexFilterMode tmu=%i minfilter_mode=%s magfilter_mode=%s\n",
		tmu, GetStringForTextureFilterMode(minfilter_mode), GetStringForTextureFilterMode(magfilter_mode));
}

FX_ENTRY void FX_CALL
	grTexDownloadMipMap(GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo* info)
{
	EnsureInitialized();
	D2DX_LOG("grTexDownloadMipMap tmu=%u startAddress=%08x evenOdd=%u info=%p\n",
		tmu, startAddress, evenOdd, info);
	FxU32 width, height;
	GetWidthHeightFromTexInfo(info, &width, &height);

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnTexDownload(tmu, (const uint8_t*)info->data, startAddress, (int32_t)width, (int32_t)height);
}

FX_ENTRY void FX_CALL
	grTexDownloadTable(GrTexTable_t type,
		void* data)
{
	EnsureInitialized();
	D2DX_LOG("grTexDownloadTable type=%i data=%p\n", type, data);

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnTexDownloadTable(type, data);
}

FX_ENTRY void FX_CALL
	grTexMipMapMode(GrChipID_t     tmu,
		GrMipMapMode_t mode,
		FxBool         lodBlend)
{
	EnsureInitialized();
	D2DX_LOG("grTexMipMapMode tmu=%i mode=%i lodBlend=%i\n", tmu, mode, lodBlend ? 1 : 0);
}

FX_ENTRY FxBool FX_CALL
	grLfbLock(GrLock_t type, GrBuffer_t buffer, GrLfbWriteMode_t writeMode,
		GrOriginLocation_t origin, FxBool pixelPipeline,
		GrLfbInfo_t* info)
{
	EnsureInitialized();
	D2DX_LOG("grLfbLock\n");

	if (!g_d2dxContext)
	{
		return FXFALSE;
	}

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

FX_ENTRY FxBool FX_CALL
	grLfbUnlock(GrLock_t type, GrBuffer_t buffer)
{
	EnsureInitialized();
	D2DX_LOG("grLfbUnlock\n");

	if (!g_d2dxContext)
	{
		return FXFALSE;
	}

	if (type == GR_LFB_WRITE_ONLY && buffer == GR_BUFFER_FRONTBUFFER)
	{
		g_d2dxContext->OnLfbUnlock((const uint32_t*)lfbInfo.lfbPtr, lfbInfo.strideInBytes);
		return FXTRUE;
	}
	else
	{
		return FXFALSE;
	}
}

FX_ENTRY void FX_CALL
	grGlideInit(void)
{
	EnsureInitialized();
	D2DX_LOG("grGlideInit\n");

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnGlideInit();
}

FX_ENTRY void FX_CALL
	grGlideShutdown(void)
{
	EnsureInitialized();
	D2DX_LOG("grGlideShutdown\n");

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnGlideShutdown();
}

FX_ENTRY void FX_CALL
	guGammaCorrectionRGB(FxFloat red, FxFloat green, FxFloat blue)
{
	EnsureInitialized();
	D2DX_LOG("guGammaCorrectionRGB\n");

	if (!g_d2dxContext)
	{
		return;
	}

	g_d2dxContext->OnGammaCorrectionRGB(red, green, blue);
}

/*
	UNUSED GLIDE FUNCTIONS 
*/

FX_ENTRY FxI32 FX_CALL
	grQueryResolutions(const GrResolution* resTemplate, GrResolution* output)
{
	assert(false && "grQueryResolutions: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grQueryResolutions resTemplate=%p output=%p\n", resTemplate, output);
	return 0;
}

FX_ENTRY FxBool FX_CALL
	grReset(FxU32 what)
{
	assert(false && "grReset: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grReset what=%u\n", what);
	return FXTRUE;
}

FX_ENTRY void FX_CALL
	grEnable(GrEnableMode_t mode)
{
	assert(false && "grEnable: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grEnable mode=%u\n", mode);
}

FX_ENTRY void FX_CALL
	grDisable(GrEnableMode_t mode)
{
	assert(false && "grDisable: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grDisable mode=%u\n", mode);
}

FX_ENTRY void FX_CALL
	grGlideGetState(void* state)
{
	assert(false && "grGlideGetState: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grGlideGetState state=%p\n", state);
	//(*pgrGlideGetState)(state);
}

FX_ENTRY void FX_CALL
	grGlideSetState(const void* state)
{
	assert(false && "grGlideSetState: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grGlideSetState state=%p\n", state);
	//(*pgrGlideSetState)(state);
}

FX_ENTRY void FX_CALL
	grGlideGetVertexLayout(void* layout)
{
	assert(false && "grGlideGetVertexLayout: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grGlideGetVertexLayout layout=%p\n", layout);
	//(*pgrGlideGetVertexLayout)(layout);
}

FX_ENTRY void FX_CALL
	grGlideSetVertexLayout(const void* layout)
{
	assert(false && "grGlideSetVertexLayout: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grGlideSetVertexLayout layout=%p\n", layout);
	//(*pgrGlideSetVertexLayout)(layout);
}

FX_ENTRY float FX_CALL
	guFogTableIndexToW(int i)
{
	assert(false && "guFogTableIndexToW: Unsupported");
	EnsureInitialized();
	//float r = (*pguFogTableIndexToW)(i);
	return 0.0f;
}

FX_ENTRY void FX_CALL
	guFogGenerateExp(GrFog_t* fogtable, float density)
{
	assert(false && "guFogGenerateExp: Unsupported");
	EnsureInitialized();
	//(*pguFogGenerateExp)(fogtable, density);
}

FX_ENTRY void FX_CALL
	guFogGenerateExp2(GrFog_t* fogtable, float density)
{
	assert(false && "guFogGenerateExp2: Unsupported");
	EnsureInitialized();
	//(*pguFogGenerateExp2)(fogtable, density);
}

FX_ENTRY void FX_CALL
	guFogGenerateLinear(GrFog_t* fogtable,
		float nearZ, float farZ)
{
	assert(false && "guFogGenerateLinear: Unsupported");
	EnsureInitialized();
	//(*pguFogGenerateLinear)(fogtable, nearZ, farZ);
}

FX_ENTRY FxBool FX_CALL
	gu3dfGetInfo(const char* filename, Gu3dfInfo* info)
{
	assert(false && "gu3dfGetInfo: Unsupported");
	EnsureInitialized();
	//FxBool r = (*pgu3dfGetInfo)(filename, info);
	return FXFALSE;
}

FX_ENTRY FxBool FX_CALL
	gu3dfLoad(const char* filename, Gu3dfInfo* data)
{
	assert(false && "gu3dfLoad: Unsupported");
	EnsureInitialized();
	//FxBool r = (*pgu3dfLoad)(filename, data);
	return FXFALSE;
}

FX_ENTRY void FX_CALL
	grCheckForRoom(FxI32 n)
{
	assert(false && "grCheckForRoom: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grCheckForRoom n=%i\n", n);
	//(*pgrCheckForRoom)(n);
}

FX_ENTRY int FX_CALL
	guEncodeRLE16(void* dst,
		void* src,
		FxU32 width,
		FxU32 height)
{
	assert(false && "guEncodeRLE16: Unsupported");
	EnsureInitialized();
	//int r = (*pguEncodeRLE16)(dst, src, width, height);
	//D2DX_LOG("guEncodeRLE16 dst=%p src=%p width=%u height=%u -> %i\n", dst, src, width, height, r);
	return 0;
}

FX_ENTRY void FX_CALL
	grSstVidMode(FxU32 whichSst, void* vidTimings)
{
	assert(false && "grSstVidMode: Unsupported");
	EnsureInitialized();
	//(*pgrSstVidMode)(whichSst, vidTimings);
}


FX_ENTRY void FX_CALL
	grAlphaControlsITRGBLighting(FxBool enable)
{
	assert(false && "grAlphaControlsITRGBLighting: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grAlphaControlsITRGBLighting enable=%i\n", enable ? 1 : 0);
	//(*pgrAlphaControlsITRGBLighting)(enable);
}

FX_ENTRY void FX_CALL
	grAlphaTestFunction(GrCmpFnc_t function)
{
	assert(false && "grAlphaTestFunction: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grAlphaTestFunction function=%i\n", function);
	//(*pgrAlphaTestFunction)(function);
}

FX_ENTRY void FX_CALL
	grAlphaTestReferenceValue(GrAlpha_t value)
{
	assert(false && "grAlphaTestReferenceValue: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grAlphaTestReferenceValue value=%u\n", value);
	//(*pgrAlphaTestReferenceValue)(value);
}

FX_ENTRY void FX_CALL
	grLfbConstantAlpha(GrAlpha_t alpha)
{
	assert(false && "grLfbConstantAlpha: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grLfbConstantDepth alpha=%u\n", alpha);
	//(*pgrLfbConstantAlpha)(alpha);
}

FX_ENTRY void FX_CALL
	grLfbConstantDepth(FxU32 depth)
{
	assert(false && "grLfbConstantDepth: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grLfbConstantDepth depth=%u\n", depth);
	//(*pgrLfbConstantDepth)(depth);
}

FX_ENTRY void FX_CALL
	grLfbWriteColorSwizzle(FxBool swizzleBytes, FxBool swapWords)
{
	assert(false && "grLfbWriteColorSwizzle: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grLfbWriteColorSwizzle swizzleBytes=%i swapWords=%i\n", swizzleBytes ? 1 : 0, swapWords ? 1 : 0);
	//(*pgrLfbWriteColorSwizzle)(swizzleBytes, swapWords);
}

FX_ENTRY void FX_CALL
	grLfbWriteColorFormat(GrColorFormat_t colorFormat)
{
	assert(false && "grLfbWriteColorFormat: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grLfbWriteColorFormat colorFormat=%i\n", colorFormat);
	//(*pgrLfbWriteColorFormat)(colorFormat);
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
	EnsureInitialized();
	D2DX_LOG("grLfbWriteRegion dst_buffer=%i dst_x=%u dst_y=%u src_format=%u src_width=%u src_height=%u pixelPipeline=%i src_stride=%i src_data=%p\n",
		dst_buffer, dst_x, dst_y, src_format, src_width, src_height, pixelPipeline ? 1 : 0, src_stride, src_data);
	//FxBool r = (*pgrLfbWriteRegion)(dst_buffer, dst_x, dst_y, src_format, src_width, src_height, pixelPipeline, src_stride, src_data);
	//return r;
	return FXFALSE;
}

FX_ENTRY FxBool FX_CALL
	grLfbReadRegion(GrBuffer_t src_buffer,
		FxU32 src_x, FxU32 src_y,
		FxU32 src_width, FxU32 src_height,
		FxU32 dst_stride, void* dst_data)
{
	assert(false && "grLfbReadRegion: Unsupported");
	EnsureInitialized();
	//FxBool r = (*pgrLfbReadRegion)(src_buffer, src_x, src_y, src_width, src_height, dst_stride, dst_data);
	//D2DX_LOG("grLfbReadRegion src_buffer=%i src_x=%u src_y=%u src_width=%u src_height=%u dst_stride=%u dst_data=%p -> %i\n",
	//	src_buffer, src_x, src_y, src_width, src_height, dst_stride, dst_data, r ? 1 : 0);
	//return r;
	return FXFALSE;
}

FX_ENTRY void FX_CALL
	grTexMultibase(GrChipID_t tmu,
		FxBool     enable)
{
	assert(false && "grTexMultibase: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grTexMultibase tmu=%i enable=%i\n", tmu, enable);
	//		(*pgrTexMultibase)(tmu, enable);
}

FX_ENTRY void FX_CALL
	grTexMultibaseAddress(GrChipID_t       tmu,
		GrTexBaseRange_t range,
		FxU32            startAddress,
		FxU32            evenOdd,
		GrTexInfo* info)
{
	assert(false && "grTexMultibaseAddress: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grTexMultibaseAddress tmu=%i range=%u startAddress=%08x evenOdd=%u info=%p\n", tmu, range, startAddress, evenOdd, info);
	//(*pgrTexMultibaseAddress)(tmu, range, startAddress, evenOdd, info);
}

FX_ENTRY void FX_CALL
	grDrawTriangle(const void* a, const void* b, const void* c)
{
	assert(false && "grDrawTriangle: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grDrawTriangle a=%p b=%p c=%p\n", a, b, c);

	if (g_d2dxContext->IsDrawingDisabled())
	{
		return;
	}

	//(*pgrDrawTriangle)(a, b, c);
}

FX_ENTRY void FX_CALL
	grAADrawTriangle(
		const void* a, const void* b, const void* c,
		FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias
	)
{
	assert(false && "grAADrawTriangle: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grAADrawTriangle a=%p b=%p c=%p ab_antialias=%i bc_antialias=%i ca_antialias=%i\n", a, b, c, ab_antialias ? 1 : 0, bc_antialias ? 1 : 0, ca_antialias ? 1 : 0);
	//(*pgrAADrawTriangle)(a, b, c, ab_antialias, bc_antialias, ca_antialias);
}

FX_ENTRY void FX_CALL
	grClipWindow(FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy)
{
	assert(false && "grClipWindow: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grClipWindow minx=%u miny=%u maxx=%u maxy=%u\n", minx, miny, maxx, maxy);
	//(*pgrClipWindow)(minx, miny, maxx, maxy);
}

FX_ENTRY void FX_CALL
	grSstOrigin(GrOriginLocation_t origin)
{
	assert(false && "grSstOrigin: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grSstOrigin origin=%i\n", origin);
	//(*pgrSstOrigin)(origin);
}

FX_ENTRY void FX_CALL
	grCullMode(GrCullMode_t mode)
{
	assert(false && "grCullMode: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grCullMode mode=%i\n", mode);
	//(*pgrCullMode)(mode);
}

FX_ENTRY void FX_CALL
	grDepthBiasLevel(FxI32 level)
{
	assert(false && "grDepthBiasLevel: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grDepthBiasLevel level=%i\n", level);
	//(*pgrDepthBiasLevel)(level);
}

FX_ENTRY void FX_CALL
	grDepthBufferFunction(GrCmpFnc_t function)
{
	assert(false && "grDepthBufferFunction: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grDepthBufferFunction function=%i\n", function);
	//(*pgrDepthBufferFunction)(function);
}

FX_ENTRY void FX_CALL
	grDepthBufferMode(GrDepthBufferMode_t mode)
{
	assert(false && "grDepthBufferMode: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grDepthBufferMode mode=%i\n", mode);
	//(*pgrDepthBufferMode)(mode);
}

FX_ENTRY void FX_CALL
	grFogColorValue(GrColor_t fogcolor)
{
	assert(false && "grFogColorValue: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grFogColorValue fogcolor=%u\n", fogcolor);
	//(*pgrFogColorValue)(fogcolor);
}

FX_ENTRY void FX_CALL
	grFogMode(GrFogMode_t mode)
{
	assert(false && "grFogMode: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grFogMode mode=%i\n", mode);
	//(*pgrFogMode)(mode);
}

FX_ENTRY void FX_CALL
	grFogTable(const GrFog_t ft[])
{
	assert(false && "grFogTable: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grFogTable ft=%p\n", ft);
	//(*pgrFogTable)(ft);
}

FX_ENTRY void FX_CALL
	grSplash(float x, float y, float width, float height, FxU32 frame)
{
	assert(false && "grSplash: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grSplash x=%f y=%f width=%f height=%f frame=%u\n", x, y, width, height, frame);
	//(*pgrSplash)(x, y, width, height, frame);
}

FX_ENTRY GrProc FX_CALL
	grGetProcAddress(char* procName)
{
	assert(false && "grGetProcAddress: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grGetProcAddress procName=%p\n", procName);
	//GrProc r = (*pgrGetProcAddress)(procName);
	//return r;
	return NULL;
}

FX_ENTRY void FX_CALL
	grDepthRange(FxFloat n, FxFloat f)
{
	assert(false && "grDepthRange: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grDepthRange n=%f f=%f\n", n, f);
	//(*pgrDepthRange)(n, f);
}

FX_ENTRY void FX_CALL
	grTexNCCTable(GrNCCTable_t table)
{
	assert(false && "grTexNCCTable: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grTexNCCTable table=%u\n",
		table);
	//(*pgrTexNCCTable)(table);
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
	EnsureInitialized();
	D2DX_LOG("grTexDetailControl tmu=%i lod_bias=%i detail_scale=%i detail_max=%f\n",
		tmu, lod_bias, detail_scale, detail_max);
	//(*pgrTexDetailControl)(tmu, lod_bias, detail_scale, detail_max);
}

FX_ENTRY void FX_CALL
	grTexLodBiasValue(GrChipID_t tmu, float bias)
{
	assert(false && "grTexLodBiasValue: Unsupported");
	assert(false);
	EnsureInitialized();
	D2DX_LOG("grTexLodBiasValue tmu=%u bias=%f\n",
		tmu, bias);
	//(*pgrTexLodBiasValue)(tmu, bias);
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
	EnsureInitialized();
	D2DX_LOG("grTexDownloadMipMapLevel tmu=%u startAddress=%08x thisLod=%i largeLod=%i aspectRatio=%i format=%i evenOdd=%u data=%p\n",
		tmu, startAddress, thisLod, largeLod, aspectRatio, format, evenOdd, data);
	//(*pgrTexDownloadMipMapLevel)(tmu, startAddress, thisLod, largeLod, aspectRatio, format, evenOdd, data);
	//FxU32 memRequired = grTexTextureMemRequired(evenOdd, info);
	//uiConnector->OnTexDownload(tmu, startAddress, );
}

FX_ENTRY FxBool FX_CALL
	grTexDownloadMipMapLevelPartial(GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod, GrLOD_t largeLod, GrAspectRatio_t aspectRatio, GrTextureFormat_t format,
		FxU32 evenOdd, void* data, int start, int end)
{
	assert(false && "grTexDownloadMipMapLevelPartial: Unsupported");
	EnsureInitialized();
	//FxBool r = (*pgrTexDownloadMipMapLevelPartial)(tmu, startAddress, thisLod, largeLod, aspectRatio, format, evenOdd, data, start, end);
	//D2DX_LOG("grTexDownloadMipMapLevelPartial tmu=%u startAddress=%08x thisLod=%i largeLod=%i aspectRatio=%i format=%i evenOdd=%u data=%p start=%i end=%i -> %i\n",
		//tmu, startAddress, thisLod, largeLod, aspectRatio, format, evenOdd, data, start, end, r ? 1 : 0);
	//		uiConnector->OnTexDownload(tmu, startAddress);
	//return r;
	return FXFALSE;
}

FX_ENTRY FxU32 FX_CALL
	grTexCalcMemRequired(
		GrLOD_t lodmin, GrLOD_t lodmax,
		GrAspectRatio_t aspect, GrTextureFormat_t fmt)
{
	assert(false && "grTexCalcMemRequired: Unsupported");
	EnsureInitialized();
	return 0;
}

FX_ENTRY FxU32 FX_CALL
	grTexTextureMemRequired(FxU32     evenOdd,
		GrTexInfo* info)
{
	assert(false && "grTexTextureMemRequired: Unsupported");
	EnsureInitialized();
	return 0;
}

FX_ENTRY void FX_CALL
	grDisableAllEffects(void)
{
	assert(false && "grDisableAllEffects: Unsupported");
	EnsureInitialized();
	D2DX_LOG("grDisableAllEffects\n");
}

FX_ENTRY const char* FX_CALL
	grGetString(FxU32 pname)
{
	//assert(false && "grGetString: Unsupported");
	EnsureInitialized();	
	D2DX_LOG("grGetString pname=%u\n", pname);
	return g_d2dxContext->OnGetString(pname);
}

FX_ENTRY void FX_CALL
	grTexDownloadTablePartial(GrTexTable_t type,
		void* data,
		int          start,
		int          end)
{
	assert(false);
	EnsureInitialized();
	D2DX_LOG("grTexDownloadTablePartial type=%i data=%p start=%i end=%i\n", type, data, start, end);
}

}
