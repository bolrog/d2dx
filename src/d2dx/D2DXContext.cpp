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
#include "D2DXContext.h"
#include "GameHelper.h"
#include "GlideHelpers.h"
#include "Simd.h"
#include "Utils.h"
#include "dx_raw.h"

using namespace d2dx;
using namespace DirectX::PackedVector;
using namespace std;

D2DXContext::D2DXContext() :
	_renderFilter(0),
	_capturingFrame(false),
	_frame(0),
	_majorGameState(MajorGameState::Unknown),
	_vertexLayout(0xFF),
	_currentFrameIndex(0),
	_tmuMemory(D2DX_TMU_MEMORY_SIZE),
	_sideTmuMemory(D2DX_SIDE_TMU_MEMORY_SIZE),
	_constantColor(0xFFFFFFFF),
	_paletteKeys(D2DX_MAX_PALETTES),
	_palettes(256 * D2DX_MAX_PALETTES)
{
	memset(_paletteKeys.items, 0, sizeof(uint32_t) * _paletteKeys.capacity);
	memset(_palettes.items, 0, sizeof(uint32_t) * _palettes.capacity);
	_paletteKeys.items[0] = 0xFFFFFFFF;

	_gammaTable = new uint32_t[256];

	const char* commandLine = GetCommandLineA();
	bool windowed = strstr(commandLine, "-w") != nullptr;
	bool gx1080 = strstr(commandLine, "-gx1080") != nullptr;
	_options.skipLogo = strstr(commandLine, "-gxskiplogo") != nullptr;

	bool gxmaximize = strstr(commandLine, "-gxmaximize") != nullptr;
	bool gxscale2 = strstr(commandLine, "-gxscale2") != nullptr;
	bool gxscale3 = strstr(commandLine, "-gxscale3") != nullptr;
	_options.defaultZoomLevel =
		gxmaximize ? 0 :
		gxscale3 ? 3 :
		gxscale2 ? 2 :
		1;

	if (gx1080)
	{
		_options.screenMode = ScreenMode::Fullscreen1920x1080;

		if (windowed)
		{
			MessageBoxA(NULL, "Can't combine -gx1080 with -w.", "D2DX", MB_OK);
			PostQuitMessage(1);
			return;
		}
	}
	else if (windowed)
	{
		_options.screenMode = ScreenMode::Windowed;
	}
	else
	{
		_options.screenMode = ScreenMode::FullscreenDefault;
	}
}

D2DXContext::~D2DXContext()
{
	delete[] _gammaTable;
	_gammaTable = nullptr;
}

const char* D2DXContext::OnGetString(uint32_t pname)
{
	switch (pname)
	{
	case GR_EXTENSION:
		return " ";
	case GR_HARDWARE:
		return "Banshee";
	case GR_RENDERER:
		return "Glide";
	case GR_VENDOR:
		return "3Dfx Interactive";
	case GR_VERSION:
		return "3.0";
	}
	return NULL;
}

uint32_t D2DXContext::OnGet(uint32_t pname, uint32_t plength, int32_t* params)
{
	switch (pname)
	{
	case GR_MAX_TEXTURE_SIZE:
		*params = 256;
		return 4;
	case GR_MAX_TEXTURE_ASPECT_RATIO:
		*params = 3;
		return 4;
	case GR_NUM_BOARDS:
		*params = 1;
		return 4;
	case GR_NUM_FB:
		*params = 1;
		return 4;
	case GR_NUM_TMU:
		*params = 1;
		return 4;
	case GR_TEXTURE_ALIGN:
		*params = 256;
		return 4;
	case GR_MEMORY_UMA:
		*params = 0;
		return 4;
	case GR_GAMMA_TABLE_ENTRIES:
		*params = 256;
		return 4;
	case GR_BITS_GAMMA:
		*params = 8;
		return 4;
	default:
		return 0;
	}
}

bool D2DXContext::IsDrawingDisabled() const
{
	return false;
}

bool D2DXContext::IsCapturingFrame() const
{
	return _capturingFrame;
}

void D2DXContext::LogGlideCall(const char* s)
{
}

void D2DXContext::OnGlideInit()
{
}

void D2DXContext::OnGlideShutdown()
{
}

void D2DXContext::OnSstWinOpen(uint32_t hWnd, int32_t width, int32_t height)
{
	int32_t windowWidth, windowHeight;
	_gameHelper.GetConfiguredGameSize(&windowWidth, &windowHeight);

	if (_customWidth > 0)
	{
		width = _customWidth;
		height = _customHeight;
	}

	if (width > 800)
	{
		windowWidth = width;
		windowHeight = height;
	}

	if (!_d3d11Context)
	{
		auto simd = Simd::Create();
		auto textureProcessor = make_shared<TextureProcessor>();
		_d3d11Context = make_unique<D3D11Context>((HWND)hWnd, windowWidth * _options.defaultZoomLevel, windowHeight * _options.defaultZoomLevel, width, height, _options, simd, textureProcessor);
	}
	else
	{
		_d3d11Context->SetWindowSize(windowWidth * _options.defaultZoomLevel, windowHeight * _options.defaultZoomLevel);
		_d3d11Context->SetGameSize(width, height);
	}

	_frames[0]._batchCount = 0;
	_frames[1]._batchCount = 0;
	_frames[0]._vertexCount = 0;
	_frames[1]._vertexCount = 0;
	_scratchBatch = Batch();
}

void D2DXContext::OnVertexLayout(uint32_t param, int32_t offset)
{
	switch (param) {
	case GR_PARAM_XY:
		_vertexLayout = (_vertexLayout & 0xFFFF) | ((offset & 0xFF) << 16);
		break;
	case GR_PARAM_ST0:
	case GR_PARAM_ST1:
		_vertexLayout = (_vertexLayout & 0xFF00FF) | ((offset & 0xFF) << 8);
		break;
	case GR_PARAM_PARGB:
		_vertexLayout = (_vertexLayout & 0xFFFF00) | (offset & 0xFF);
		break;
	}
}

