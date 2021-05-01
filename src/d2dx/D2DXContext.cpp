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
#include "D2DXContext.h"
#include "Detours.h"
#include "BuiltinResMod.h"
#include "RenderContext.h"
#include "GameHelper.h"
#include "SimdSse2.h"
#include "Metrics.h"
#include "Utils.h"
#include "Vertex.h"
#include "dx256_bmp.h"

using namespace d2dx;
using namespace DirectX::PackedVector;
using namespace std;

#define D2DX_GLIDE_ALPHA_BLEND(rgb_sf, rgb_df, alpha_sf, alpha_df) \
		(uint16_t)(((rgb_sf & 0xF) << 12) | ((rgb_df & 0xF) << 8) | ((alpha_sf & 0xF) << 4) | (alpha_df & 0xF))

static bool CheckOption(const char* commandLine, const Buffer<char>& cfgFile, const char* option)
{
	bool hasOption = strstr(commandLine, option) != nullptr || strstr(cfgFile.items, option) != nullptr;
	if (hasOption)
	{
		D2DX_LOG("Option: %s", option);
	}
	return hasOption;
}

static Options GetCommandLineOptions()
{
	Options options;

	Buffer<char> cfgFile = ReadTextFile("d2dx.cfg");
	const char* commandLine = GetCommandLineA();

	options.noClipCursor = CheckOption(commandLine, cfgFile, "-dxnoclipcursor");
	options.noFpsFix = CheckOption(commandLine, cfgFile, "-dxnofpsfix");
	options.noResMod = CheckOption(commandLine, cfgFile, "-dxnoresmod");
	options.noWide = CheckOption(commandLine, cfgFile, "-dxnowide");
	options.noLogo = CheckOption(commandLine, cfgFile, "-dxnologo");
	options.noVSync = CheckOption(commandLine, cfgFile, "-dxnovsync");
	options.noAA = CheckOption(commandLine, cfgFile, "-dxnoaa");
	options.noCompatModeFix = CheckOption(commandLine, cfgFile, "-dxnocompatmodefix");
	options.noTitleChange = CheckOption(commandLine, cfgFile, "-dxnotitlechange");

	bool dxscale2 = CheckOption(commandLine, cfgFile, "-dxscale2");
	bool dxscale3 = CheckOption(commandLine, cfgFile, "-dxscale3");
	options.defaultZoomLevel =
		dxscale3 ? 3 :
		dxscale2 ? 2 :
		1;

	options.debugDumpTextures = CheckOption(commandLine, cfgFile, "-dxdbg_dump");

	options.screenMode = CheckOption(commandLine, cfgFile, "-w") ? ScreenMode::Windowed : ScreenMode::FullscreenDefault;

	options.testMoP = CheckOption(commandLine, cfgFile, "-dxtestmop");

	return options;
}

_Use_decl_annotations_
D2DXContext::D2DXContext(
	const std::shared_ptr<IGameHelper>& gameHelper,
	const std::shared_ptr<ISimd>& simd,
	const std::shared_ptr<CompatibilityModeDisabler>& compatibilityModeDisabler) :
	_gameHelper{ gameHelper },
	_simd{ simd },
	_compatibilityModeDisabler{ compatibilityModeDisabler },
	_frame(0),
	_majorGameState(MajorGameState::Unknown),
	_vertexLayout(0xFF),
	_tmuMemory(D2DX_TMU_MEMORY_SIZE),
	_sideTmuMemory(D2DX_SIDE_TMU_MEMORY_SIZE),
	_constantColor(0xFFFFFFFF),
	_paletteKeys(D2DX_MAX_PALETTES, true),
	_gammaTable(256),
	_batchCount(0),
	_batches(D2DX_MAX_BATCHES_PER_FRAME),
	_vertexCount(0),
	_vertices(D2DX_MAX_VERTICES_PER_FRAME),
	_customGameSize{ 0,0 },
	_suggestedGameSize{ 0, 0 },
	_options{ GetCommandLineOptions() },
	_lastScreenOpenMode{ 0 },
	_textureHashCache{ D2DX_TMU_MEMORY_SIZE / 256, true },
	_textureHashCacheHits{ 0 },
	_textureHashCacheMisses{ 0 },
	_palettes{ D2DX_MAX_PALETTES * 256 },
	_surfaceIdTracker{ gameHelper },
	_playerMotionPredictor{ gameHelper }
{
	if (!_options.noCompatModeFix)
	{
		_compatibilityModeDisabler->DisableCompatibilityMode();
	}

	auto apparentWindowsVersion = GetWindowsVersion();
	auto actualWindowsVersion = GetActualWindowsVersion();
	D2DX_LOG("Apparent Windows version: %u.%u (build %u).", apparentWindowsVersion.major, apparentWindowsVersion.minor, apparentWindowsVersion.build);
	D2DX_LOG("Actual Windows version: %u.%u (build %u).", actualWindowsVersion.major, actualWindowsVersion.minor, actualWindowsVersion.build);

	if (_options.testMoP)
	{
		D2DX_LOG("MoP testing enabled.");
	}

	if (!_options.noResMod)
	{
		try
		{
			_builtinResMod = std::make_unique<BuiltinResMod>(GetModuleHandleA("glide3x.dll"), _gameHelper);
			if (!_builtinResMod->IsActive())
			{
				_options.noResMod = true;
			}
		}
		catch (...)
		{
			_options.noResMod = true;
		}
	}

	if (!_options.noFpsFix)
	{
		_gameHelper->TryApplyFpsFix();
	}
}

