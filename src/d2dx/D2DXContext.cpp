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

static Options GetCommandLineOptions()
{
	Options options;

	const char* commandLine = GetCommandLineA();
	bool windowed = strstr(commandLine, "-w") != nullptr;
	options.noClipCursor = strstr(commandLine, "-dxnoclipcursor") != nullptr;
	options.noFpsFix = strstr(commandLine, "-dxnofpsfix") != nullptr;
	options.noResMod = strstr(commandLine, "-dxnoresmod") != nullptr;
	options.noWide = strstr(commandLine, "-dxnowide") != nullptr;
	options.noLogo = strstr(commandLine, "-dxnologo") != nullptr || strstr(commandLine, "-gxskiplogo") != nullptr;
	options.noVSync = strstr(commandLine, "-dxnovsync") != nullptr;
	options.noAA = strstr(commandLine, "-dxnoaa") != nullptr;

	bool dxscale2 = strstr(commandLine, "-dxscale2") != nullptr || strstr(commandLine, "-gxscale2") != nullptr;
	bool dxscale3 = strstr(commandLine, "-dxscale3") != nullptr || strstr(commandLine, "-gxscale3") != nullptr;
	options.defaultZoomLevel =
		dxscale3 ? 3 :
		dxscale2 ? 2 :
		1;

	options.screenMode = windowed ? ScreenMode::Windowed : ScreenMode::FullscreenDefault;

	return options;
}

_Use_decl_annotations_
D2DXContext::D2DXContext(
	IGameHelper* gameHelper,
	ISimd* simd) :
	_frame(0),
	_majorGameState(MajorGameState::Unknown),
	_vertexLayout(0xFF),
	_tmuMemory(D2DX_TMU_MEMORY_SIZE),
	_sideTmuMemory(D2DX_SIDE_TMU_MEMORY_SIZE),
	_constantColor(0xFFFFFFFF),
	_paletteKeys(D2DX_MAX_PALETTES),
	_gammaTable(256),
	_batchCount(0),
	_batches(D2DX_MAX_BATCHES_PER_FRAME),
	_vertexCount(0),
	_vertices(D2DX_MAX_VERTICES_PER_FRAME),
	_customGameSize{ 0,0 },
	_suggestedGameSize{ 0, 0 },
	_gameHelper{ gameHelper },
	_simd{ simd },
	_options{ GetCommandLineOptions() },
	_lastScreenOpenMode{ 0 },
	_batchId{ 0 },
	_nextBatchId{ 0 }
{
	memset(_paletteKeys.items, 0, sizeof(uint32_t)* _paletteKeys.capacity);

	if (!_options.noResMod)
	{
		HRESULT hr = MakeAndInitialize<BuiltinResMod>(&_builtinResMod, GetModuleHandleA("glide3x.dll"), _gameHelper.Get());
		if (FAILED(hr) || !_builtinResMod->IsActive())
		{
			_options.noResMod = true;
		}
	}

	if (!_options.noFpsFix)
	{
		_gameHelper->TryApplyFpsFix();
	}
}