//#include <sys/stat.h>   // stat
//#include <stdbool.h>    // bool type
////supply an array of pixels[height][width] <- notice that height comes first
//int writeBMP(char* filename, unsigned int width, unsigned int height, uint8_t* pixels) {
//	FILE* fp;
//	fopen_s(&fp, filename, "wb");
//	static unsigned char header[54] = { 66,77,0,0,0,0,0,0,0,0,54,0,0,0,40,0,0,0,0,0,0,0,0,0,0,0,1,0,24 }; //rest is zeroes
//	unsigned int pixelBytesPerRow = width * 3;
//	unsigned int paddingBytesPerRow = (4 - (pixelBytesPerRow % 4)) % 4;
//	unsigned int* sizeOfFileEntry = (unsigned int*)&header[2];
//	*sizeOfFileEntry = 54 + (pixelBytesPerRow + paddingBytesPerRow) * height;
//	unsigned int* widthEntry = (unsigned int*)&header[18];
//	*widthEntry = width;
//	unsigned int* heightEntry = (unsigned int*)&header[22];
//	*heightEntry = height;
//	fwrite(header, 54, 1, fp);
//	static unsigned char zeroes[3] = { 0,0,0 }; //for padding    
//	for (uint32_t row = 0; row < height; row++) {
//		fwrite(pixels, pixelBytesPerRow, 1, fp);
//		fwrite(zeroes, paddingBytesPerRow, 1, fp);
//		pixels += 3 * width;
//	}
//	fclose(fp);
//	return 0;
//}
//bool file_exists(char* filename) {
//	struct stat   buffer;
//	return (stat(filename, &buffer) == 0);
//}
//unsigned char const act1pal[] = {
//		0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x08, 0x18, 0x1c, 0x10, 0x24, 0x2c, 0x18, 0x34, 0x3c, 0x00,
//		0x00, 0x5c, 0x20, 0x40, 0x48, 0x28, 0x48, 0x54, 0x00, 0x00, 0x90, 0x10, 0x48, 0x8c, 0x00, 0x00,
//		0xbc, 0x20, 0x84, 0xd0, 0x6c, 0xc4, 0xf4, 0x50, 0x7c, 0x8c, 0x64, 0x9c, 0xac, 0x08, 0x0c, 0x0c,
//		0x10, 0x10, 0x14, 0x1c, 0x1c, 0x1c, 0x1c, 0x24, 0x28, 0x2c, 0x2c, 0x2c, 0x30, 0x38, 0x3c, 0x38,
//		0x38, 0x38, 0x48, 0x48, 0x48, 0x48, 0x50, 0x58, 0x34, 0x58, 0x64, 0x58, 0x58, 0x58, 0x3c, 0x64,
//		0x74, 0x64, 0x64, 0x64, 0x70, 0x74, 0x7c, 0x84, 0x84, 0x84, 0x94, 0x98, 0x9c, 0xc4, 0xc4, 0xc4,
//		0xf4, 0xf4, 0xf4, 0x04, 0x04, 0x08, 0x00, 0x04, 0x10, 0x04, 0x08, 0x18, 0x08, 0x10, 0x18, 0x10,
//		0x14, 0x1c, 0x04, 0x0c, 0x24, 0x0c, 0x18, 0x20, 0x14, 0x18, 0x20, 0x08, 0x10, 0x2c, 0x10, 0x1c,
//		0x24, 0x0c, 0x20, 0x28, 0x04, 0x08, 0x38, 0x10, 0x1c, 0x30, 0x14, 0x28, 0x30, 0x0c, 0x14, 0x40,
//		0x10, 0x28, 0x38, 0x04, 0x0c, 0x48, 0x1c, 0x28, 0x38, 0x0c, 0x20, 0x4c, 0x14, 0x2c, 0x44, 0x20,
//		0x2c, 0x40, 0x08, 0x10, 0x58, 0x20, 0x34, 0x48, 0x28, 0x34, 0x44, 0x1c, 0x28, 0x54, 0x14, 0x1c,
//		0x5c, 0x08, 0x24, 0x5c, 0x18, 0x38, 0x54, 0x24, 0x38, 0x54, 0x10, 0x18, 0x6c, 0x10, 0x2c, 0x68,
//		0x24, 0x44, 0x5c, 0x18, 0x24, 0x70, 0x24, 0x3c, 0x68, 0x0c, 0x2c, 0x7c, 0x2c, 0x4c, 0x64, 0x08,
//		0x48, 0x70, 0x18, 0x28, 0x80, 0x28, 0x50, 0x74, 0x24, 0x30, 0x88, 0x40, 0x50, 0x6c, 0x0c, 0x34,
//		0x8c, 0x68, 0x20, 0x70, 0x44, 0x58, 0x78, 0x40, 0x40, 0x8c, 0x30, 0x5c, 0x84, 0x24, 0x34, 0x9c,
//		0x18, 0x28, 0xa4, 0x14, 0x48, 0xa4, 0x40, 0x64, 0x8c, 0x38, 0x70, 0x8c, 0x50, 0x68, 0x8c, 0x34,
//		0x44, 0xb0, 0x20, 0x4c, 0xb4, 0x58, 0x70, 0x98, 0x40, 0x78, 0xa0, 0x1c, 0x60, 0xbc, 0x48, 0x84,
//		0x9c, 0x54, 0x54, 0xc4, 0x20, 0x6c, 0xc8, 0x58, 0x7c, 0xac, 0x48, 0x88, 0xb0, 0x54, 0x7c, 0xc8,
//		0x20, 0x70, 0xe0, 0x54, 0x9c, 0xb8, 0x00, 0x2c, 0xfc, 0x70, 0x8c, 0xc0, 0x50, 0x98, 0xcc, 0x30,
//		0x84, 0xe4, 0x70, 0x70, 0xe0, 0x78, 0x98, 0xd0, 0x3c, 0x88, 0xf8, 0x38, 0xa0, 0xec, 0x64, 0xb8,
//		0xd8, 0x84, 0xa4, 0xe0, 0x44, 0xb4, 0xf0, 0x4c, 0xc0, 0xf4, 0x8c, 0xb0, 0xf0, 0x5c, 0xd4, 0xfc,
//		0xb0, 0xb0, 0xfc, 0x10, 0x24, 0x04, 0x18, 0x24, 0x14, 0x18, 0x3c, 0x20, 0x0c, 0x48, 0x18, 0x30,
//		0x44, 0x00, 0x08, 0x64, 0x18, 0x24, 0x5c, 0x24, 0x24, 0x5c, 0x38, 0x44, 0x6c, 0x08, 0x14, 0x7c,
//		0x28, 0x34, 0x74, 0x40, 0x30, 0x78, 0x58, 0x1c, 0x9c, 0x34, 0x38, 0x84, 0x70, 0x34, 0xa0, 0x48,
//		0x4c, 0x90, 0x58, 0x28, 0xbc, 0x44, 0x48, 0x98, 0x84, 0x4c, 0xb8, 0x60, 0x00, 0xfc, 0x18, 0x5c,
//		0xdc, 0x74, 0x7c, 0xd0, 0x8c, 0x88, 0xfc, 0xa0, 0x28, 0x0c, 0x0c, 0x48, 0x18, 0x18, 0x58, 0x00,
//		0x00, 0x44, 0x14, 0x38, 0x68, 0x24, 0x10, 0x64, 0x3c, 0x28, 0x78, 0x28, 0x28, 0x80, 0x10, 0x4c,
//		0x84, 0x4c, 0x38, 0x94, 0x30, 0x30, 0x8c, 0x60, 0x48, 0xa0, 0x5c, 0x38, 0xac, 0x50, 0x50, 0xac,
//		0x6c, 0x4c, 0xbc, 0x78, 0x54, 0xd8, 0x60, 0x24, 0xd0, 0x78, 0x64, 0xe0, 0x90, 0x64, 0xdc, 0xa0,
//		0x80, 0xfc, 0x20, 0xa4, 0xf0, 0x84, 0x84, 0xfc, 0xa0, 0xa0, 0xfc, 0xb8, 0x90, 0x90, 0x8c, 0x58,
//		0xa4, 0xa0, 0x68, 0xc4, 0xc0, 0x84, 0xd4, 0xd0, 0x98, 0xfc, 0xcc, 0xa8, 0xf4, 0xf4, 0xcc, 0x80,
//		0xa0, 0xc0, 0xa8, 0xc0, 0xc4, 0x94, 0xc4, 0xe0, 0x74, 0xe8, 0xfc, 0xb0, 0xfc, 0xc4, 0xa4, 0xe4,
//		0xfc, 0xc4, 0xfc, 0xfc, 0x04, 0x04, 0x04, 0x08, 0x08, 0x08, 0x0c, 0x0c, 0x0c, 0x10, 0x10, 0x10,
//		0x14, 0x14, 0x14, 0x18, 0x18, 0x18, 0x18, 0x1c, 0x24, 0x20, 0x20, 0x20, 0x24, 0x24, 0x24, 0x28,
//		0x28, 0x28, 0x20, 0x28, 0x30, 0x30, 0x30, 0x30, 0x28, 0x30, 0x38, 0x34, 0x34, 0x34, 0x3c, 0x38,
//		0x34, 0x34, 0x38, 0x44, 0x3c, 0x3c, 0x3c, 0x30, 0x3c, 0x4c, 0x40, 0x40, 0x40, 0x3c, 0x40, 0x44,
//		0x44, 0x44, 0x44, 0x3c, 0x48, 0x50, 0x38, 0x44, 0x58, 0x4c, 0x4c, 0x4c, 0x3c, 0x4c, 0x60, 0x5c,
//		0x58, 0x34, 0x50, 0x50, 0x50, 0x54, 0x54, 0x54, 0x5c, 0x5c, 0x5c, 0x50, 0x5c, 0x68, 0x60, 0x60,
//		0x60, 0x74, 0x70, 0x44, 0x50, 0x64, 0x7c, 0x68, 0x68, 0x68, 0x6c, 0x6c, 0x6c, 0x60, 0x6c, 0x78,
//		0x70, 0x70, 0x70, 0x74, 0x74, 0x74, 0x7c, 0x7c, 0x7c, 0x64, 0x80, 0x94, 0x74, 0x84, 0x90, 0x70,
//		0x88, 0xac, 0x90, 0x90, 0x90, 0x84, 0x94, 0x9c, 0xb8, 0x94, 0x80, 0xa0, 0xa0, 0xa0, 0x98, 0xac,
//		0xb0, 0xac, 0xac, 0xac, 0xb8, 0xb8, 0xb8, 0xcc, 0xcc, 0xcc, 0xd8, 0xd8, 0xd8, 0xfc, 0xcc, 0xcc,
//		0xe4, 0xe4, 0xe4, 0x80, 0x00, 0xfc, 0x08, 0x14, 0x10, 0x08, 0x1c, 0x14, 0x08, 0x20, 0x14, 0x0c,
//		0x24, 0x18, 0x0c, 0x2c, 0x20, 0x18, 0x34, 0x20, 0x1c, 0x38, 0x30, 0x44, 0x20, 0x00, 0x68, 0x00,
//		0x18, 0x04, 0x08, 0x08, 0x10, 0x14, 0x14, 0x14, 0x18, 0x18, 0x14, 0x1c, 0x1c, 0x10, 0x20, 0x1c,
//		0x18, 0x20, 0x24, 0x14, 0x28, 0x24, 0x20, 0x28, 0x28, 0x24, 0x28, 0x2c, 0x30, 0x2c, 0x20, 0x24,
//		0x2c, 0x30, 0x28, 0x30, 0x34, 0x2c, 0x34, 0x38, 0x40, 0x3c, 0x30, 0x34, 0x3c, 0x40, 0x38, 0x44,
//		0x48, 0x44, 0x4c, 0x50, 0x4c, 0x58, 0x5c, 0x54, 0x5c, 0x60, 0x58, 0x64, 0x68, 0xff, 0xff, 0xff };
//
//void dumpTex(const uint8_t* sourceAddress, uint32_t memRequired, uint32_t width, uint32_t height, uint32_t hash)
//{
//	auto tc = _gameHelper.GetTextureCategoryFromHash(hash);
//
//	char fn[256];
//	if (tc == TextureCategory::InGamePanel)
//		sprintf_s(fn, "img/ingamepanel/%08x.bmp", hash);
//	else if (tc == TextureCategory::MousePointer)
//		sprintf_s(fn, "img/mousepointer/%08x.bmp", hash);
//	else if (tc == TextureCategory::Font)
//		sprintf_s(fn, "img/fonts/%08x.bmp", hash);
//	else if (tc == TextureCategory::Item)
//		sprintf_s(fn, "img/items/%08x.bmp", hash);
//	else if (tc == TextureCategory::LoadingScreen)
//		sprintf_s(fn, "img/loadingscreen/%08x.bmp", hash);
//	else if (tc == TextureCategory::FlamingLogo)
//		sprintf_s(fn, "img/flaminglogo/%08x.bmp", hash);
//	else if (tc == TextureCategory::TitleScreen)
//		sprintf_s(fn, "img/titlescreen/%08x.bmp", hash);
//	else if (tc == TextureCategory::FoldoutNeedingFix)
//		sprintf_s(fn, "img/foldoutneedingfix/%08x.bmp", hash);
//	else
//		sprintf_s(fn, "img/%08x.bmp", hash);
//
//	if (file_exists(fn))
//		return;
//
//	Buffer<uint8_t> pixels(width * height * 3);
//
//	for (int32_t y = 0; y < (int32_t)height; ++y)
//	{
//		for (int32_t x = 0; x < (int32_t)width; ++x)
//		{
//			pixels.items[(x + y * width) * 3 + 0] = act1pal[3 * sourceAddress[x + (height - y - 1) * width] + 0];
//			pixels.items[(x + y * width) * 3 + 1] = act1pal[3 * sourceAddress[x + (height - y - 1) * width] + 1];
//			pixels.items[(x + y * width) * 3 + 2] = act1pal[3 * sourceAddress[x + (height - y - 1) * width] + 2];
//		}
//	}
//
//	writeBMP(fn, width, height, pixels.items);
//}