_Use_decl_annotations_
const char* D2DXContext::OnGetString(
	uint32_t pname)
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

_Use_decl_annotations_
uint32_t D2DXContext::OnGet(
	uint32_t pname,
	uint32_t plength,
	int32_t* params)
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
		*params = D2DX_TMU_ADDRESS_ALIGNMENT;
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

_Use_decl_annotations_
void D2DXContext::OnSstWinOpen(
	uint32_t hWnd,
	int32_t width,
	int32_t height)
{	
	Size windowSize = _gameHelper->GetConfiguredGameSize();
	if (!_options.noResMod)
	{
		windowSize = { 800,600 };
	}

	Size gameSize{ width, height };

	if (_customGameSize.width > 0)
	{
		gameSize = _customGameSize;
	}

	if (gameSize.width != 640 || gameSize.height != 480)
	{
		windowSize = gameSize;
	}

	if (!_renderContext)
	{
		_renderContext = std::make_shared<RenderContext>(
			(HWND)hWnd,
			gameSize,
			windowSize * _options.defaultZoomLevel,
			this,
			_simd);
	}
	else
	{
		if (width > windowSize.width || height > windowSize.height)
		{
			windowSize.width = width;
			windowSize.height = height;
		}
		_renderContext->SetSizes(gameSize, windowSize);
	}

	_batchCount = 0;
	_vertexCount = 0;
	_scratchBatch = Batch();
}

_Use_decl_annotations_
void D2DXContext::OnVertexLayout(
	uint32_t param,
	int32_t offset)
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

_Use_decl_annotations_
void D2DXContext::OnTexDownload(
	uint32_t tmu,
	const uint8_t* sourceAddress,
	uint32_t startAddress,
	int32_t width,
	int32_t height)
{
	assert(tmu == 0 && (startAddress & 255) == 0);
	if (!(tmu == 0 && (startAddress & 255) == 0))
	{
		return;
	}

	_textureHashCache.items[startAddress >> 8] = 0;

	uint32_t memRequired = (uint32_t)(width * height);

	auto pStart = _tmuMemory.items + startAddress;
	auto pEnd = _tmuMemory.items + startAddress + memRequired;
	assert(pEnd <= (_tmuMemory.items + _tmuMemory.capacity));
	memcpy_s(pStart, _tmuMemory.capacity - startAddress, sourceAddress, memRequired);
}

_Use_decl_annotations_
void D2DXContext::OnTexSource(
	uint32_t tmu,
	uint32_t startAddress,
	int32_t width,
	int32_t height)
{
	assert(tmu == 0 && (startAddress & 255) == 0);
	if (!(tmu == 0 && (startAddress & 255) == 0))
	{
		return;
	}

	uint8_t* pixels = _tmuMemory.items + startAddress;
	const uint32_t pixelsSize = width * height;

	uint32_t hash = _textureHashCache.items[startAddress >> 8];

	if (hash)
	{
		++_textureHashCacheHits;
	}
	else
	{
		++_textureHashCacheMisses;
		hash = fnv_32a_buf(pixels, pixelsSize, FNV1_32A_INIT);
		_textureHashCache.items[startAddress >> 8] = hash;
	}

	/* Patch the '5' to not look like '6'. */
	if (hash == 0x8a12f6bb)
	{
		pixels[1 + 10 * 16] = 181;
		pixels[2 + 10 * 16] = 181;
		pixels[1 + 11 * 16] = 29;
	}

	_scratchBatch.SetTextureStartAddress(startAddress);
	_scratchBatch.SetTextureHash(hash);
	_scratchBatch.SetTextureSize(width, height);
	
	if (_scratchBatch.GetTextureCategory() == TextureCategory::Unknown)
	{
		_scratchBatch.SetTextureCategory(_gameHelper->GetTextureCategoryFromHash(hash));
	}

	if (_options.debugDumpTextures)
	{
		DumpTexture(hash, width, height, pixels, pixelsSize, (uint32_t)_scratchBatch.GetTextureCategory(), _palettes.items + _scratchBatch.GetPaletteIndex() * 256);
	}
}

void D2DXContext::CheckMajorGameState()
{
	const int32_t batchCount = (int32_t)_batchCount;

	if (_majorGameState == MajorGameState::Unknown && batchCount == 0)
	{
		_majorGameState = MajorGameState::FmvIntro;
	}

	_majorGameState = MajorGameState::Menus;

	if (_gameHelper->ScreenOpenMode() == 3)
	{
		_majorGameState = MajorGameState::InGame;
	}
	else
	{
		for (int32_t i = 0; i < batchCount; ++i)
		{
			const Batch& batch = _batches.items[i];

			if ((GameAddress)batch.GetGameAddress() == GameAddress::DrawFloor)
			{
				_majorGameState = MajorGameState::InGame;
				AttachLateDetours(_gameHelper.get());
				break;
			}
		}

		if (_majorGameState == MajorGameState::Menus)
		{
			for (int32_t i = 0; i < batchCount; ++i)
			{
				const Batch& batch = _batches.items[i];
				const int32_t y0 = _vertices.items[batch.GetStartVertex()].GetY();

				if (batch.GetHash() == 0x4bea7b80 && y0 >= 550)
				{
					_majorGameState = MajorGameState::TitleScreen;
					break;
				}
			}
		}
	}
}

