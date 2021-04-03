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
#define D2DX_GLIDE_ALPHA_BLEND(rgb_sf, rgb_df, alpha_sf, alpha_df) \
		(uint16_t)(((rgb_sf & 0xF) << 12) | ((rgb_df & 0xF) << 8) | ((alpha_sf & 0xF) << 4) | (alpha_df & 0xF))

#define XGR_PARAMS \
	XGR_PARAM(GR_PARAM_XY) \
	XGR_PARAM(GR_PARAM_Z) \
	XGR_PARAM(GR_PARAM_W) \
	XGR_PARAM(GR_PARAM_Q) \
	XGR_PARAM(GR_PARAM_FOG_EXT) \
	XGR_PARAM(GR_PARAM_A) \
	XGR_PARAM(GR_PARAM_RGB) \
	XGR_PARAM(GR_PARAM_PARGB) \
	XGR_PARAM(GR_PARAM_ST0) \
	XGR_PARAM(GR_PARAM_ST1) \
	XGR_PARAM(GR_PARAM_ST2) \
	XGR_PARAM(GR_PARAM_Q0) \
	XGR_PARAM(GR_PARAM_Q1) \
	XGR_PARAM(GR_PARAM_Q2)

#define XGR_COMBINE_FACTORS \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_ZERO) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_LOCAL) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_OTHER_ALPHA) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_LOCAL_ALPHA) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_TEXTURE_ALPHA) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_TEXTURE_RGB) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_ONE) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_ONE_MINUS_LOCAL) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA) \
	XGR_COMBINE_FACTOR(GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION)

#define XGR_COMBINE_FUNCTIONS \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_ZERO) \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_LOCAL) \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_LOCAL_ALPHA) \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_SCALE_OTHER) \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL) \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA) \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL) \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL) \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA) \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL) \
	XGR_COMBINE_FUNCTION(GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA)

#define XGR_COMBINE_LOCALS \
	XGR_COMBINE_LOCAL(GR_COMBINE_LOCAL_ITERATED) \
	XGR_COMBINE_LOCAL(GR_COMBINE_LOCAL_CONSTANT) \
	XGR_COMBINE_LOCAL(GR_COMBINE_LOCAL_DEPTH)