D2DXContext::~D2DXContext()
{
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
		_renderContext = Make<RenderContext>(
			(HWND)hWnd,
			gameSize,
			windowSize * _options.defaultZoomLevel,
			_options,
			_simd.Get());

		uint32_t whitePalette[256];
		for (int32_t i = 0; i < 256; ++i)
		{
			whitePalette[i] = 0xFFFFFFFF;
		}
		_renderContext->SetPalette(D2DX_WHITE_PALETTE_INDEX, whitePalette);
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

	uint32_t hash = fnv_32a_buf(pixels, pixelsSize, FNV1_32A_INIT);


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
	_scratchBatch.SetTextureCategory(_gameHelper->GetTextureCategoryFromHash(hash));

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

void D2DXContext::DrawBatches()
{
	const int32_t batchCount = (int32_t)_batchCount;

	Batch mergedBatch;
	int32_t drawCalls = 0;

	for (int32_t i = 0; i < batchCount; ++i)
	{
		const Batch& batch = _batches.items[i];

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
			if (_renderContext->GetTextureCache(batch) != _renderContext->GetTextureCache(mergedBatch) ||
				(batch.GetTextureAtlas() != mergedBatch.GetTextureAtlas()) ||
				batch.GetAlphaBlend() != mergedBatch.GetAlphaBlend() ||
				batch.GetPrimitiveType() != mergedBatch.GetPrimitiveType() ||
				((mergedBatch.GetVertexCount() + batch.GetVertexCount()) > 65535))
			{
				_renderContext->Draw(mergedBatch);
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
		_renderContext->Draw(mergedBatch);
		++drawCalls;
	}


	if (!(_frame & 31))
	{
		DEBUG_PRINT("Nr draw calls: %i", drawCalls);
	}
}

void D2DXContext::OnBufferSwap()
{
	CheckMajorGameState();
	InsertLogoOnTitleScreen();

	_renderContext->BulkWriteVertices(_vertices.items, _vertexCount);

	DrawBatches();

	_renderContext->Present();

	++_frame;

	_batchCount = 0;
	_vertexCount = 0;

	_lastScreenOpenMode = _gameHelper->ScreenOpenMode();

	_nextBatchId = 0;
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

	if (function == GR_COMBINE_FUNCTION_ZERO && factor == GR_COMBINE_FACTOR_ZERO && local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT)
	{
		alphaCombine = AlphaCombine::One;
	}
	else if (function == GR_COMBINE_FUNCTION_LOCAL && factor == GR_COMBINE_FACTOR_ZERO && local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT)
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
void D2DXContext::OnDrawLine(
	const void* v1,
	const void* v2,
	uint32_t gameContext)
{
	auto gameAddress = _gameHelper->IdentifyGameAddress(gameContext);

	Batch batch = _scratchBatch;
	batch.SetPrimitiveType(PrimitiveType::Triangles);
	batch.SetGameAddress(gameAddress);
	batch.SetStartVertex(_vertexCount);
	batch.SetTextureCategory(_gameHelper->RefineTextureCategoryFromGameAddress(batch.GetTextureCategory(), gameAddress));

	Vertex startVertex = ReadVertex((const uint8_t*)v1, _vertexLayout, batch, _batchId);
	Vertex endVertex = ReadVertex((const uint8_t*)v2, _vertexLayout, batch, _batchId);

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

	assert(_batchCount < _batches.capacity);
	_batches.items[_batchCount++] = batch;
}

_Use_decl_annotations_
Vertex D2DXContext::ReadVertex(
	const uint8_t* vertex,
	uint32_t vertexLayout,
	const Batch& batch,
	int32_t batchIndex)
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

	auto pargb = pargbOffset != 0xFF ? *(const uint32_t*)(vertex + pargbOffset) : 0xFFFFFFFF;

	if (batch.GetAlphaCombine() == AlphaCombine::One)
	{
		pargb |= 0xFF000000;
	}

	int32_t paletteIndex = batch.GetRgbCombine() == RgbCombine::ColorMultipliedByTexture ? batch.GetPaletteIndex() : D2DX_WHITE_PALETTE_INDEX;
	return Vertex((int32_t)xy[0], (int32_t)xy[1], s, t, batch.SelectColorAndAlpha(pargb, _constantColor), batch.IsChromaKeyEnabled(), batch.GetTextureIndex(), paletteIndex, batchIndex);
}

_Use_decl_annotations_
const Batch D2DXContext::PrepareBatchForSubmit(
	Batch batch,
	PrimitiveType primitiveType,
	uint32_t vertexCount,
	uint32_t gameContext) const
{
	auto gameAddress = _gameHelper->IdentifyGameAddress(gameContext);
	batch.SetPrimitiveType(PrimitiveType::Triangles);

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
void D2DXContext::UpdateBatchSurfaceId(
	Batch& batch)
{
	int32_t firstVertexInBatch = _vertexCount - batch.GetVertexCount();

	int32_t minx = INT_MAX;
	int32_t miny = INT_MAX;
	int32_t maxx = INT_MIN;
	int32_t maxy = INT_MIN;

	for (uint32_t i = 0; i < batch.GetVertexCount(); ++i)
	{
		int32_t x = (int32_t)_vertices.items[firstVertexInBatch + i].GetX();
		int32_t y = (int32_t)_vertices.items[firstVertexInBatch + i].GetY();
		minx = min(minx, x);
		miny = min(miny, y);
		maxx = max(maxx, x);
		maxy = max(maxy, y);
	}

	uint64_t drawCallTexture = (uint64_t)batch.GetTextureIndex() | ((uint64_t)batch.GetTextureAtlas() << 32ULL);

	switch (batch.GetTextureCategory())
	{
	case TextureCategory::MousePointer:
	case TextureCategory::UserInterface:
	case TextureCategory::Font:
	case TextureCategory::LoadingScreen:
	case TextureCategory::TitleScreen:
		_batchId = (1 << 14) - 1;
		break;
	case TextureCategory::Floor:
	case TextureCategory::Wall:
	case TextureCategory::Unknown:
	{
		if (
			(_previousDrawCallTexture == drawCallTexture) ||
			(
				(batch.GetWidth() == 32 && batch.GetHeight() == 32) &&
				(
					/* 32x32 wall block drawn to the left of the previous one. */
					(maxx == _previousDrawCallRect.offset.x && miny == _previousDrawCallRect.offset.y) ||

					/* 32x32 wall block drawn on the next row from the previous one. */
					(miny == (_previousDrawCallRect.offset.y + _previousDrawCallRect.size.height))
				)
			)
		)
		{
			_batchId = _nextBatchId;
		}
		else
		{
			_batchId = ++_nextBatchId;
		}
		break;
	}
	}

	for (uint32_t i = 0; i < batch.GetVertexCount(); ++i)
	{
		_vertices.items[firstVertexInBatch + i].SetSurfaceId(_batchId);
	}

	_previousDrawCallTexture = drawCallTexture;
	_previousDrawCallRect.offset.x = minx;
	_previousDrawCallRect.offset.y = miny;
	_previousDrawCallRect.size.width = maxx - minx;
	_previousDrawCallRect.size.height = maxy - miny;
}

_Use_decl_annotations_
void D2DXContext::OnDrawVertexArray(
	uint32_t mode,
	uint32_t count,
	uint8_t** pointers,
	uint32_t gameContext)
{
	Batch batch = PrepareBatchForSubmit(_scratchBatch, PrimitiveType::Triangles, (count - 2) * 3, gameContext);
	const uint32_t vertexLayout = _vertexLayout;

	switch (mode)
	{
	case GR_TRIANGLE_FAN:
	{
		Vertex firstVertex = ReadVertex((const uint8_t*)pointers[0], vertexLayout, batch, _batchId);
		Vertex prevVertex = ReadVertex((const uint8_t*)pointers[1], vertexLayout, batch, _batchId);

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex((const uint8_t*)pointers[i], vertexLayout, batch, _batchId);

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
		Vertex prevPrevVertex = ReadVertex((const uint8_t*)pointers[0], vertexLayout, batch, _batchId);
		Vertex prevVertex = ReadVertex((const uint8_t*)pointers[1], vertexLayout, batch, _batchId);

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex((const uint8_t*)pointers[i], vertexLayout, batch, _batchId);

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

	UpdateBatchSurfaceId(batch);

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
	const uint32_t vertexLayout = _vertexLayout;

	Rect batchRect;

	switch (mode)
	{
	case GR_TRIANGLE_FAN:
	{
		Vertex firstVertex = ReadVertex(vertex, _vertexLayout, batch, _batchId);
		vertex += stride;

		Vertex prevVertex = ReadVertex(vertex, _vertexLayout, batch, _batchId);
		vertex += stride;

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex(vertex, _vertexLayout, batch, _batchId);
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
		Vertex prevPrevVertex = ReadVertex(vertex, _vertexLayout, batch, _batchId);
		vertex += stride;

		Vertex prevVertex = ReadVertex(vertex, _vertexLayout, batch, _batchId);
		vertex += stride;

		for (uint32_t i = 2; i < count; ++i)
		{
			Vertex currentVertex = ReadVertex(vertex, _vertexLayout, batch, _batchId);
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

	UpdateBatchSurfaceId(batch);

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

	for (uint32_t i = 0; i < _paletteKeys.capacity; ++i)
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

	for (uint32_t i = 0; i < _paletteKeys.capacity; ++i)
	{
		if (_paletteKeys.items[i] == 0)
		{
			_paletteKeys.items[i] = hash;
			_scratchBatch.SetPaletteIndex(i);
			_renderContext->SetPalette(i, (const uint32_t*)data);
			return;
		}
	}

	assert(false && "Too many palettes.");
	ALWAYS_PRINT("Too many palettes.");
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
	const uint32_t* palette = (const uint32_t*)(dx_logo256 + 0x36);

	_renderContext->SetPalette(D2DX_LOGO_PALETTE_INDEX, palette);

	uint32_t hash = fnv_32a_buf((void*)srcPixels, sizeof(uint8_t) * 81 * 40, FNV1_32A_INIT);

	uint8_t* data = _sideTmuMemory.items;

	_logoTextureBatch.SetTextureStartAddress(0);
	_logoTextureBatch.SetTextureHash(hash);
	_logoTextureBatch.SetTextureSize(128, 128);
	_logoTextureBatch.SetTextureCategory(TextureCategory::TitleScreen);
	_logoTextureBatch.SetPrimitiveType(PrimitiveType::Triangles);
	_logoTextureBatch.SetAlphaBlend(AlphaBlend::SrcAlphaInvSrcAlpha);
	_logoTextureBatch.SetIsChromaKeyEnabled(true);
	_logoTextureBatch.SetRgbCombine(RgbCombine::ColorMultipliedByTexture);
	_logoTextureBatch.SetAlphaCombine(AlphaCombine::One);
	_logoTextureBatch.SetPaletteIndex(D2DX_LOGO_PALETTE_INDEX);
	_logoTextureBatch.SetVertexCount(6);

	memset(data, 0, _logoTextureBatch.GetWidth() * _logoTextureBatch.GetHeight());

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

	Vertex vertex0(x, y, 0, 0, color, true, _logoTextureBatch.GetTextureIndex(), D2DX_LOGO_PALETTE_INDEX, 16383);
	Vertex vertex1(x + 80, y, 80, 0, color, true, _logoTextureBatch.GetTextureIndex(), D2DX_LOGO_PALETTE_INDEX, 16383);
	Vertex vertex2(x + 80, y + 41, 80, 41, color, true, _logoTextureBatch.GetTextureIndex(), D2DX_LOGO_PALETTE_INDEX, 16383);
	Vertex vertex3(x, y + 41, 0, 41, color, true, _logoTextureBatch.GetTextureIndex(), D2DX_LOGO_PALETTE_INDEX, 16383);

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

		//OnMousePosChanged(mousePos);

		return pos;
	}

	return { -1, -1 };
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
		ALWAYS_PRINT("Suggesting game size %ix%i.", _suggestedGameSize.width, _suggestedGameSize.height);
	}

	return _suggestedGameSize;
}

void D2DXContext::DisableBuiltinResMod()
{
	_options.noResMod = true;
}