_Use_decl_annotations_
void D2DXContext::DrawBatches(
	uint32_t startVertexLocation)
{
	const int32_t batchCount = (int32_t)_batchCount;

	Batch mergedBatch;
	int32_t drawCalls = 0;

	for (int32_t i = 0; i < batchCount; ++i)
	{
		const Batch& batch = _batches.items[i];

		if (!batch.IsValid())
		{
			D2DX_DEBUG_LOG("Skipping batch %i, it is invalid.", i);
			continue;
		}

		if (!mergedBatch.IsValid())
		{
			mergedBatch = batch;
		}
		else
		{
			if (_renderContext->GetTextureCache(batch) != _renderContext->GetTextureCache(mergedBatch) ||
				batch.GetTextureAtlas() != mergedBatch.GetTextureAtlas() ||
				batch.GetAlphaBlend() != mergedBatch.GetAlphaBlend() ||
				((mergedBatch.GetVertexCount() + batch.GetVertexCount()) > 65535))
			{
				_renderContext->Draw(mergedBatch, startVertexLocation);
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
		_renderContext->Draw(mergedBatch, startVertexLocation);
		++drawCalls;
	}

	if (!(_frame & 31))
	{
		D2DX_DEBUG_LOG("Nr draw calls: %i", drawCalls);
	}
}

void D2DXContext::OnBufferSwap()
{
	if (_options.testMoP && _majorGameState == MajorGameState::InGame)
	{
		_playerMotionPredictor.Update(_renderContext.get());
	}

	CheckMajorGameState();
	InsertLogoOnTitleScreen();

	if (_options.testMoP && _majorGameState == MajorGameState::InGame)
	{
		auto offset = _playerMotionPredictor.GetOffset();

		int32_t screenOffsetX = (int32_t)(32.0f * (offset.x - offset.y) / sqrt(2.0f) + 0.5f);
		int32_t screenOffsetY = (int32_t)(16.0f * (offset.x + offset.y) / sqrt(2.0f) + 0.5f);

		for (int32_t i = 0; i < _batchCount; ++i)
		{
			const Batch& batch = _batches.items[i];
			auto surfaceId = _vertices.items[batch.GetStartVertex()].GetSurfaceId();

			if (surfaceId != D2DX_SURFACE_ID_USER_INTERFACE &&
				batch.GetTextureCategory() != TextureCategory::Player)
			{
				const auto batchVertexCount = batch.GetVertexCount();
				int32_t vertexIndex = batch.GetStartVertex();
				for (uint32_t j = 0; j < batchVertexCount; ++j)
				{
					_vertices.items[vertexIndex++].AddOffset({ -screenOffsetX, -screenOffsetY });
				}
			}
		}
	}

	auto startVertexLocation = _renderContext->BulkWriteVertices(_vertices.items, _vertexCount);

	DrawBatches(startVertexLocation);

	_renderContext->Present();

	++_frame;

	if (!(_frame & 255))
	{
		D2DX_DEBUG_LOG("Texture hash cache hits: %u (%i%%) misses %u",
			_textureHashCacheHits,
			(int32_t)(100.0f * (float)_textureHashCacheHits / (_textureHashCacheHits + _textureHashCacheMisses)),
			_textureHashCacheMisses
		);
	}

	_batchCount = 0;
	_vertexCount = 0;

	_lastScreenOpenMode = _gameHelper->ScreenOpenMode();

	_surfaceIdTracker.OnNewFrame();

	Rect renderRect;
	Size desktopSize;
	_renderContext->GetCurrentMetrics(&_gameSize, &renderRect, &desktopSize);

	//D2DX_LOG("Time %f, Player %f,  %f (interpolated %f, %f)", _time, _lastPlayerX, _lastPlayerY, _itpPlayerX, _itpPlayerY);
}

_Use_decl_annotations_
void D2DXContext::OnColorCombine(
	GrCombineFunction_t function,
	GrCombineFactor_t factor,
	GrCombineLocal_t local,
	GrCombineOther_t other,
	bool invert)
{
	auto rgbCombine = RgbCombine::ColorMultipliedByTexture;

	if (function == GR_COMBINE_FUNCTION_SCALE_OTHER && factor == GR_COMBINE_FACTOR_LOCAL &&
		local == GR_COMBINE_LOCAL_ITERATED && other == GR_COMBINE_OTHER_TEXTURE)
	{
		rgbCombine = RgbCombine::ColorMultipliedByTexture;
	}
	else if (function == GR_COMBINE_FUNCTION_LOCAL && factor == GR_COMBINE_FACTOR_ZERO &&
		local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT)
	{
		rgbCombine = RgbCombine::ConstantColor;
	}
	else
	{
		assert(false && "Unhandled color combine.");
	}

	_scratchBatch.SetRgbCombine(rgbCombine);
}

_Use_decl_annotations_
void D2DXContext::OnAlphaCombine(
	GrCombineFunction_t function,
	GrCombineFactor_t factor,
	GrCombineLocal_t local,
	GrCombineOther_t other,
	bool invert)
{
	auto alphaCombine = AlphaCombine::One;

	if (function == GR_COMBINE_FUNCTION_ZERO && factor == GR_COMBINE_FACTOR_ZERO &&
		local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT)
	{
		alphaCombine = AlphaCombine::One;
	}
	else if (function == GR_COMBINE_FUNCTION_LOCAL && factor == GR_COMBINE_FACTOR_ZERO &&
		local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT)
	{
		alphaCombine = AlphaCombine::FromColor;
	}
	else
	{
		assert(false && "Unhandled alpha combine.");
	}

	_scratchBatch.SetAlphaCombine(alphaCombine);
}

_Use_decl_annotations_
void D2DXContext::OnConstantColorValue(
	uint32_t color)
{
	_constantColor = (color >> 8) | (color << 24);
}

_Use_decl_annotations_
void D2DXContext::OnAlphaBlendFunction(
	GrAlphaBlendFnc_t rgb_sf,
	GrAlphaBlendFnc_t rgb_df,
	GrAlphaBlendFnc_t alpha_sf,
	GrAlphaBlendFnc_t alpha_df)
{
	auto alphaBlend = AlphaBlend::Opaque;

	switch (D2DX_GLIDE_ALPHA_BLEND(rgb_sf, rgb_df, alpha_sf, alpha_df))
	{
	case D2DX_GLIDE_ALPHA_BLEND(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO, GR_BLEND_ZERO):
		alphaBlend = AlphaBlend::Opaque;
		break;
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

_Use_decl_annotations_
void D2DXContext::OnDrawPoint(
	const void* pt,
	uint32_t gameContext)
{
	auto gameAddress = _gameHelper->IdentifyGameAddress(gameContext);

	Batch batch = _scratchBatch;
	batch.SetGameAddress(gameAddress);
	batch.SetStartVertex(_vertexCount);
	batch.SetTextureCategory(_gameHelper->RefineTextureCategoryFromGameAddress(batch.GetTextureCategory(), gameAddress));

	Vertex centerVertex = ReadVertex((const uint8_t*)pt, _vertexLayout, batch, 0);

	Vertex vertex0 = centerVertex;
	Vertex vertex1 = centerVertex;
	vertex1.SetX((int32_t)(vertex1.GetX() + 1));
	Vertex vertex2 = centerVertex;
	vertex2.SetX((int32_t)(vertex2.GetX() + 1));
	vertex2.SetY((int32_t)(vertex2.GetY() + 1));

	assert((_vertexCount + 3) < _vertices.capacity);
	_vertices.items[_vertexCount++] = vertex0;
	_vertices.items[_vertexCount++] = vertex1;
	_vertices.items[_vertexCount++] = vertex2;

	batch.SetVertexCount(3);

	_surfaceIdTracker.UpdateBatchSurfaceId(batch, _majorGameState, _gameSize, &_vertices.items[batch.GetStartVertex()], batch.GetVertexCount());

	assert(_batchCount < _batches.capacity);
	_batches.items[_batchCount++] = batch;
}

_Use_decl_annotations_
void D2DXContext::OnDrawLine(
	const void* v1,
	const void* v2,
	uint32_t gameContext)
{
	Batch batch = _scratchBatch;
	batch.SetGameAddress(GameAddress::DrawLine);
	batch.SetStartVertex(_vertexCount);
	batch.SetTextureCategory(_gameHelper->RefineTextureCategoryFromGameAddress(batch.GetTextureCategory(), GameAddress::DrawLine));

	Vertex startVertex = ReadVertex((const uint8_t*)v1, _vertexLayout, batch, 0);
	Vertex endVertex = ReadVertex((const uint8_t*)v2, _vertexLayout, batch, 0);

	float dx = (float)(startVertex.GetY() - endVertex.GetY());
	float dy = (float)(endVertex.GetX() - startVertex.GetX());
	const float lensqr = dx * dx + dy * dy;
	const float len = lensqr > 0.01f ? sqrtf(lensqr) : 1.0f;
	const float halfinvlen = 1.0f / (2.0f * len);
	dx *= halfinvlen;
	dy *= halfinvlen;

	Vertex vertex0 = startVertex;
	vertex0.SetX((int32_t)(vertex0.GetX() - dx));
	vertex0.SetY((int32_t)(vertex0.GetY() - dy));

	Vertex vertex1 = startVertex;
	vertex1.SetX((int32_t)(vertex1.GetX() + dx));
	vertex1.SetY((int32_t)(vertex1.GetY() + dy));

	Vertex vertex2 = endVertex;
	vertex2.SetX((int32_t)(vertex2.GetX() - dx));
	vertex2.SetY((int32_t)(vertex2.GetY() - dy));

	Vertex vertex3 = endVertex;
	vertex3.SetX((int32_t)(vertex3.GetX() + dx));
	vertex3.SetY((int32_t)(vertex3.GetY() + dy));

	assert((_vertexCount + 6) < _vertices.capacity);
	_vertices.items[_vertexCount++] = vertex0;
	_vertices.items[_vertexCount++] = vertex1;
	_vertices.items[_vertexCount++] = vertex2;
	_vertices.items[_vertexCount++] = vertex1;
	_vertices.items[_vertexCount++] = vertex2;
	_vertices.items[_vertexCount++] = vertex3;

	batch.SetVertexCount(6);

	_surfaceIdTracker.UpdateBatchSurfaceId(batch, _majorGameState, _gameSize, &_vertices.items[batch.GetStartVertex()], batch.GetVertexCount());

	assert(_batchCount < _batches.capacity);
	_batches.items[_batchCount++] = batch;
}

_Use_decl_annotations_
Vertex D2DXContext::ReadVertex(
	const uint8_t* vertex,
	uint32_t vertexLayout,
	const Batch& batch,
	int32_t surfaceId)
{
	uint32_t stShift = 0;
	_BitScanReverse((DWORD*)&stShift, max(batch.GetTextureWidth(), batch.GetTextureHeight()));
	stShift = 8 - stShift;

	const int32_t xyOffset = (vertexLayout >> 16) & 0xFF;
	const int32_t stOffset = (vertexLayout >> 8) & 0xFF;
	const int32_t pargbOffset = vertexLayout & 0xFF;

	auto xy = (const float*)(vertex + xyOffset);
	auto st = (const float*)(vertex + stOffset);
	assert((st[0] - floor(st[0])) < 1e6);
	assert((st[1] - floor(st[1])) < 1e6);
	int16_t s = (int16_t)st[0] >> stShift;
	int16_t t = (int16_t)st[1] >> stShift;

	auto pargb = pargbOffset != 0xFF ? *(const uint32_t*)(vertex + pargbOffset) : 0xFFFFFFFF;

	if (batch.GetAlphaCombine() == AlphaCombine::One)
	{
		pargb |= 0xFF000000;
	}

	int32_t paletteIndex = batch.GetRgbCombine() == RgbCombine::ColorMultipliedByTexture ? batch.GetPaletteIndex() : D2DX_WHITE_PALETTE_INDEX;
	return Vertex((int32_t)xy[0], (int32_t)xy[1], s, t, batch.SelectColorAndAlpha(pargb, _constantColor), batch.IsChromaKeyEnabled(), batch.GetTextureIndex(), paletteIndex, surfaceId);
}

_Use_decl_annotations_
const Batch D2DXContext::PrepareBatchForSubmit(
	Batch batch,
	PrimitiveType primitiveType,
	uint32_t vertexCount,
	uint32_t gameContext) const
{
	auto gameAddress = _gameHelper->IdentifyGameAddress(gameContext);

	auto tcl = _renderContext->UpdateTexture(batch, _tmuMemory.items, _tmuMemory.capacity);
	batch.SetTextureAtlas(tcl._textureAtlas);
	batch.SetTextureIndex(tcl._textureIndex);

	batch.SetGameAddress(gameAddress);
	batch.SetStartVertex(_vertexCount);
	batch.SetVertexCount(vertexCount);
	batch.SetTextureCategory(_gameHelper->RefineTextureCategoryFromGameAddress(batch.GetTextureCategory(), gameAddress));
	return batch;
}

_Use_decl_annotations_
void D2DXContext::OnDrawVertexArray(
	uint32_t mode,
	uint32_t count,
	uint8_t** pointers,
	uint32_t gameContext)
{
	Batch batch = PrepareBatchForSubmit(_scratchBatch, PrimitiveType::Triangles, (count - 2) * 3, gameContext);

	switch (mode)
	{
	case GR_TRIANGLE_FAN:
	{
		Vertex firstVertex = ReadVertex((const uint8_t*)pointers[0], _vertexLayout, batch, 0);
		Vertex prevVertex = ReadVertex((const uint8_t*)pointers[1], _vertexLayout, batch, 0);

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex((const uint8_t*)pointers[i], _vertexLayout, batch, 0);

			assert((_vertexCount + 3) < _vertices.capacity);
			_vertices.items[_vertexCount++] = firstVertex;
			_vertices.items[_vertexCount++] = prevVertex;
			_vertices.items[_vertexCount++] = currentVertex;

			prevVertex = currentVertex;
		}
		break;
	}
	case GR_TRIANGLE_STRIP:
	{
		Vertex prevPrevVertex = ReadVertex((const uint8_t*)pointers[0], _vertexLayout, batch, 0);
		Vertex prevVertex = ReadVertex((const uint8_t*)pointers[1], _vertexLayout, batch, 0);

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex((const uint8_t*)pointers[i], _vertexLayout, batch, 0);

			assert((_vertexCount + 3) < _vertices.capacity);
			_vertices.items[_vertexCount++] = prevPrevVertex;
			_vertices.items[_vertexCount++] = prevVertex;
			_vertices.items[_vertexCount++] = currentVertex;

			prevPrevVertex = prevVertex;
			prevVertex = currentVertex;
		}
		break;
	}
	default:
		assert(false && "Unhandled primitive type.");
		return;
	}

	_surfaceIdTracker.UpdateBatchSurfaceId(batch, _majorGameState, _gameSize, &_vertices.items[batch.GetStartVertex()], batch.GetVertexCount());

	assert(_batchCount < _batches.capacity);
	_batches.items[_batchCount++] = batch;
}

_Use_decl_annotations_
void D2DXContext::OnDrawVertexArrayContiguous(
	uint32_t mode,
	uint32_t count,
	uint8_t* vertex,
	uint32_t stride,
	uint32_t gameContext)
{
	Batch batch = PrepareBatchForSubmit(_scratchBatch, PrimitiveType::Triangles, (count - 2) * 3, gameContext);

	switch (mode)
	{
	case GR_TRIANGLE_FAN:
	{
		Vertex firstVertex = ReadVertex(vertex, _vertexLayout, batch, 0);
		vertex += stride;

		Vertex prevVertex = ReadVertex(vertex, _vertexLayout, batch, 0);
		vertex += stride;

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex(vertex, _vertexLayout, batch, 0);
			vertex += stride;

			assert((_vertexCount + 3) < _vertices.capacity);
			_vertices.items[_vertexCount++] = firstVertex;
			_vertices.items[_vertexCount++] = prevVertex;
			_vertices.items[_vertexCount++] = currentVertex;

			prevVertex = currentVertex;
		}
		break;
	}
	case GR_TRIANGLE_STRIP:
	{
		Vertex prevPrevVertex = ReadVertex(vertex, _vertexLayout, batch, 0);
		vertex += stride;

		Vertex prevVertex = ReadVertex(vertex, _vertexLayout, batch, 0);
		vertex += stride;

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex(vertex, _vertexLayout, batch, 0);
			vertex += stride;

			assert((_vertexCount + 3) < _vertices.capacity);
			_vertices.items[_vertexCount++] = prevPrevVertex;
			_vertices.items[_vertexCount++] = prevVertex;
			_vertices.items[_vertexCount++] = currentVertex;

			prevPrevVertex = prevVertex;
			prevVertex = currentVertex;
		}
		break;
	}
	default:
		assert(false && "Unhandled primitive type.");
		return;
	}

	_surfaceIdTracker.UpdateBatchSurfaceId(batch, _majorGameState, _gameSize, &_vertices.items[batch.GetStartVertex()], batch.GetVertexCount());

	assert(_batchCount < _batches.capacity);
	_batches.items[_batchCount++] = batch;
}