void D2DXContext::OnTexDownload(uint32_t tmu, const uint8_t* sourceAddress, uint32_t startAddress, int32_t width, int32_t height)
{
	assert(tmu == 0 && (startAddress & 255) == 0);

	uint32_t memRequired = (uint32_t)(width * height);

	auto pStart = _tmuMemory.items + startAddress;
	auto pEnd = _tmuMemory.items + startAddress + memRequired;
	assert(pEnd <= (_tmuMemory.items + _tmuMemory.capacity));
	memcpy_s(pStart, _tmuMemory.capacity - startAddress, sourceAddress, memRequired);
}

void D2DXContext::OnTexSource(uint32_t tmu, uint32_t startAddress, int32_t width, int32_t height)
{
	assert(tmu == 0 && (startAddress & 255) == 0);

	const uint32_t memRequired = width * height;

	uint32_t hash = fnv_32a_buf(_tmuMemory.items + startAddress, memRequired, FNV1_32A_INIT);

	bool isStFlipped = height > width;
	if (isStFlipped)
	{
		std::swap(width, height);
	}

	_scratchBatch.SetTextureStartAddress(startAddress);
	_scratchBatch.SetTextureHash(hash);
	_scratchBatch.SetTextureSize(width, height);
	_scratchBatch.SetIsStFlipped(isStFlipped);
	_scratchBatch.SetTextureCategory(_gameHelper.GetTextureCategoryFromHash(hash));
}

void D2DXContext::CheckMajorGameState()
{
	const int32_t batchCount = (int32_t)_frames[_currentFrameIndex]._batchCount;

	if (_majorGameState == MajorGameState::Unknown && batchCount == 0)
	{
		_majorGameState = MajorGameState::FmvIntro;
	}

	_majorGameState = MajorGameState::Menus;

	if (_gameHelper.ScreenOpenMode() == 3)
	{
		_majorGameState = MajorGameState::InGame;
	}
	else
	{
		for (int32_t i = 0; i < batchCount; ++i)
		{
			const Batch& batch = _frames[_currentFrameIndex]._batches.items[i];

			if ((GameAddress)batch.GetGameAddress() == GameAddress::DrawFloor)
			{
				_majorGameState = MajorGameState::InGame;
				break;
			}
		}

		if (_majorGameState == MajorGameState::Menus)
		{
			for (int32_t i = 0; i < batchCount; ++i)
			{
				const Batch& batch = _frames[_currentFrameIndex]._batches.items[i];
				const float y0 = _frames[_currentFrameIndex]._vertices.items[batch.GetStartVertex()].GetY();

				if (batch.GetHash() == 0x4bea7b80 && y0 >= 550)
				{
					_majorGameState = MajorGameState::TitleScreen;
					break;
				}
			}
		}
	}
}

