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
#include "TextureProcessor.h"
#include "Utils.h"

using namespace d2dx;

TextureProcessor::TextureProcessor()
{
}

TextureProcessor::~TextureProcessor()
{
}

_Use_decl_annotations_
void TextureProcessor::CopyPixels(
	int32_t srcWidth,
	int32_t srcHeight,
	const uint8_t* __restrict srcPixels,
	uint32_t srcPitch,
	uint8_t* __restrict dstPixels,
	uint32_t dstPitch)
{
	const int32_t srcSkip = srcPitch - srcWidth;
	const int32_t dstSkip = dstPitch - srcWidth;

	assert(srcSkip >= 0);
	assert(dstSkip >= 0);

	for (int32_t y = 0; y < srcHeight; ++y)
	{
		for (int32_t x = 0; x < srcWidth; ++x)
		{
			*dstPixels++ = *srcPixels++;
		}
		srcPixels += srcSkip;
		dstPixels += dstSkip;
	}
}

_Use_decl_annotations_
void TextureProcessor::ConvertToRGBA(int32_t width, int32_t height, const uint8_t* __restrict srcPixels, const uint32_t* __restrict palette, uint32_t* __restrict dstPixels)
{
	assert(width <= 256 && height <= 256);
	const uint32_t pixelCount = (uint32_t)(width * height);

	for (uint32_t i = 0; i < pixelCount; ++i)
	{
		dstPixels[i] = palette[srcPixels[i]] | 0xFF000000;
	}
}

_Use_decl_annotations_
void TextureProcessor::ConvertChromaKeyedToRGBA(
	int32_t width,
	int32_t height,
	const uint8_t* __restrict srcPixels,
	const uint32_t* __restrict palette,
	uint32_t* __restrict dstPixels,
	uint32_t dstPitch)
{
	assert(width <= 256 && height <= 256);

	uint32_t skip = dstPitch - width;

	const uint32_t pixelCount = (uint32_t)(width * height);
	for (int32_t y = 0; y < height; ++y)
	{
		for (int32_t x = 0; x < width; ++x)
		{
			const uint32_t c32 = palette[*srcPixels++] | 0xFF000000;
			*dstPixels++ = c32 == 0xFF000000 ? 0 : c32;
		}
		dstPixels += skip;
	}
}

_Use_decl_annotations_
void TextureProcessor::DilateFloorTile(
	int32_t width,
	int32_t height,
	const uint32_t* __restrict srcPixels,
	uint32_t* __restrict dstPixels)
{
	static const int32_t padx[] = { 79, 77, 75, 73, 71, 69, 67, 65, 63, 61, 59, 57, 55, 53, 51, 49, 47, 45, 43, 41, 39, 37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 5, 3, 1 };

	for (int32_t y = 0; y < 40; ++y)
	{
		int32_t padxval = padx[y];
		uint32_t padPixel = srcPixels[padxval + y * width];

		for (int32_t x = 0; x < padxval; ++x)
		{
			dstPixels[x + y * width] = padPixel;
		}

		for (int32_t x = padxval; x < (159 - padxval) + 1; ++x)
		{
			dstPixels[x + y * width] = srcPixels[x + y * width];
		}

		padPixel = srcPixels[(159 - padxval) + y * width];

		for (int32_t x = (159 - padxval) + 1; x < 160; ++x)
		{
			dstPixels[x + y * width] = padPixel;
		}
	}
	for (int32_t y = 40; y < 79; ++y)
	{
		int32_t padxval = padx[78 - y];

		uint32_t padPixel = srcPixels[padxval + y * width];

		for (int32_t x = 0; x < padxval; ++x)
		{
			dstPixels[x + y * width] = padPixel;
		}

		for (int32_t x = padxval; x < (159 - padxval) + 1; ++x)
		{
			dstPixels[x + y * width] = srcPixels[x + y * width];
		}

		padPixel = srcPixels[(159 - padxval) + y * width];

		for (int32_t x = (159 - padxval) + 1; x < 160; ++x)
		{
			dstPixels[x + y * width] = padPixel;
		}
	}
}

_Use_decl_annotations_
void TextureProcessor::Dilate(
	int32_t width,
	int32_t height,
	uint32_t* __restrict pixels)
{
	const uint32_t pixelCount = (uint32_t)(width * height);

	for (int32_t y = 0; y < (height - 1); ++y)
	{
		uint32_t* pRow = &pixels[y * width];
		for (int32_t x = 0; x < (width - 1); ++x)
		{
			uint32_t c00 = pRow[x];
			uint32_t c10 = pRow[x + 1];
			uint32_t c11 = pRow[x + 1 + width];
			uint32_t c01 = pRow[x + width];

			uint32_t a00 = c00 >> 24;
			uint32_t a10 = c10 >> 24;
			uint32_t a11 = c11 >> 24;
			uint32_t a01 = c01 >> 24;

			if (a00 == 0)
			{
				if (a10 > a11)
				{
					if (a01 > a10)
					{
						c00 = c01;
					}
					else
					{
						c00 = c10;
					}
				}
				else
				{
					c00 = c11;
				}

				pRow[x] = c00;
			}
		}
	}

	uint32_t* pRow = &pixels[(height - 1) * width];
	for (int32_t x = 0; x < (width - 1); ++x)
	{
		const uint32_t c00 = pRow[x];
		const uint32_t c10 = pRow[x + 1];
		const uint32_t a00 = c00 >> 24;
		if (a00 == 0)
		{
			pRow[x] = c10;
		}
	}

	for (int32_t y = (height - 1); y >= 1; --y)
	{
		uint32_t* pRow = &pixels[y * width];
		for (int32_t x = (width - 1); x >= 1; --x)
		{
			uint32_t c00 = pRow[x];
			uint32_t c10 = pRow[x - 1];
			uint32_t c11 = pRow[x - 1 - width];
			uint32_t c01 = pRow[x - width];

			uint32_t a00 = c00 >> 24;
			uint32_t a10 = c10 >> 24;
			uint32_t a11 = c11 >> 24;
			uint32_t a01 = c01 >> 24;

			if (a00 == 0)
			{
				if (a10 > a11)
				{
					if (a01 > a10)
					{
						c00 = c01;
					}
					else
					{
						c00 = c10;
					}
				}
				else
				{
					c00 = c11;
				}

				pRow[x] = c00;
			}
		}

		pRow = pixels;
		for (int32_t x = width - 1; x >= 1; --x)
		{
			const uint32_t c00 = pRow[x];
			const uint32_t c10 = pRow[x - 1];
			const uint32_t a00 = c00 >> 24;
			if (a00 == 0)
			{
				pRow[x] = c10;
			}
		}
	}
}

_Use_decl_annotations_
void TextureProcessor::CopyPixels(
	int32_t width,
	int32_t height,
	const uint32_t* __restrict srcPixels,
	uint32_t* __restrict dstPixels)
{
	const uint32_t pixelCount = (uint32_t)(width * height);
	memcpy(dstPixels, srcPixels, pixelCount * sizeof(uint32_t));
}

_Use_decl_annotations_
void TextureProcessor::CopyAlpha(
	int32_t width,
	int32_t height,
	const uint32_t* __restrict srcPixels,
	uint32_t* __restrict dstPixels)
{
	const uint32_t pixelCount = (uint32_t)(width * height);

	for (uint32_t i = 0; i < pixelCount; ++i)
	{
		const uint32_t a = srcPixels[i] & 0xFF000000;
		const uint32_t c = dstPixels[i] & 0x00FFFFFF;
		dstPixels[i] = c | a;
	}
}