_Use_decl_annotations_
void D2DXContext::OnTexDownloadTable(
	GrTexTable_t type,
	void* data)
{
	if (type != GR_TEXTABLE_PALETTE)
	{
		assert(false && "Unhandled table type.");
		return;
	}

	uint32_t hash = fnv_32a_buf(data, 1024, FNV1_32A_INIT);
	assert(hash != 0);

	for (uint32_t i = 0; i < D2DX_MAX_GAME_PALETTES; ++i)
	{
		if (_paletteKeys.items[i] == 0)
		{
			break;
		}

		if (_paletteKeys.items[i] == hash)
		{
			_scratchBatch.SetPaletteIndex(i);
			return;
		}
	}

	for (uint32_t i = 0; i < D2DX_MAX_GAME_PALETTES; ++i)
	{
		if (_paletteKeys.items[i] == 0)
		{
			_paletteKeys.items[i] = hash;
			_scratchBatch.SetPaletteIndex(i);

			uint32_t* palette = (uint32_t*)data;

			for (int32_t j = 0; j < 256; ++j)
			{
				palette[j] |= 0xFF000000;
			}

			if (_options.debugDumpTextures)
			{
				memcpy(_palettes.items + 256 * i, palette, 1024);
			}

			_renderContext->SetPalette(i, palette);
			return;
		}
	}

	assert(false && "Too many palettes.");
	D2DX_LOG("Too many palettes.");
}