void D2DXContext::ClassifyBatches()
{
	const int32_t batchCount = (int32_t)_frames[_currentFrameIndex]._batchCount;
	bool hasDrawnWorld = false;

	int32_t mousePointerBatch = -1;

	for (int32_t i = 0; i < batchCount; ++i)
	{
		const Vertex* vertices = _frames[_currentFrameIndex]._vertices.items;
		Batch& batch = _frames[_currentFrameIndex]._batches.items[i];

		const auto& v0 = vertices[batch.GetStartVertex() + 0];
		const auto& v1 = vertices[batch.GetStartVertex() + 1];

		const int32_t y0 = (int32_t)v0.GetY();
		const int32_t s0 = v0.GetS();
		const int32_t t0 = v0.GetT();
		const auto colorCombine = v0.GetRgbCombine();
		const auto alphaCombine = v0.GetAlphaCombine();
		const uint32_t color = v0.GetColor();
		const int32_t y1 = (int32_t)v1.GetY();
		const int32_t s1 = v1.GetS();
		const int32_t t1 = v1.GetT();

		auto gameAddress = (GameAddress)batch.GetGameAddress();

		if (_majorGameState == MajorGameState::Menus)
		{
		}
		else if (_majorGameState == MajorGameState::InGame)
		{
			if (y0 < 40 && s0 == 0x0000 && t0 == 0x0000 &&
				y1 < 40 && s1 == 0x0000 && t1 == 0x0010 &&
				colorCombine == RgbCombine::ColorMultipliedByTexture &&
				alphaCombine == AlphaCombine::One)
			{
				// Monster text
				batch.SetCategory(BatchCategory::TopUI);
			}
			else if (
				y0 == 18 && s0 == 0x0000 && t0 == 0x0000 &&
				y1 == 34 && s1 == 0x0000 && t1 == 0x0000 &&
				colorCombine == RgbCombine::ConstantColor &&
				alphaCombine == AlphaCombine::Texture)
			{
				// Monster health
				batch.SetCategory(BatchCategory::TopUI);
			}
			else if (
				y0 == 0xFFFC && s0 == 0x0000 && t0 == 0x0000 &&
				y1 == 0x003C && s1 == 0x0000 && t1 == 0x0040)
			{
				// Hireling portrait
				batch.SetCategory(BatchCategory::TopUI);
			}
			else if (y0 == 14 && s0 == 0x0000 && t0 == 0x0000 &&
				y1 == 19 && s1 == 0x0000 && t1 == 0x0000 &&
				colorCombine == RgbCombine::ConstantColor &&
				alphaCombine == AlphaCombine::Texture)
			{
				// Hireling health bar
				batch.SetCategory(BatchCategory::TopUI);
			}
			else if (y0 == 56 && s0 == 0x0000 && t0 == 0x0000 &&
				y1 == 72 && s1 == 0x0000 && t1 == 0x0010 &&
				colorCombine == RgbCombine::ColorMultipliedByTexture &&
				alphaCombine == AlphaCombine::One)
			{
				// Hireling name
				batch.SetCategory(BatchCategory::TopUI);
			}
			else if (
				batch.GetTextureCategory() == TextureCategory::LoadingScreen ||
				batch.GetTextureCategory() == TextureCategory::FlamingLogo)
			{
				batch.SetCategory(BatchCategory::UiElement);
			}
			else if (batch.GetTextureCategory() == TextureCategory::MousePointer)
			{
				batch.SetCategory(BatchCategory::UiElement);
			}
			else if (batch.GetTextureCategory() == TextureCategory::InGamePanel)
			{
				batch.SetCategory(BatchCategory::InGamePanel);
			}
			else if (gameAddress == GameAddress::DrawShadow)
			{
				batch.SetCategory(BatchCategory::World);
			}
			else if (gameAddress == GameAddress::DrawFloor)
			{
				batch.SetCategory(BatchCategory::World);
				batch.SetTextureCategory(TextureCategory::Floor);
			}
			else
			{
				switch (gameAddress)
				{
				case GameAddress::DrawWall1:
				case GameAddress::DrawWall2:
					batch.SetCategory(BatchCategory::World);
					batch.SetTextureCategory(TextureCategory::Wall);
					break;
				default:
					break;
				}

				hasDrawnWorld = true;
			}
		}
	}
}

static int16_t replacementFloorTexcoords[] =
{
	80	, 0,
	64	, 8,
	96	, 8,
	80	, 16,
	112	, 16,
	96	, 24,
	128	, 24,
	112	, 32,
	144	, 32,
	128	, 40,
	160	, 40,
	144	, 48,
	64	, 8	,
	48	, 16,
	80	, 16,
	64	, 24,
	96	, 24,
	80	, 32,
	112	, 32,
	96	, 40,
	128	, 40,
	112	, 48,
	144	, 48,
	128	, 56,
	48	, 16,
	32	, 24,
	64	, 24,
	48	, 32,
	80	, 32,
	64	, 40,
	96	, 40,
	80	, 48,
	112	, 48,
	96	, 56,
	128	, 56,
	112	, 64,
	32	, 24,
	16	, 32,
	48	, 32,
	32	, 40,
	64	, 40,
	48	, 48,
	80	, 48,
	64	, 56,
	96	, 56,
	80	, 64,
	112	, 64,
	96	, 72,
	16	, 32,
	0	, 40,
	32	, 40,
	16	, 48,
	48	, 48,
	32	, 56,
	64	, 56,
	48	, 64,
	80	, 64,
	64	, 72,
	96	, 72,
	80	, 78,
};

void D2DXContext::AdjustBatchesIn1080p()
{
	const int32_t batchCount = (int32_t)_frames[_currentFrameIndex]._batchCount;

	for (int32_t i = 0; i < batchCount; ++i)
	{
		Vertex* vertices = _frames[_currentFrameIndex]._vertices.items;
		Batch& batch = _frames[_currentFrameIndex]._batches.items[i];

		if (batch.GetCategory() == BatchCategory::TopUI)
		{
			for (uint32_t j = 0; j < batch.GetVertexCount(); ++j)
			{
				vertices[batch.GetStartVertex() + j].SetY(vertices[batch.GetStartVertex() + j].GetY() + 50);
			}
		}
	}
}

void D2DXContext::DrawBatches()
{
	const int32_t batchCount = (int32_t)_frames[_currentFrameIndex]._batchCount;
	Vertex* vertices = _frames[_currentFrameIndex]._vertices.items;

	Batch mergedBatch;
	int32_t drawCalls = 0;

	for (int32_t i = 0; i < batchCount; ++i)
	{
		const Batch& batch = _frames[_currentFrameIndex]._batches.items[i];

		if (!batch.IsValid())
		{
			DEBUG_PRINT("Skipping batch %i, it is invalid.", i);
			continue;
		}

		if (!mergedBatch.IsValid())
		{
			mergedBatch = batch;
		}
		else
		{
			if (_d3d11Context->GetAtlasForTextureSize(batch) != _d3d11Context->GetAtlasForTextureSize(mergedBatch) ||
				(batch.GetAtlasIndex() / 512) != (mergedBatch.GetAtlasIndex() / 512) ||
				batch.GetAlphaBlend() != mergedBatch.GetAlphaBlend() ||
				batch.GetPrimitiveType() != mergedBatch.GetPrimitiveType() ||
				((mergedBatch.GetVertexCount() + batch.GetVertexCount()) > 65535))
			{
				_d3d11Context->Draw(mergedBatch);
				++drawCalls;
				mergedBatch = batch;
			}
			else
			{
				mergedBatch.SetVertexCount(mergedBatch.GetVertexCount() + batch.GetVertexCount());
			}
		}
	}

	if (mergedBatch.IsValid())
	{
		_d3d11Context->Draw(mergedBatch);
		++drawCalls;
	}


	if (!(_frame & 31))
	{
		DEBUG_PRINT("Nr draw calls: %i", drawCalls);
	}
}