#define XGR_COMBINE_OTHERS \
	XGR_COMBINE_OTHER(GR_COMBINE_OTHER_ITERATED) \
	XGR_COMBINE_OTHER(GR_COMBINE_OTHER_TEXTURE) \
	XGR_COMBINE_OTHER(GR_COMBINE_OTHER_CONSTANT)

	static const char* GetPrimitiveTypeString(FxU32 mode)
	{
		static const char* str[] = {
			"GR_POINTS",
			"GR_LINE_STRIP",
			"GR_LINES",
			"GR_POLYGON",
			"GR_TRIANGLE_STRIP",
			"GR_TRIANGLE_FAN",
			"GR_TRIANGLES",
			"GR_TRIANGLE_STRIP_CONTINUE",
			"GR_TRIANGLE_FAN_CONTINUE"
		};
		return str[mode];
	}

	static const char* GetAlphaBlendFncString(GrAlphaBlendFnc_t fnc)
	{
		static const char* str[16] = {
			"GR_BLEND_ZERO",
			"GR_BLEND_SRC_ALPHA",
			"GR_BLEND_SRC_COLOR",
			"GR_BLEND_DST_ALPHA",
			"GR_BLEND_ONE",
			"GR_BLEND_ONE_MINUS_SRC_ALPHA",
			"GR_BLEND_ONE_MINUS_SRC_COLOR",
			"GR_BLEND_ONE_MINUS_DST_ALPHA",
			"GR_BLEND_RESERVED_8",
			"GR_BLEND_RESERVED_9",
			"GR_BLEND_RESERVED_A",
			"GR_BLEND_RESERVED_B",
			"GR_BLEND_RESERVED_C",
			"GR_BLEND_RESERVED_D",
			"GR_BLEND_RESERVED_E",
			"GR_BLEND_ALPHA_SATURATE"
		};
		return str[fnc];
	}

	static const char* GetVertexLayoutParamString(FxU32 param)
	{
		switch (param) {
#define XGR_PARAM(x) case x: return #x;
			XGR_PARAMS
#undef XGR_PARAM
		default: return "";
		}
	}

	static const char* GetCombineLocalString(GrCombineLocal_t local)
	{
		switch (local)
		{
#define XGR_COMBINE_LOCAL(x) case x: return #x;
			XGR_COMBINE_LOCALS
#undef XGR_COMBINE_LOCAL
		default: return "";
		}
	}

	static const char* GetCombineOtherString(GrCombineOther_t other)
	{
		switch (other)
		{
#define XGR_COMBINE_OTHER(x) case x: return #x;
			XGR_COMBINE_OTHERS
#undef XGR_COMBINE_OTHER
		default: return "";
		}
	}

	static const char* GetCombineFactorString(GrCombineFactor_t factor)
	{
		switch (factor)
		{
#define XGR_COMBINE_FACTOR(x) case x: return #x;
			XGR_COMBINE_FACTORS
#undef XGR_COMBINE_FACTOR
		default: return "";
		}
	}

	static const char* GetCombineFunctionString(GrCombineFunction_t fnc)
	{
		switch (fnc)
		{
#define XGR_COMBINE_FUNCTION(x) case x: return #x;
			XGR_COMBINE_FUNCTIONS
#undef XGR_COMBINE_FUNCTION
		default: return "";
		}
	}

	static const char* GetStringForTextureFilterMode(GrTextureFilterMode_t fm)
	{
		switch (fm) {
		case GR_TEXTUREFILTER_POINT_SAMPLED:
			return "GR_TEXTUREFILTER_POINT_SAMPLED";
		case GR_TEXTUREFILTER_BILINEAR:
			return "GR_TEXTUREFILTER_BILINEAR";
		default:
			return "";
		}
	}

	static const char* GetTextureFormatString(GrTextureFormat_t fmt)
	{
		static const char* str[16] = {
			"GR_TEXFMT_8BIT",
			"GR_TEXFMT_YIQ_422",
			"GR_TEXFMT_ALPHA_8",
			"GR_TEXFMT_INTENSITY_8",
			"GR_TEXFMT_ALPHA_INTENSITY_44",
			"GR_TEXFMT_P_8",
			"GR_TEXFMT_RSVD0",
			"GR_TEXFMT_RSVD1",
			"GR_TEXFMT_16BIT",
			"GR_TEXFMT_AYIQ_8422",
			"GR_TEXFMT_RGB_565",
			"GR_TEXFMT_ARGB_1555",
			"GR_TEXFMT_ARGB_4444",
			"GR_TEXFMT_ALPHA_INTENSITY_88",
			"GR_TEXFMT_AP_88",
			"GR_TEXFMT_RSVD2"
		};
		return str[fmt];
	}

	static const char* GetCoordinateSpaceModeString(GrCoordinateSpaceMode_t mode)
	{
		if (mode == GR_WINDOW_COORDS) return "GR_WINDOW_COORDS";
		if (mode == GR_CLIP_COORDS) return "GR_CLIP_COORDS";
		return "";
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

	static uint32_t GetTextureMemRequired(GrTexInfo* info)
	{
		FxU32 width, height;
		GetWidthHeightFromTexInfo(info, &width, &height);
		assert(info->format == GR_TEXFMT_P_8);
		return width * height;
	}

	static PrimitiveType GetPrimitiveType(uint32_t mode)
	{
		switch (mode)
		{
		case GR_TRIANGLES:
		case GR_TRIANGLE_FAN:
		case GR_TRIANGLE_STRIP:
		case GR_TRIANGLE_FAN_CONTINUE:
		case GR_TRIANGLE_STRIP_CONTINUE:
			return PrimitiveType::Triangles;
		case GR_LINES:
			return PrimitiveType::Lines;
		case GR_POINTS:
			return PrimitiveType::Points;
		default:
			assert(false && "Unhandled primitive type.");
			return PrimitiveType::Points;
		}
	}
}