_Use_decl_annotations_
void D2DXContext::OnChromakeyMode(
	GrChromakeyMode_t mode)
{
	_scratchBatch.SetIsChromaKeyEnabled(mode == GR_CHROMAKEY_ENABLE);
}

_Use_decl_annotations_
void D2DXContext::OnLoadGammaTable(
	uint32_t nentries,
	uint32_t* red,
	uint32_t* green,
	uint32_t* blue)
{
	for (int32_t i = 0; i < (int32_t)min(nentries, 256); ++i)
	{
		_gammaTable.items[i] = ((blue[i] & 0xFF) << 16) | ((green[i] & 0xFF) << 8) | (red[i] & 0xFF);
	}

	_renderContext->LoadGammaTable(_gammaTable.items, _gammaTable.capacity);
}

_Use_decl_annotations_
void D2DXContext::OnLfbUnlock(
	const uint32_t* lfbPtr,
	uint32_t strideInBytes)
{
	_renderContext->WriteToScreen(lfbPtr, 640, 480);
}

_Use_decl_annotations_
void D2DXContext::OnGammaCorrectionRGB(
	float red,
	float green,
	float blue)
{
	uint32_t gammaTable[256];

	for (int32_t i = 0; i < 256; ++i)
	{
		float v = i / 255.0f;
		float r = powf(v, 1.0f / red);
		float g = powf(v, 1.0f / green);
		float b = powf(v, 1.0f / blue);
		uint32_t ri = (uint32_t)(r * 255.0f);
		uint32_t gi = (uint32_t)(g * 255.0f);
		uint32_t bi = (uint32_t)(b * 255.0f);
		gammaTable[i] = (ri << 16) | (gi << 8) | bi;
	}

	_renderContext->LoadGammaTable(gammaTable, ARRAYSIZE(gammaTable));
}