void D2DXContext::OnBufferSwap()
{
	/*DEBUG_PRINT("ScreenOpenMode %u", _gameVariables.ScreenOpenMode());*/
	//DEBUG_PRINT("Ingamemousepos %u %u", _gameVariables.IngameMousePositionX(), _gameVariables.IngameMousePositionY());

	CheckMajorGameState();
	InsertLogoOnTitleScreen();
	ClassifyBatches();

	if (_options.screenMode == ScreenMode::Fullscreen1920x1080)
	{
		AdjustBatchesIn1080p();
	}

	//CorrelateBatches();

	const int32_t batchCount = (int32_t)_frames[_currentFrameIndex]._batchCount;
	Vertex* vertices = _frames[_currentFrameIndex]._vertices.items;

	int32_t hudParts = 0;

	//for (int32_t i = 0; i < batchCount; ++i)
	//{
	//	Batch& batch = _frames[_currentFrameIndex]._batches.items[i];
	//	const int32_t batchVertexCount = batch.GetVertexCount();

	//	if (batch.GetTextureCategory() == TextureCategory::Floor && batchVertexCount == 60)
	//	{
	//		for (int32_t j = 0; j < batchVertexCount; ++j)
	//		{
	//			vertices[batch.GetStartVertex() + j].SetS(replacementFloorTexcoords[j * 2]);
	//			vertices[batch.GetStartVertex() + j].SetT(replacementFloorTexcoords[j * 2 + 1]);
	//		}

	//		batch.SetAlphaBlend(AlphaBlend::Opaque);
	//		batch.SetIsChromaKeyEnabled(false);
	//	}
	//}

	_d3d11Context->BulkWriteVertices(vertices, _frames[_currentFrameIndex]._vertexCount);

	DrawBatches();

	_d3d11Context->Present();

	++_frame;
	_currentFrameIndex = (_currentFrameIndex + 1) & 1;

	_frames[_currentFrameIndex]._batchCount = 0;
	_frames[_currentFrameIndex]._vertexCount = 0;
}

void D2DXContext::OnColorCombine(GrCombineFunction_t function, GrCombineFactor_t factor, GrCombineLocal_t local, GrCombineOther_t other, bool invert)
{
	auto rgbCombine = RgbCombine::ColorMultipliedByTexture;

	if (function == GR_COMBINE_FUNCTION_SCALE_OTHER && factor == GR_COMBINE_FACTOR_LOCAL && local == GR_COMBINE_LOCAL_ITERATED && other == GR_COMBINE_OTHER_TEXTURE)
	{
		rgbCombine = RgbCombine::ColorMultipliedByTexture;
	}
	else if (function == GR_COMBINE_FUNCTION_LOCAL && factor == GR_COMBINE_FACTOR_ZERO && local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT)
	{
		rgbCombine = RgbCombine::ConstantColor;
	}
	else
	{
		assert(false && "Unhandled");
	}

	_scratchBatch.SetRgbCombine(rgbCombine);
}

void D2DXContext::OnAlphaCombine(GrCombineFunction_t function, GrCombineFactor_t factor, GrCombineLocal_t local, GrCombineOther_t other, bool invert)
{
	auto alphaCombine = AlphaCombine::One;

	if (function == GR_COMBINE_FUNCTION_ZERO && factor == GR_COMBINE_FACTOR_ZERO && local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT)
	{
		alphaCombine = AlphaCombine::One;
	}
	else if (function == GR_COMBINE_FUNCTION_LOCAL && factor == GR_COMBINE_FACTOR_ZERO && local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT)
	{
		alphaCombine = AlphaCombine::Texture;
	}
	else
	{
		assert(false && "Unhandled");
	}

	_scratchBatch.SetAlphaCombine(alphaCombine);
}

void D2DXContext::OnConstantColorValue(uint32_t color)
{
	_constantColor = (color >> 8) | (color << 24);
}

void D2DXContext::OnAlphaBlendFunction(GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df, GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df)
{
	auto alphaBlend = AlphaBlend::Opaque;

	switch (D2DX_GLIDE_ALPHA_BLEND(rgb_sf, rgb_df, alpha_sf, alpha_df))
	{
	case D2DX_GLIDE_ALPHA_BLEND(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO, GR_BLEND_ZERO):
	case D2DX_GLIDE_ALPHA_BLEND(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ZERO, GR_BLEND_ZERO):
		alphaBlend = AlphaBlend::SrcAlphaInvSrcAlpha;
		break;
	case D2DX_GLIDE_ALPHA_BLEND(GR_BLEND_ONE, GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO):
		alphaBlend = AlphaBlend::Additive;
		break;
	case D2DX_GLIDE_ALPHA_BLEND(GR_BLEND_ZERO, GR_BLEND_SRC_COLOR, GR_BLEND_ZERO, GR_BLEND_ZERO):
		alphaBlend = AlphaBlend::Multiplicative;
		break;
	}

	_scratchBatch.SetAlphaBlend(alphaBlend);
}


void D2DXContext::OnDrawLine(const void* v1, const void* v2, uint32_t gameContext)
{
	FixIngameMousePosition();

	Vertex* vertices = _frames[_currentFrameIndex]._vertices.items;

	auto gameAddress = _gameHelper.IdentifyGameAddress(gameContext);

	Batch batch = _scratchBatch;
	batch.SetPrimitiveType(PrimitiveType::Triangles);
	batch.SetGameAddress(gameAddress);
	batch.SetStartVertex(_frames[_currentFrameIndex]._vertexCount);
	batch.SetTextureCategory(_gameHelper.RefineTextureCategoryFromGameAddress(batch.GetTextureCategory(), gameAddress));

	TextureCacheLocation textureCacheLocation = _d3d11Context->UpdateTexture(_scratchBatch, _tmuMemory.items, &_palettes.items[256 * _scratchBatch.GetPaletteIndex()]);

	Vertex startVertex = ReadVertex((const uint8_t*)v1, _vertexLayout, batch, textureCacheLocation);
	Vertex endVertex = ReadVertex((const uint8_t*)v2, _vertexLayout, batch, textureCacheLocation);

	float dx = endVertex.GetX() - startVertex.GetX();
	float dy = endVertex.GetY() - startVertex.GetY();
	const float lensqr = dx * dx + dy * dy;
	const float len = lensqr > 0.01f ? sqrtf(lensqr) : 1.0f;
	std::swap(dx, dy);
	dx = -dx;
	const float halfinvlen = 1.0f / (2.0f * len);
	dx *= halfinvlen;
	dy *= halfinvlen;

	Vertex vertex0 = startVertex;
	vertex0.SetX(vertex0.GetX() - dx);
	vertex0.SetY(vertex0.GetY() - dy);

	Vertex vertex1 = startVertex;
	vertex1.SetX(vertex1.GetX() + dx);
	vertex1.SetY(vertex1.GetY() + dy);

	Vertex vertex2 = endVertex;
	vertex2.SetX(vertex2.GetX() - dx);
	vertex2.SetY(vertex2.GetY() - dy);

	Vertex vertex3 = endVertex;
	vertex3.SetX(vertex3.GetX() + dx);
	vertex3.SetY(vertex3.GetY() + dy);

	assert((_frames[_currentFrameIndex]._vertexCount + 6) < _frames[_currentFrameIndex]._vertices.capacity);
	int32_t vertexWriteIndex = _frames[_currentFrameIndex]._vertexCount;
	vertices[vertexWriteIndex++] = vertex0;
	vertices[vertexWriteIndex++] = vertex1;
	vertices[vertexWriteIndex++] = vertex2;
	vertices[vertexWriteIndex++] = vertex1;
	vertices[vertexWriteIndex++] = vertex2;
	vertices[vertexWriteIndex++] = vertex3;
	_frames[_currentFrameIndex]._vertexCount = vertexWriteIndex;

	batch.SetVertexCount(6);

	assert(_frames[_currentFrameIndex]._batchCount < _frames[_currentFrameIndex]._batches.capacity);
	_frames[_currentFrameIndex]._batches.items[_frames[_currentFrameIndex]._batchCount++] = batch;
}