void D2DXContext::PrepareLogoTextureBatch()
{
	if (_logoTextureBatch.IsValid())
	{
		return;
	}

	const uint8_t* srcPixels = dx_logo256 + 0x436;

	Buffer<uint32_t> palette(256);
	memcpy_s(palette.items, palette.capacity * sizeof(uint32_t), (uint32_t*)(dx_logo256 + 0x36), 256 * sizeof(uint32_t));

	for (int32_t i = 0; i < 256; ++i)
	{
		palette.items[i] |= 0xFF000000;
	}

	_renderContext->SetPalette(D2DX_LOGO_PALETTE_INDEX, palette.items);

	uint32_t hash = fnv_32a_buf((void*)srcPixels, sizeof(uint8_t) * 81 * 40, FNV1_32A_INIT);

	uint8_t* data = _sideTmuMemory.items;

	_logoTextureBatch.SetTextureStartAddress(0);
	_logoTextureBatch.SetTextureHash(hash);
	_logoTextureBatch.SetTextureSize(128, 128);
	_logoTextureBatch.SetTextureCategory(TextureCategory::TitleScreen);
	_logoTextureBatch.SetAlphaBlend(AlphaBlend::SrcAlphaInvSrcAlpha);
	_logoTextureBatch.SetIsChromaKeyEnabled(true);
	_logoTextureBatch.SetRgbCombine(RgbCombine::ColorMultipliedByTexture);
	_logoTextureBatch.SetAlphaCombine(AlphaCombine::One);
	_logoTextureBatch.SetPaletteIndex(D2DX_LOGO_PALETTE_INDEX);
	_logoTextureBatch.SetVertexCount(6);

	memset(data, 0, _logoTextureBatch.GetTextureWidth() * _logoTextureBatch.GetTextureHeight());

	for (int32_t y = 0; y < 41; ++y)
	{
		for (int32_t x = 0; x < 80; ++x)
		{
			data[x + (40 - y) * 128] = *srcPixels++;
		}
	}
}

void D2DXContext::InsertLogoOnTitleScreen()
{
	if (_options.noLogo || _majorGameState != MajorGameState::TitleScreen || _batchCount <= 0)
		return;

	PrepareLogoTextureBatch();

	auto tcl = _renderContext->UpdateTexture(_logoTextureBatch, _sideTmuMemory.items, _sideTmuMemory.capacity);

	_logoTextureBatch.SetTextureAtlas(tcl._textureAtlas);
	_logoTextureBatch.SetTextureIndex(tcl._textureIndex);
	_logoTextureBatch.SetStartVertex(_vertexCount);

	Size gameSize;
	Rect renderRect;
	Size desktopSize;
	_renderContext->GetCurrentMetrics(&gameSize, &renderRect, &desktopSize);

	const int32_t x = gameSize.width - 90 - 16;
	const int32_t y = gameSize.height - 50 - 16;
	const uint32_t color = 0xFFFFa090;

	Vertex vertex0(x, y, 0, 0, color, true, _logoTextureBatch.GetTextureIndex(), D2DX_LOGO_PALETTE_INDEX, D2DX_SURFACE_ID_USER_INTERFACE);
	Vertex vertex1(x + 80, y, 80, 0, color, true, _logoTextureBatch.GetTextureIndex(), D2DX_LOGO_PALETTE_INDEX, D2DX_SURFACE_ID_USER_INTERFACE);
	Vertex vertex2(x + 80, y + 41, 80, 41, color, true, _logoTextureBatch.GetTextureIndex(), D2DX_LOGO_PALETTE_INDEX, D2DX_SURFACE_ID_USER_INTERFACE);
	Vertex vertex3(x, y + 41, 0, 41, color, true, _logoTextureBatch.GetTextureIndex(), D2DX_LOGO_PALETTE_INDEX, D2DX_SURFACE_ID_USER_INTERFACE);

	assert((_vertexCount + 6) < _vertices.capacity);
	_vertices.items[_vertexCount++] = vertex0;
	_vertices.items[_vertexCount++] = vertex1;
	_vertices.items[_vertexCount++] = vertex2;
	_vertices.items[_vertexCount++] = vertex0;
	_vertices.items[_vertexCount++] = vertex2;
	_vertices.items[_vertexCount++] = vertex3;

	_batches.items[_batchCount++] = _logoTextureBatch;
}

GameVersion D2DXContext::GetGameVersion() const
{
	return _gameHelper->GetVersion();
}