Vertex D2DXContext::ReadVertex(const uint8_t* vertex, uint32_t vertexLayout, Batch& batch, TextureCacheLocation textureCacheLocation)
{
	uint32_t stShift = 0;
	_BitScanReverse((DWORD*)&stShift, max(batch.GetWidth(), batch.GetHeight()));
	stShift = 8 - stShift;

	const int32_t xyOffset = (vertexLayout >> 16) & 0xFF;
	const int32_t stOffset = (vertexLayout >> 8) & 0xFF;
	const int32_t pargbOffset = vertexLayout & 0xFF;

	auto xy = (const float*)(vertex + xyOffset);
	auto st = (const float*)(vertex + stOffset);
	assert((st[0] - floor(st[0])) < 1e6);
	assert((st[1] - floor(st[1])) < 1e6);
	int16_t s = ((int16_t)st[0] >> stShift);
	int16_t t = ((int16_t)st[1] >> stShift);
	if (batch.IsStFlipped())
	{
		std::swap(s, t);
	}
	s += textureCacheLocation.OffsetS;
	t += textureCacheLocation.OffsetT;
	auto pargb = pargbOffset != 0xFF ? *(const uint32_t*)(vertex + pargbOffset) : 0xFFFFFFFF;

	return Vertex(xy[0], xy[1], s, t, batch.SelectColorAndAlpha(pargb, _constantColor), batch.GetRgbCombine(), batch.GetAlphaCombine(), batch.IsChromaKeyEnabled(), textureCacheLocation.ArrayIndex, batch.GetPaletteIndex());
}

void D2DXContext::OnDrawVertexArray(uint32_t mode, uint32_t count, uint8_t** pointers, uint32_t gameContext)
{
	//ALWAYS_PRINT("gc %08x", gameContext);
	FixIngameMousePosition();

	Vertex* vertices = _frames[_currentFrameIndex]._vertices.items;

	auto gameAddress = _gameHelper.IdentifyGameAddress(gameContext);

	Batch batch = _scratchBatch;
	batch.SetPrimitiveType(PrimitiveType::Triangles);
	batch.SetGameAddress(gameAddress);
	batch.SetStartVertex(_frames[_currentFrameIndex]._vertexCount);
	batch.SetTextureCategory(_gameHelper.RefineTextureCategoryFromGameAddress(batch.GetTextureCategory(), gameAddress));

	auto textureCacheLocation = _d3d11Context->UpdateTexture(batch, _tmuMemory.items, &_palettes.items[256 * batch.GetPaletteIndex()]);

	switch (mode)
	{
	case GR_TRIANGLE_FAN:
	{
		Vertex firstVertex = ReadVertex((const uint8_t*)pointers[0], _vertexLayout, batch, textureCacheLocation);
		Vertex prevVertex = ReadVertex((const uint8_t*)pointers[1], _vertexLayout, batch, textureCacheLocation);

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex((const uint8_t*)pointers[i], _vertexLayout, batch, textureCacheLocation);

			assert((_frames[_currentFrameIndex]._vertexCount + 3) < _frames[_currentFrameIndex]._vertices.capacity);
			int32_t vertexWriteIndex = _frames[_currentFrameIndex]._vertexCount;
			vertices[vertexWriteIndex++] = firstVertex;
			vertices[vertexWriteIndex++] = prevVertex;
			vertices[vertexWriteIndex++] = currentVertex;
			_frames[_currentFrameIndex]._vertexCount = vertexWriteIndex;

			prevVertex = currentVertex;
			batch.SetVertexCount(batch.GetVertexCount() + 3);
		}
		break;
	}
	case GR_TRIANGLE_STRIP:
	{
		Vertex prevPrevVertex = ReadVertex((const uint8_t*)pointers[0], _vertexLayout, batch, textureCacheLocation);
		Vertex prevVertex = ReadVertex((const uint8_t*)pointers[1], _vertexLayout, batch, textureCacheLocation);

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex((const uint8_t*)pointers[i], _vertexLayout, batch, textureCacheLocation);

			assert((_frames[_currentFrameIndex]._vertexCount + 3) < _frames[_currentFrameIndex]._vertices.capacity);
			int32_t vertexWriteIndex = _frames[_currentFrameIndex]._vertexCount;
			vertices[vertexWriteIndex++] = prevPrevVertex;
			vertices[vertexWriteIndex++] = prevVertex;
			vertices[vertexWriteIndex++] = currentVertex;
			_frames[_currentFrameIndex]._vertexCount = vertexWriteIndex;

			prevPrevVertex = prevVertex;
			prevVertex = currentVertex;
			batch.SetVertexCount(batch.GetVertexCount() + 3);
		}
		break;
	}
	default:
		assert(false && "Unhandled primitive type.");
	}

	assert(_frames[_currentFrameIndex]._batchCount < _frames[_currentFrameIndex]._batches.capacity);
	_frames[_currentFrameIndex]._batches.items[_frames[_currentFrameIndex]._batchCount++] = batch;
}

void D2DXContext::OnDrawVertexArrayContiguous(uint32_t mode, uint32_t count, uint8_t* vertex, uint32_t stride, uint32_t gameContext)
{
	//ALWAYS_PRINT("gc %08x", gameContext);
	FixIngameMousePosition();

	Vertex* vertices = _frames[_currentFrameIndex]._vertices.items;

	auto gameAddress = _gameHelper.IdentifyGameAddress(gameContext);

	Batch batch = _scratchBatch;
	batch.SetPrimitiveType(PrimitiveType::Triangles);
	batch.SetGameAddress(gameAddress);
	batch.SetStartVertex(_frames[_currentFrameIndex]._vertexCount);
	batch.SetTextureCategory(_gameHelper.RefineTextureCategoryFromGameAddress(batch.GetTextureCategory(), gameAddress));

	auto textureCacheLocation = _d3d11Context->UpdateTexture(batch, _tmuMemory.items, &_palettes.items[256 * batch.GetPaletteIndex()]);

	switch (mode)
	{
	case GR_TRIANGLE_FAN:
	{
		Vertex firstVertex = ReadVertex(vertex, _vertexLayout, batch, textureCacheLocation);
		vertex += stride;

		Vertex prevVertex = ReadVertex(vertex, _vertexLayout, batch, textureCacheLocation);
		vertex += stride;

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex(vertex, _vertexLayout, batch, textureCacheLocation);
			vertex += stride;

			assert((_frames[_currentFrameIndex]._vertexCount + 3) < _frames[_currentFrameIndex]._vertices.capacity);
			int32_t vertexWriteIndex = _frames[_currentFrameIndex]._vertexCount;
			vertices[vertexWriteIndex++] = firstVertex;
			vertices[vertexWriteIndex++] = prevVertex;
			vertices[vertexWriteIndex++] = currentVertex;
			_frames[_currentFrameIndex]._vertexCount = vertexWriteIndex;

			prevVertex = currentVertex;
			batch.SetVertexCount(batch.GetVertexCount() + 3);
		}
		break;
	}
	case GR_TRIANGLE_STRIP:
	{
		Vertex prevPrevVertex = ReadVertex(vertex, _vertexLayout, batch, textureCacheLocation);
		vertex += stride;

		Vertex prevVertex = ReadVertex(vertex, _vertexLayout, batch, textureCacheLocation);
		vertex += stride;

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex(vertex, _vertexLayout, batch, textureCacheLocation);
			vertex += stride;

			assert((_frames[_currentFrameIndex]._vertexCount + 3) < _frames[_currentFrameIndex]._vertices.capacity);
			int32_t vertexWriteIndex = _frames[_currentFrameIndex]._vertexCount;
			vertices[vertexWriteIndex++] = prevPrevVertex;
			vertices[vertexWriteIndex++] = prevVertex;
			vertices[vertexWriteIndex++] = currentVertex;
			_frames[_currentFrameIndex]._vertexCount = vertexWriteIndex;

			prevPrevVertex = prevVertex;
			prevVertex = currentVertex;
			batch.SetVertexCount(batch.GetVertexCount() + 3);
		}
		break;
	}
	default:
		assert(false && "Unhandled primitive type.");
	}

	assert(_frames[_currentFrameIndex]._batchCount < _frames[_currentFrameIndex]._batches.capacity);
	_frames[_currentFrameIndex]._batches.items[_frames[_currentFrameIndex]._batchCount++] = batch;
}

void D2DXContext::OnTexDownloadTable(GrTexTable_t type, void* data)
{
	if (type == GR_TEXTABLE_PALETTE)
	{
		uint32_t hash = fnv_32a_buf(data, 1024, FNV1_32A_INIT);
		assert(hash != 0);

		for (uint32_t i = 0; i < _paletteKeys.capacity; ++i)
		{
			if (_paletteKeys.items[i] == hash)
			{
				_scratchBatch.SetPaletteIndex(i);
				return;
			}
		}

		for (uint32_t i = 0; i < _paletteKeys.capacity; ++i)
		{
			if (_paletteKeys.items[i] == 0)
			{
				_paletteKeys.items[i] = hash;
				memcpy(&_palettes.items[256 * i], data, 1024);
				_scratchBatch.SetPaletteIndex(i);
				_d3d11Context->SetPalette(i, (const uint32_t*)data);
				return;
			}
		}

		assert(false && "Too many palettes.");
	}
	else
	{
		assert(false && "Unhandled table type.");
	}
}

void D2DXContext::OnChromakeyMode(GrChromakeyMode_t mode)
{
	_scratchBatch.SetIsChromaKeyEnabled(mode == GR_CHROMAKEY_ENABLE);
}

void D2DXContext::OnLoadGammaTable(uint32_t nentries, uint32_t* red, uint32_t* green, uint32_t* blue)
{
	for (int32_t i = 0; i < (int32_t)min(nentries, 256); ++i)
	{
		_gammaTable[i] = ((blue[i] & 0xFF) << 16) | ((green[i] & 0xFF) << 8) | (red[i] & 0xFF);
	}

	_d3d11Context->LoadGammaTable(_gammaTable);
}

void D2DXContext::OnLfbUnlock(const uint32_t* lfbPtr, uint32_t strideInBytes)
{
	_d3d11Context->WriteToScreen(lfbPtr, 640, 480);
}

void D2DXContext::OnGammaCorrectionRGB(float red, float green, float blue)
{
	_d3d11Context->SetGamma(red, green, blue);
}

void D2DXContext::CorrelateBatches()
{
	uint32_t correlatedBatchCount = 0;

	Batch* cfBatches = _frames[_currentFrameIndex]._batches.items;
	Batch* pfBatches = _frames[1 - _currentFrameIndex]._batches.items;

	const int32_t cfBatchCount = _frames[_currentFrameIndex]._batchCount;
	const int32_t pfBatchCount = _frames[1 - _currentFrameIndex]._batchCount;

	for (int32_t cfBatchIndex = 0; cfBatchIndex < cfBatchCount; ++cfBatchIndex)
	{
		Batch& cfBatch = cfBatches[cfBatchIndex];

		if (cfBatch.GetCategory() == BatchCategory::InGamePanel ||
			cfBatch.GetCategory() == BatchCategory::TopUI ||
			cfBatch.GetCategory() == BatchCategory::UiElement)
		{
			continue;
		}

		const Vertex* cfVertices = _frames[_currentFrameIndex]._vertices.items;
		const Vertex* pfVertices = _frames[1 - _currentFrameIndex]._vertices.items;

		const int32_t cfX0 = (int32_t)cfVertices[cfBatch.GetStartVertex() + 0].GetX();
		const int32_t cfY0 = (int32_t)cfVertices[cfBatch.GetStartVertex() + 0].GetY();
		const int32_t cfS0 = cfVertices[cfBatch.GetStartVertex() + 0].GetS();
		const int32_t cfT0 = cfVertices[cfBatch.GetStartVertex() + 0].GetT();

		const int32_t cfX1 = (int32_t)cfVertices[cfBatch.GetStartVertex() + 1].GetX();
		const int32_t cfY1 = (int32_t)cfVertices[cfBatch.GetStartVertex() + 1].GetY();
		const int32_t cfS1 = cfVertices[cfBatch.GetStartVertex() + 1].GetS();
		const int32_t cfT1 = cfVertices[cfBatch.GetStartVertex() + 1].GetT();

		int32_t pfCandidateBatchIndices[] = {
			cfBatchIndex,
			cfBatchIndex - 1, cfBatchIndex + 1,
			cfBatchIndex - 2, cfBatchIndex + 2,
			cfBatchIndex - 3, cfBatchIndex + 3,
			cfBatchIndex - 4, cfBatchIndex + 4,
			cfBatchIndex - 5, cfBatchIndex + 5,
			cfBatchIndex - 6, cfBatchIndex + 6,
			cfBatchIndex - 7, cfBatchIndex + 7,
			cfBatchIndex - 8, cfBatchIndex + 8,
			cfBatchIndex - 9, cfBatchIndex + 9,
		};

		bool found = false;

		for (int32_t candidate = 0; candidate < ARRAYSIZE(pfCandidateBatchIndices); ++candidate)
		{
			const int32_t pfCandidateBatchIndex = pfCandidateBatchIndices[candidate];

			if (pfCandidateBatchIndex < 0 || pfCandidateBatchIndex >= pfBatchCount)
				continue;

			const Batch& pfBatch = pfBatches[pfCandidateBatchIndex];

			const int32_t pfX0 = (int32_t)pfVertices[pfBatch.GetStartVertex() + 0].GetX();
			const int32_t pfY0 = (int32_t)pfVertices[pfBatch.GetStartVertex() + 0].GetY();
			const int32_t pfS0 = pfVertices[pfBatch.GetStartVertex() + 0].GetS();
			const int32_t pfT0 = pfVertices[pfBatch.GetStartVertex() + 0].GetT();

			const int32_t pfX1 = (int32_t)pfVertices[pfBatch.GetStartVertex() + 1].GetX();
			const int32_t pfY1 = (int32_t)pfVertices[pfBatch.GetStartVertex() + 1].GetY();
			const int32_t pfS1 = pfVertices[pfBatch.GetStartVertex() + 1].GetS();
			const int32_t pfT1 = pfVertices[pfBatch.GetStartVertex() + 1].GetT();

			if (cfBatch.GetCategory() == pfBatch.GetCategory() &&
				cfBatch.GetAlphaBlend() == pfBatch.GetAlphaBlend() &&
				cfBatch.GetRgbCombine() == pfBatch.GetRgbCombine() &&
				cfBatch.GetAlphaCombine() == pfBatch.GetAlphaCombine() &&
				cfBatch.GetVertexCount() == pfBatch.GetVertexCount() &&
				cfBatch.GetPrimitiveType() == pfBatch.GetPrimitiveType())
			{
#define XY_THRESHOLD 64
				if (abs(cfX0 - pfX0) < XY_THRESHOLD && abs(cfY0 - pfY0) < XY_THRESHOLD)
				{
					//					cfBatch.previousFrameCorrelatedIndex = pfCandidateBatchIndex;
					found = true;
					break;
				}
			}
		}

		if (found)
		{
			++correlatedBatchCount;
		}
		else
		{
			ColorizeBatch(cfBatch, 0xFFFF0000);
		}
	}

	ALWAYS_PRINT("Correlated %u of %i batches.", correlatedBatchCount, cfBatchCount);
}