_Use_decl_annotations_
Offset D2DXContext::OnSetCursorPos(
	Offset pos)
{
	auto currentScreenOpenMode = _gameHelper->ScreenOpenMode();

	if (_lastScreenOpenMode != currentScreenOpenMode)
	{
		POINT originalPos;
		GetCursorPos(&originalPos);

		auto hWnd = _renderContext->GetHWnd();

		ScreenToClient(hWnd, (LPPOINT)&pos);

		Size gameSize;
		Rect renderRect;
		Size desktopSize;
		_renderContext->GetCurrentMetrics(&gameSize, &renderRect, &desktopSize);

		const bool isFullscreen = _options.screenMode == ScreenMode::FullscreenDefault;
		const float scale = (float)renderRect.size.height / gameSize.height;
		const uint32_t scaledWidth = (uint32_t)(scale * gameSize.width);
		const float mouseOffsetX = isFullscreen ? (float)(desktopSize.width / 2 - scaledWidth / 2) : 0.0f;

		pos.x = (int32_t)(pos.x * scale + mouseOffsetX);
		pos.y = (int32_t)(pos.y * scale);

		ClientToScreen(hWnd, (LPPOINT)&pos);

		pos.y = originalPos.y;

		return pos;
	}

	return { -1, -1 };
}

_Use_decl_annotations_
Offset D2DXContext::OnMouseMoveMessage(
	Offset pos)
{
	auto currentScreenOpenMode = _gameHelper->ScreenOpenMode();

	Size gameSize;
	Rect renderRect;
	Size desktopSize;
	_renderContext->GetCurrentMetrics(&gameSize, &renderRect, &desktopSize);

	const bool isFullscreen = _options.screenMode == ScreenMode::FullscreenDefault;
	const float scale = (float)renderRect.size.height / gameSize.height;
	const uint32_t scaledWidth = (uint32_t)(scale * gameSize.width);
	const float mouseOffsetX = isFullscreen ? (float)(desktopSize.width / 2 - scaledWidth / 2) : 0.0f;

	pos.x = (int32_t)(pos.x * scale + mouseOffsetX);
	pos.y = (int32_t)(pos.y * scale);

	return pos;
}

_Use_decl_annotations_
void D2DXContext::SetCustomResolution(
	Size size)
{
	_customGameSize = size;
}

Size D2DXContext::GetSuggestedCustomResolution()
{
	if (_suggestedGameSize.width == 0)
	{
		Size desktopSize{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
		_suggestedGameSize = Metrics::GetSuggestedGameSize(desktopSize, !_options.noWide);
		D2DX_LOG("Suggesting game size %ix%i.", _suggestedGameSize.width, _suggestedGameSize.height);
	}

	return _suggestedGameSize;
}

void D2DXContext::DisableBuiltinResMod()
{
	_options.noResMod = true;
}

Options& D2DXContext::GetOptions()
{
	return _options;
}

void D2DXContext::OnBufferClear()
{
}

void D2DXContext::BeginDrawText()
{
	_scratchBatch.SetTextureCategory(TextureCategory::UserInterface);
	_isDrawingText = true;
}

void D2DXContext::EndDrawText()
{
	_scratchBatch.SetTextureCategory(TextureCategory::Unknown);
	_isDrawingText = false;
}

#define MAKE_PLAYER_TYPE(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

_Use_decl_annotations_
void D2DXContext::BeginDrawImage(
	CellContext* pCellContext, 
	Offset pos)
{
	if (_isDrawingText)
	{
		return;
	}

	if (pCellContext->dwUnit == 0 && (
		pCellContext->dwPlayerType == MAKE_PLAYER_TYPE('A', 'M', ' ', ' ') ||
		pCellContext->dwPlayerType == MAKE_PLAYER_TYPE('S', 'O', ' ', ' ') ||
		pCellContext->dwPlayerType == MAKE_PLAYER_TYPE('N', 'E', ' ', ' ') ||
		pCellContext->dwPlayerType == MAKE_PLAYER_TYPE('P', 'A', ' ', ' ') ||
		pCellContext->dwPlayerType == MAKE_PLAYER_TYPE('B', 'A', ' ', ' ') ||
		pCellContext->dwPlayerType == MAKE_PLAYER_TYPE('A', 'M', ' ', ' ') ||
		pCellContext->dwPlayerType == MAKE_PLAYER_TYPE('D', 'Z', ' ', ' ') ||
		pCellContext->dwPlayerType == MAKE_PLAYER_TYPE('A', 'I', ' ', ' ')))
	{
		// The player unit itself.
		_scratchBatch.SetTextureCategory(TextureCategory::Player);
		_playerScreenPos = pos;
	}
	else if (_playerScreenPos.x > 0 &&
		pos.x == _playerScreenPos.x &&
		pos.y == _playerScreenPos.y)
	{
		// Overlays will be drawn at the player position, so mark them as part of the player.
		_scratchBatch.SetTextureCategory(TextureCategory::Player);
	}
	else if (
		pCellContext->dwClass == 0 &&
		pCellContext->dwUnit == 0 &&
		pCellContext->dwMode == 0)
	{
		// UI elements have neither class nor unit set.
		_scratchBatch.SetTextureCategory(TextureCategory::UserInterface);
	}
}

void D2DXContext::EndDrawImage()
{
	if (_isDrawingText)
	{
		return;
	}

	_scratchBatch.SetTextureCategory(TextureCategory::Unknown);
}