void D2DXContext::ColorizeBatch(Batch& batch, uint32_t color)
{
	Vertex* cfVertices = _frames[_currentFrameIndex]._vertices.items;
	const int32_t batchVertexCount = batch.GetVertexCount();

	for (int32_t vertexIndex = 0; vertexIndex < batchVertexCount; ++vertexIndex)
	{
		cfVertices[batch.GetStartVertex() + vertexIndex].SetColor(color);
	}
}

void D2DXContext::PrepareLogoTextureBatch()
{
	if (_logoTextureBatch.IsValid())
	{
		return;
	}

	uint32_t hash = fnv_32a_buf((void*)dx_raw, sizeof(uint32_t) * 81 * 40, FNV1_32A_INIT);

	uint8_t* data = _sideTmuMemory.items;

	_logoTextureBatch.SetTextureStartAddress(0);
	_logoTextureBatch.SetTextureHash(hash);
	_logoTextureBatch.SetTextureSize(128, 128);
	_logoTextureBatch.SetTextureCategory(TextureCategory::TitleScreen);
	_logoTextureBatch.SetIsRgba(true);
	_logoTextureBatch.SetPrimitiveType(PrimitiveType::Triangles);
	_logoTextureBatch.SetAlphaBlend(AlphaBlend::SrcAlphaInvSrcAlpha);
	_logoTextureBatch.SetIsChromaKeyEnabled(true);
	_logoTextureBatch.SetRgbCombine(RgbCombine::ColorMultipliedByTexture);
	_logoTextureBatch.SetAlphaCombine(AlphaCombine::One);
	_logoTextureBatch.SetCategory(BatchCategory::UiElement);
	_logoTextureBatch.SetVertexCount(6);

	memset(data, 0, _logoTextureBatch.GetTextureMemSize());

	const uint32_t* pSrc32 = (const uint32_t*)dx_raw;
	uint32_t* pDst32 = (uint32_t*)data;

	for (int32_t y = 0; y < 40; ++y)
	{
		for (int32_t x = 0; x < 81; ++x)
		{
			uint32_t c = *pSrc32++;
			uint32_t r = (c & 0x00FF0000) >> 16;
			uint32_t b = c & 0x000000FF;
			c &= 0xFF00FF00;
			c |= r;
			c |= b << 16;
			pDst32[x + y * 128] = c;
		}
	}
}

void D2DXContext::InsertLogoOnTitleScreen()
{
	if (_options.skipLogo || _majorGameState != MajorGameState::TitleScreen || _frames[_currentFrameIndex]._batchCount <= 0)
		return;

	Vertex* vertices = _frames[_currentFrameIndex]._vertices.items;

	PrepareLogoTextureBatch();

	auto textureCacheLocation = _d3d11Context->UpdateTexture(_logoTextureBatch, _sideTmuMemory.items, nullptr);
	_logoTextureBatch.SetStartVertex(_frames[_currentFrameIndex]._vertexCount);

	const float x = (float)(_d3d11Context->GetGameWidth() - 90 - 16);
	const float y = (float)(_d3d11Context->GetGameHeight() - 50 - 16);
	const uint32_t color = 0xFFFFa090;

	Vertex vertex0(x, y, 0, 0, color, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, true, textureCacheLocation.ArrayIndex, 0);
	Vertex vertex1(x + 80, y, 80, 0, color, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, true, textureCacheLocation.ArrayIndex, 0);
	Vertex vertex2(x + 80, y + 41, 80, 41, color, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, true, textureCacheLocation.ArrayIndex, 0);
	Vertex vertex3(x, y + 41, 0, 41, color, RgbCombine::ColorMultipliedByTexture, AlphaCombine::One, true, textureCacheLocation.ArrayIndex, 0);

	assert((_frames[_currentFrameIndex]._vertexCount + 6) < _frames[_currentFrameIndex]._vertices.capacity);
	int32_t vertexWriteIndex = _frames[_currentFrameIndex]._vertexCount;
	vertices[vertexWriteIndex++] = vertex0;
	vertices[vertexWriteIndex++] = vertex1;
	vertices[vertexWriteIndex++] = vertex2;
	vertices[vertexWriteIndex++] = vertex0;
	vertices[vertexWriteIndex++] = vertex2;
	vertices[vertexWriteIndex++] = vertex3;
	_frames[_currentFrameIndex]._vertexCount = vertexWriteIndex;

	_frames[_currentFrameIndex]._batches.items[_frames[_currentFrameIndex]._batchCount++] = _logoTextureBatch;
}

GameVersion D2DXContext::GetGameVersion() const
{
	return _gameHelper.GetVersion();
}

void D2DXContext::OnMousePosChanged(int32_t x, int32_t y)
{
	_mouseX = x;
	_mouseY = y;
}

void D2DXContext::FixIngameMousePosition()
{
	/* When opening UI panels, the game will screw up the mouse position when
	   we are using a non-1 window scale. This fix, which is run as early in the frame
	   as we can, forces the ingame variables back to the proper values. */

	if (_frames[_currentFrameIndex]._batchCount == 0)
	{
		_gameHelper.SetIngameMousePos(_mouseX, _mouseY);
	}
}

void D2DXContext::SetCustomResolution(int32_t width, int32_t height)
{
	_customWidth = width;
	_customHeight = height;
}

void D2DXContext::GetSuggestedCustomResolution(int32_t* width, int32_t* height)
{
	int32_t desktopWidth = GetSystemMetrics(SM_CXSCREEN);
	int32_t desktopHeight = GetSystemMetrics(SM_CYSCREEN);
	int32_t customWidth = desktopWidth;
	int32_t customHeight = desktopHeight;
	int32_t scaleFactor = 1;

	while (customHeight > 600)
	{
		++scaleFactor;
		customWidth = desktopWidth / scaleFactor;
		customHeight = desktopHeight / scaleFactor;
	}

	*width = customWidth;
	*height = customHeight;
}
