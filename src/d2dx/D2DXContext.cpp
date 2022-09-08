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

extern D2::UnitAny* currentlyDrawingUnit;
extern uint32_t currentlyDrawingWeatherParticles;
extern uint32_t* currentlyDrawingWeatherParticleIndexPtr;

#define D2DX_GLIDE_ALPHA_BLEND(rgb_sf, rgb_df, alpha_sf, alpha_df) \
		(uint16_t)(((rgb_sf & 0xF) << 12) | ((rgb_df & 0xF) << 8) | ((alpha_sf & 0xF) << 4) | (alpha_df & 0xF))

static Options GetCommandLineOptions()
{
	Options options;
	auto fileData = ReadTextFile("d2dx.cfg");
	options.ApplyCfg(fileData.items);
	options.ApplyCommandLine(GetCommandLineA());
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
	_paletteKeys(D2DX_MAX_PALETTES, true),
	_batchCount(0),
	_batches(D2DX_MAX_BATCHES_PER_FRAME),
	_vertexCount(0),
	_vertices(D2DX_MAX_VERTICES_PER_FRAME),
	_customGameSize{ 0,0 },
	_suggestedGameSize{ 0, 0 },
	_options{ GetCommandLineOptions() },
	_lastScreenOpenMode{ 0 },
	_surfaceIdTracker{ gameHelper },
	_textMotionPredictor{ gameHelper },
	_unitMotionPredictor{ gameHelper },
	_weatherMotionPredictor{ gameHelper },
	_featureFlags{ 0 }
{
	_threadId = GetCurrentThreadId();

	if (!_options.GetFlag(OptionsFlag::NoCompatModeFix))
	{
		_compatibilityModeDisabler->DisableCompatibilityMode();
	}

	auto apparentWindowsVersion = GetWindowsVersion();
	auto actualWindowsVersion = GetActualWindowsVersion();
	D2DX_LOG("Apparent Windows version: %u.%u (build %u).", apparentWindowsVersion.major, apparentWindowsVersion.minor, apparentWindowsVersion.build);
	D2DX_LOG("Actual Windows version: %u.%u (build %u).", actualWindowsVersion.major, actualWindowsVersion.minor, actualWindowsVersion.build);

	if (!_options.GetFlag(OptionsFlag::NoResMod))
	{
		try
		{
			_builtinResMod = std::make_unique<BuiltinResMod>(GetModuleHandleA("glide3x.dll"), GetSuggestedCustomResolution(), _gameHelper);
			if (!_builtinResMod->IsActive())
			{
				_options.SetFlag(OptionsFlag::NoResMod, true);
			}
		}
		catch (...)
		{
			_options.SetFlag(OptionsFlag::NoResMod, true);
		}
	}

	if (!_options.GetFlag(OptionsFlag::NoFpsFix))
	{
		_gameHelper->TryApplyInGameFpsFix();
		_gameHelper->TryApplyMenuFpsFix();
		_gameHelper->TryApplyInGameSleepFixes();
	}
}

D2DXContext::~D2DXContext() noexcept
{
	DetachLateDetours();
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
	_threadId = GetCurrentThreadId();

	Size windowSize = _gameHelper->GetConfiguredGameSize();
	if (!_options.GetFlag(OptionsFlag::NoResMod))
	{
		windowSize = { 800,600 };
	}

	Size gameSize{ width, height };

	if (_customGameSize.width > 0)
	{
		gameSize = _customGameSize;
		_customGameSize = { 0,0 };
	}

	if (gameSize.width != 640 || gameSize.height != 480)
	{
		windowSize = gameSize;
	}

	if (!_renderContext)
	{
		auto initialScreenMode = strstr(GetCommandLineA(), "-w") ?
			ScreenMode::Windowed :
			ScreenMode::FullscreenDefault;

		_renderContext = std::make_shared<RenderContext>(
			(HWND)hWnd,
			gameSize,
			windowSize * _options.GetWindowScale(),
			initialScreenMode,
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
		_renderContext->SetSizes(gameSize, windowSize * _options.GetWindowScale());
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
		assert(offset == 0);
		break;
	case GR_PARAM_PARGB:
		assert(offset == 8);
		break;
	case GR_PARAM_ST0:
		assert(offset == 16);
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

	_textureHasher.Invalidate(startAddress);

	uint32_t memRequired = (uint32_t)(width * height);

	auto pStart = _glideState.tmuMemory.items + startAddress;
	auto pEnd = _glideState.tmuMemory.items + startAddress + memRequired;
	assert(pEnd <= (_glideState.tmuMemory.items + _glideState.tmuMemory.capacity));
	memcpy_s(pStart, _glideState.tmuMemory.capacity - startAddress, sourceAddress, memRequired);
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

	_readVertexState.isDirty = true;

	uint8_t* pixels = _glideState.tmuMemory.items + startAddress;
	const uint32_t pixelsSize = width * height;

	int32_t stShift = 0;
	_BitScanReverse((DWORD*)&stShift, max(width, height));
	_glideState.stShift = 8 - stShift;

	uint32_t hash = _textureHasher.GetHash(startAddress, pixels, pixelsSize);

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

	if (_options.GetFlag(OptionsFlag::DbgDumpTextures))
	{
		DumpTexture(hash, width, height, pixels, pixelsSize, (uint32_t)_scratchBatch.GetTextureCategory(), _glideState.palettes.items + _scratchBatch.GetPaletteIndex() * 256);
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

	if (_gameHelper->IsInGame())
	{
		_majorGameState = MajorGameState::InGame;
		AttachLateDetours(_gameHelper.get(), this);
	}
	else
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

	if (!(_frame & 255))
	{
		D2DX_DEBUG_LOG("Nr draw calls: %i", drawCalls);
	}
}


void D2DXContext::OnBufferSwap()
{
	CheckMajorGameState();
	InsertLogoOnTitleScreen();

	if (IsFeatureEnabled(Feature::UnitMotionPrediction) &&
		_majorGameState == MajorGameState::InGame)
	{
		const Offset offset = _unitMotionPredictor.GetOffset(_gameHelper->GetPlayerUnit());

		for (uint32_t i = 0; i < _batchCount; ++i)
		{
			const auto& batch = _batches.items[i];
			auto surfaceId = _vertices.items[batch.GetStartVertex()].GetSurfaceId();

			if (surfaceId != D2DX_SURFACE_ID_USER_INTERFACE &&
				batch.GetTextureCategory() != TextureCategory::Player)
			{
				const auto batchVertexCount = batch.GetVertexCount();
				auto vertexIndex = batch.GetStartVertex();
				for (uint32_t j = 0; j < batchVertexCount; ++j)
				{
					_vertices.items[vertexIndex++].AddOffset(
						-offset.x,
						-offset.y);
				}
			}
		}
	}

	auto startVertexLocation = _renderContext->BulkWriteVertices(_vertices.items, _vertexCount);

	DrawBatches(startVertexLocation);

	_skipCountingSleep = true;
	_renderContext->Present();
	_skipCountingSleep = false;

	++_frame;

	if (!(_frame & 255))
	{
		_textureHasher.PrintStats();

		D2DX_DEBUG_LOG("Sleeps/frame: %.2f", _sleeps / 256.0f);
		_sleeps = 0;
	}

	_batchCount = 0;
	_vertexCount = 0;

	_lastScreenOpenMode = _gameHelper->ScreenOpenMode();

	_surfaceIdTracker.OnNewFrame();

	_renderContext->GetCurrentMetrics(&_gameSize, nullptr, nullptr);

	_avgDir = { 0.0f, 0.0f };

	_readVertexState.isDirty = true;
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

	_readVertexState.isDirty = true;
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

	if (function == GR_COMBINE_FUNCTION_LOCAL && factor == GR_COMBINE_FACTOR_ZERO &&
		local == GR_COMBINE_LOCAL_CONSTANT && other == GR_COMBINE_OTHER_CONSTANT)
	{
		alphaCombine = AlphaCombine::FromColor;
	}

	_scratchBatch.SetAlphaCombine(alphaCombine);
}

_Use_decl_annotations_
void D2DXContext::OnConstantColorValue(
	uint32_t color)
{
	_glideState.constantColor = (color >> 8) | (color << 24);
	_readVertexState.isDirty = true;
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

	_readVertexState.isDirty = true;
}

_Use_decl_annotations_
void D2DXContext::OnDrawPoint(
	const void* pt,
	uint32_t gameContext)
{
	Batch batch = _scratchBatch;
	batch.SetGameAddress(GameAddress::Unknown);
	batch.SetStartVertex(_vertexCount);

	EnsureReadVertexStateUpdated(batch);

	auto vertex0 = _readVertexState.templateVertex;

	const uint32_t iteratedColorMask = _readVertexState.iteratedColorMask;
	const uint32_t maskedConstantColor = _readVertexState.maskedConstantColor;
	const int32_t stShift = _glideState.stShift;

	const D2::Vertex* d2Vertex = (const D2::Vertex*)pt;

	vertex0.SetPosition((int32_t)d2Vertex->x, (int32_t)d2Vertex->y);
	vertex0.SetTexcoord((int32_t)d2Vertex->s >> stShift, (int32_t)d2Vertex->t >> stShift);
	vertex0.SetColor(maskedConstantColor | (d2Vertex->color & iteratedColorMask));
	vertex0.SetSurfaceId(_surfaceIdTracker.GetCurrentSurfaceId());

	Vertex vertex1 = vertex0;
	Vertex vertex2 = vertex0;

	vertex1.AddOffset(1, 0);
	vertex2.AddOffset(1, 1);

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
	batch.SetPaletteIndex(D2DX_WHITE_PALETTE_INDEX);
	batch.SetTextureCategory(TextureCategory::UserInterface);

	EnsureReadVertexStateUpdated(batch);

	auto vertex0 = _readVertexState.templateVertex;
	vertex0.SetSurfaceId(D2DX_SURFACE_ID_USER_INTERFACE);

	const uint32_t iteratedColorMask = _readVertexState.iteratedColorMask;
	const uint32_t maskedConstantColor = _readVertexState.maskedConstantColor;

	const D2::Vertex* d2Vertex0 = (const D2::Vertex*)v1;
	const D2::Vertex* d2Vertex1 = (const D2::Vertex*)v2;

	vertex0.SetTexcoord((int32_t)d2Vertex1->s >> _glideState.stShift, (int32_t)d2Vertex1->t >> _glideState.stShift);
	vertex0.SetColor(maskedConstantColor | (d2Vertex1->color & iteratedColorMask));

	if (IsFeatureEnabled(Feature::WeatherMotionPrediction) &&
		currentlyDrawingWeatherParticles)
	{
		uint32_t currentWeatherParticleIndex = *currentlyDrawingWeatherParticleIndexPtr;
		const int32_t act = _gameHelper->GetCurrentAct();

		OffsetF startPos{ d2Vertex0->x, d2Vertex0->y };
		OffsetF endPos{ d2Vertex1->x, d2Vertex1->y };

		// Snow is drawn with two independent lines per particle index (different places on screen).
		// We solve this by tracking each line separately.
		if (currentWeatherParticleIndex == _lastWeatherParticleIndex)
		{
			currentWeatherParticleIndex += 256;
		}

		const auto offset = _weatherMotionPredictor.GetOffset(currentWeatherParticleIndex, startPos);
		startPos += offset;
		endPos += offset;

		auto dir = endPos - startPos;
		float len = dir.Length();
		dir.Normalize();

		const float blendFactor = 0.1f;
		const float oneMinusBlendFactor = 1.0f - blendFactor;

		if (_avgDir.x == 0.0f && _avgDir.y == 0.0f)
		{
			_avgDir = dir;
		}
		else
		{
			_avgDir = _avgDir * oneMinusBlendFactor + dir * blendFactor;
			_avgDir.Normalize();
		}
		dir = _avgDir;

		const OffsetF wideningVec{ -dir.y * 1.25f, dir.x * 1.25f };
		const float stretchBack = act == 4 ? 1.0f : 3.0f;
		const float stretchAhead = act == 4 ? 1.0f : 1.0f;

		auto midPos = startPos;
		startPos -= dir * len * stretchBack;
		endPos = startPos + dir * len * (stretchBack + stretchAhead);

		Vertex vertex1 = vertex0;
		Vertex vertex2 = vertex0;
		Vertex vertex3 = vertex0;
		Vertex vertex4 = vertex0;

		vertex0.SetPosition((int32_t)midPos.x, (int32_t)midPos.y);
		vertex1.SetPosition((int32_t)startPos.x, (int32_t)startPos.y);
		vertex2.SetPosition((int32_t)(midPos.x + wideningVec.x), (int32_t)(midPos.y + wideningVec.y));
		vertex3.SetPosition((int32_t)endPos.x, (int32_t)endPos.y);
		vertex4.SetPosition((int32_t)(midPos.x - wideningVec.x), (int32_t)(midPos.y - wideningVec.y));

		uint32_t c = vertex0.GetColor();
		c &= 0x00FFFFFF;
		vertex1.SetColor(c);
		vertex2.SetColor(c);
		vertex3.SetColor(c);
		vertex4.SetColor(c);

		assert((_vertexCount + 3 * 4) < _vertices.capacity);

		_vertices.items[_vertexCount++] = vertex0;
		_vertices.items[_vertexCount++] = vertex1;
		_vertices.items[_vertexCount++] = vertex2;

		_vertices.items[_vertexCount++] = vertex0;
		_vertices.items[_vertexCount++] = vertex2;
		_vertices.items[_vertexCount++] = vertex3;

		_vertices.items[_vertexCount++] = vertex0;
		_vertices.items[_vertexCount++] = vertex3;
		_vertices.items[_vertexCount++] = vertex4;

		_vertices.items[_vertexCount++] = vertex0;
		_vertices.items[_vertexCount++] = vertex4;
		_vertices.items[_vertexCount++] = vertex1;

		batch.SetVertexCount(3 * 4);

		_lastWeatherParticleIndex = currentWeatherParticleIndex;
	}
	else
	{
		OffsetF wideningVec = { d2Vertex0->y - d2Vertex1->y, d2Vertex1->x - d2Vertex0->x };
		const float len = wideningVec.Length();
		const float halfinvlen = 1.0f / (2.0f * len);
		wideningVec *= halfinvlen;

		Vertex vertex1 = vertex0;
		Vertex vertex2 = vertex0;
		Vertex vertex3 = vertex0;

		vertex0.SetPosition(
			(int32_t)(d2Vertex0->x - wideningVec.x),
			(int32_t)(d2Vertex0->y - wideningVec.y));

		vertex1.SetPosition(
			(int32_t)(d2Vertex0->x + wideningVec.x),
			(int32_t)(d2Vertex0->y + wideningVec.y));

		vertex2.SetPosition(
			(int32_t)(d2Vertex1->x - wideningVec.x),
			(int32_t)(d2Vertex1->y - wideningVec.y));

		vertex3.SetPosition(
			(int32_t)(d2Vertex1->x + wideningVec.x),
			(int32_t)(d2Vertex1->y + wideningVec.y));

		assert((_vertexCount + 6) < _vertices.capacity);
		_vertices.items[_vertexCount++] = vertex0;
		_vertices.items[_vertexCount++] = vertex1;
		_vertices.items[_vertexCount++] = vertex2;
		_vertices.items[_vertexCount++] = vertex1;
		_vertices.items[_vertexCount++] = vertex2;
		_vertices.items[_vertexCount++] = vertex3;

		batch.SetVertexCount(6);
	}

	assert(_batchCount < _batches.capacity);
	_batches.items[_batchCount++] = batch;
}

_Use_decl_annotations_
const Batch D2DXContext::PrepareBatchForSubmit(
	Batch batch,
	PrimitiveType primitiveType,
	uint32_t vertexCount,
	uint32_t gameContext) const
{
	auto gameAddress = _gameHelper->IdentifyGameAddress(gameContext);

	auto tcl = _renderContext->UpdateTexture(batch, _glideState.tmuMemory.items, _glideState.tmuMemory.capacity);

	if (tcl._textureAtlas < 0)
	{
		return batch;
	}

	batch.SetTextureAtlas(tcl._textureAtlas);
	batch.SetTextureIndex(tcl._textureIndex);

	batch.SetGameAddress(gameAddress);
	batch.SetStartVertex(_vertexCount);
	batch.SetVertexCount(vertexCount);
	batch.SetTextureCategory(_gameHelper->RefineTextureCategoryFromGameAddress(batch.GetTextureCategory(), gameAddress));
	return batch;
}

_Use_decl_annotations_
void D2DXContext::EnsureReadVertexStateUpdated(
	const Batch& batch)
{
	if (!_readVertexState.isDirty)
	{
		return;
	}

	_readVertexState.templateVertex = Vertex(
		0, 0,
		0, 0,
		0,
		batch.IsChromaKeyEnabled(),
		batch.GetTextureIndex(),
		batch.GetRgbCombine() == RgbCombine::ColorMultipliedByTexture ? batch.GetPaletteIndex() : D2DX_WHITE_PALETTE_INDEX,
		0);

	const bool isIteratedColor = batch.GetRgbCombine() == RgbCombine::ColorMultipliedByTexture;
	const uint32_t constantColorMask = isIteratedColor ? 0xFF000000 : 0xFFFFFFFF;
	_readVertexState.constantColorMask = constantColorMask;
	_readVertexState.iteratedColorMask = isIteratedColor ? 0x00FFFFFF : 0x00000000;
	_readVertexState.maskedConstantColor = constantColorMask & (_glideState.constantColor | (batch.GetAlphaBlend() != AlphaBlend::SrcAlphaInvSrcAlpha ? 0xFF000000 : 0));
	_readVertexState.isDirty = false;
}

_Use_decl_annotations_
void D2DXContext::OnDrawVertexArray(
	uint32_t mode,
	uint32_t count,
	uint8_t** pointers,
	uint32_t gameContext)
{
	assert(mode == GR_TRIANGLE_STRIP || mode == GR_TRIANGLE_FAN);

	if (count < 3 || (mode != GR_TRIANGLE_STRIP && mode != GR_TRIANGLE_FAN))
	{
		return;
	}

	Batch batch = PrepareBatchForSubmit(_scratchBatch, PrimitiveType::Triangles, 3 * (count - 2), gameContext);

	if (!batch.IsValid())
	{
		return;
	}

	EnsureReadVertexStateUpdated(batch);

	Vertex v = _readVertexState.templateVertex;

	const uint32_t iteratedColorMask = _readVertexState.iteratedColorMask;
	const uint32_t maskedConstantColor = _readVertexState.maskedConstantColor;

	Vertex* pVertices = &_vertices.items[_vertexCount];

	for (int32_t i = 0; i < 3; ++i)
	{
		const D2::Vertex* d2Vertex = (const D2::Vertex*)pointers[i];
		v.SetPosition((int32_t)d2Vertex->x, (int32_t)d2Vertex->y);
		v.SetTexcoord((int32_t)d2Vertex->s >> _glideState.stShift, (int32_t)d2Vertex->t >> _glideState.stShift);
		v.SetColor(maskedConstantColor | (d2Vertex->color & iteratedColorMask));
		*pVertices++ = v;
	}

	if (mode == GR_TRIANGLE_FAN)
	{
		auto vertex0 = pVertices[-3];

		for (uint32_t i = 0; i < (count - 3); ++i)
		{
			*pVertices++ = vertex0;
			*pVertices++ = pVertices[-2];
			const D2::Vertex* d2Vertex = (const D2::Vertex*)pointers[i + 3];
			v.SetPosition((int32_t)d2Vertex->x, (int32_t)d2Vertex->y);
			v.SetTexcoord((int32_t)d2Vertex->s >> _glideState.stShift, (int32_t)d2Vertex->t >> _glideState.stShift);
			v.SetColor(maskedConstantColor | (d2Vertex->color & iteratedColorMask));
			*pVertices++ = v;
		}
	}
	else
	{
		for (uint32_t i = 0; i < (count - 3); ++i)
		{
			*pVertices++ = pVertices[-2];
			*pVertices++ = pVertices[-2];
			const D2::Vertex* d2Vertex = (const D2::Vertex*)pointers[i + 3];
			v.SetPosition((int32_t)d2Vertex->x, (int32_t)d2Vertex->y);
			v.SetTexcoord((int32_t)d2Vertex->s >> _glideState.stShift, (int32_t)d2Vertex->t >> _glideState.stShift);
			v.SetColor(maskedConstantColor | (d2Vertex->color & iteratedColorMask));
			*pVertices++ = v;
		}
	}

	_vertexCount += 3 * (count - 2);

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
	assert(count == 4);
	assert(mode == GR_TRIANGLE_FAN);
	assert(stride == sizeof(D2::Vertex));

	if (mode != GR_TRIANGLE_FAN || count != 4 || stride != sizeof(D2::Vertex))
	{
		return;
	}

	Batch batch = PrepareBatchForSubmit(_scratchBatch, PrimitiveType::Triangles, 6, gameContext);

	if (!batch.IsValid())
	{
		return;
	}

	EnsureReadVertexStateUpdated(batch);

	const uint32_t iteratedColorMask = _readVertexState.iteratedColorMask;
	const uint32_t maskedConstantColor = _readVertexState.maskedConstantColor;

	const D2::Vertex* d2Vertices = (const D2::Vertex*)vertex;

	Vertex v = _readVertexState.templateVertex;

	Vertex* pVertices = &_vertices.items[_vertexCount];

	for (int32_t i = 0; i < 4; ++i)
	{
		v.SetPosition((int32_t)d2Vertices[i].x, (int32_t)d2Vertices[i].y);
		v.SetTexcoord((int32_t)d2Vertices[i].s >> _glideState.stShift, (int32_t)d2Vertices[i].t >> _glideState.stShift);
		v.SetColor(maskedConstantColor | (d2Vertices[i].color & iteratedColorMask));
		pVertices[i] = v;
	}

	pVertices[4] = pVertices[0];
	pVertices[5] = pVertices[2];

	_vertexCount += 6;

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

	_readVertexState.isDirty = true;

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

			if (_options.GetFlag(OptionsFlag::DbgDumpTextures))
			{
				memcpy(_glideState.palettes.items + 256 * i, palette, 1024);
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

	_readVertexState.isDirty = true;
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
		_glideState.gammaTable.items[i] = ((blue[i] & 0xFF) << 16) | ((green[i] & 0xFF) << 8) | (red[i] & 0xFF);
	}

	_renderContext->LoadGammaTable(_glideState.gammaTable.items, _glideState.gammaTable.capacity);
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

	uint8_t* data = _glideState.sideTmuMemory.items;

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
	if (_options.GetFlag(OptionsFlag::NoLogo) || _majorGameState != MajorGameState::TitleScreen || _batchCount <= 0)
		return;

	PrepareLogoTextureBatch();

	auto tcl = _renderContext->UpdateTexture(_logoTextureBatch, _glideState.sideTmuMemory.items, _glideState.sideTmuMemory.capacity);

	_logoTextureBatch.SetTextureAtlas(tcl._textureAtlas);
	_logoTextureBatch.SetTextureIndex(tcl._textureIndex);
	_logoTextureBatch.SetStartVertex(_vertexCount);

	Size gameSize;
	_renderContext->GetCurrentMetrics(&gameSize, nullptr, nullptr);

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

		const bool isFullscreen = _renderContext->GetScreenMode() == ScreenMode::FullscreenDefault;
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

	const bool isFullscreen = _renderContext->GetScreenMode() == ScreenMode::FullscreenDefault;
	const float scale = (float)renderRect.size.height / gameSize.height;
	const uint32_t scaledWidth = (uint32_t)(scale * gameSize.width);
	const float mouseOffsetX = isFullscreen ? (float)(desktopSize.width / 2 - scaledWidth / 2) : 0.0f;

	pos.x = (int32_t)(pos.x * scale + mouseOffsetX);
	pos.y = (int32_t)(pos.y * scale);

	return pos;
}

_Use_decl_annotations_
int32_t D2DXContext::OnSleep(
	int32_t ms)
{
	if (_skipCountingSleep || _threadId != GetCurrentThreadId())
	{
		return ms;
	}

	++_sleeps;

	if (_majorGameState == MajorGameState::InGame)
	{
		return ms;
	}

	return ms;
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
		if (_gameHelper->IsProjectDiablo2())
		{
			_suggestedGameSize = { 1068, 600 };
		}
		else
		{
			Size desktopSize{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

			_suggestedGameSize = _options.GetUserSpecifiedGameSize();
			_suggestedGameSize.width = min(_suggestedGameSize.width, desktopSize.width);
			_suggestedGameSize.height = min(_suggestedGameSize.height, desktopSize.height);

			if (_suggestedGameSize.width < 0 || _suggestedGameSize.height < 0)
			{
				_suggestedGameSize = Metrics::GetSuggestedGameSize(desktopSize, !_options.GetFlag(OptionsFlag::NoWide));
			}
		}

		D2DX_LOG("Suggesting game size %ix%i.", _suggestedGameSize.width, _suggestedGameSize.height);
	}

	return _suggestedGameSize;
}

void D2DXContext::DisableBuiltinResMod()
{
	_options.SetFlag(OptionsFlag::NoResMod, true);
}

const Options& D2DXContext::GetOptions() const
{
	return _options;
}

void D2DXContext::OnBufferClear()
{
	if (_majorGameState == MajorGameState::InGame)
	{
		if (IsFeatureEnabled(Feature::UnitMotionPrediction))
		{
			_unitMotionPredictor.Update(_renderContext.get());
		}

		if (IsFeatureEnabled(Feature::TextMotionPrediction))
		{
			_textMotionPredictor.Update(_renderContext.get());
		}

		if (IsFeatureEnabled(Feature::WeatherMotionPrediction))
		{
			_weatherMotionPredictor.Update(_renderContext.get());
		}
	}
}

_Use_decl_annotations_
Offset D2DXContext::BeginDrawText(
	wchar_t* str,
	Offset pos,
	uint32_t returnAddress,
	D2Function d2Function)
{
	_scratchBatch.SetTextureCategory(TextureCategory::UserInterface);
	_isDrawingText = true;

	Offset offset{ 0, 0 };

	if (!str)
	{
		return offset;
	}

	if (d2Function != D2Function::D2Win_DrawText && IsFeatureEnabled(Feature::TextMotionPrediction))
	{
		auto hash = fnv_32a_buf((void*)str, wcslen(str), FNV1_32A_INIT);

		const uint64_t textId =
			(((uint64_t)(returnAddress & 0xFFFFFF) << 40ULL) |
			((uint64_t)((uintptr_t)str & 0xFFFFFF) << 16ULL)) ^
			(uint64_t)hash;

		offset = _textMotionPredictor.GetOffset(textId, pos);
	}

	if (_gameHelper->GetVersion() == GameVersion::Lod114d)
	{
		// In 1.14d, some color codes are black. Remap them.

		// Bright white -> white
		while (wchar_t* subStr = wcsstr(str, L"ÿc/"))
		{
			subStr[2] = L'0';
		}
	}

	return offset;
}

void D2DXContext::EndDrawText()
{
	_scratchBatch.SetTextureCategory(TextureCategory::Unknown);
	_isDrawingText = false;
}

_Use_decl_annotations_
Offset D2DXContext::BeginDrawImage(
	const D2::CellContextAny* cellContext,
	uint32_t drawMode,
	Offset pos,
	D2Function d2Function)
{
	Offset offset{ 0,0 };

	if (_isDrawingText)
	{
		return offset;
	}

	if (currentlyDrawingUnit)
	{
		_unitMotionPredictor.SetUnitScreenPos(currentlyDrawingUnit, pos.x, pos.y);

		if (currentlyDrawingUnit == _gameHelper->GetPlayerUnit())
		{
			// The player unit itself.
			_scratchBatch.SetTextureCategory(TextureCategory::Player);
			_playerScreenPos = pos;
		}
		else
		{
			offset = _unitMotionPredictor.GetOffset(currentlyDrawingUnit);
		}
	}
	else
	{
		if (d2Function == D2Function::D2Gfx_DrawShadow)
		{
			const bool isPlayerShadow =
				_playerScreenPos.x > 0 &&
				max(abs(pos.x - _playerScreenPos.x), abs(pos.y - _playerScreenPos.y)) < 8;

			if (isPlayerShadow)
			{
				_scratchBatch.SetTextureCategory(TextureCategory::Player);
			}
			else
			{
				offset = _unitMotionPredictor.GetOffsetForShadow(pos.x, pos.y);
			}
		}
		else
		{
			DrawParameters drawParameters = _gameHelper->GetDrawParameters(cellContext);
			const bool isMiscUi = drawParameters.unitType == 0 && drawParameters.unitMode == 0 && drawMode != 3;
			const bool isBeltItem = drawParameters.unitType == 4 && drawParameters.unitMode == 4;

			if (isMiscUi || isBeltItem)
			{
				_scratchBatch.SetTextureCategory(TextureCategory::UserInterface);
			}
		}
	}

	return offset;
}

void D2DXContext::EndDrawImage()
{
	if (_isDrawingText)
	{
		return;
	}

	_scratchBatch.SetTextureCategory(TextureCategory::Unknown);
}

_Use_decl_annotations_
bool D2DXContext::IsFeatureEnabled(
	Feature feature)
{
	if (!_areFeatureFlagsInitialized)
	{
		const auto gameVersion = _gameHelper->GetVersion();

		_featureFlags = 0;
		D2DX_LOG("Feature flags:");

		if (!_options.GetFlag(OptionsFlag::NoMotionPrediction))
		{
			if (
				gameVersion == GameVersion::Lod109d ||
				gameVersion == GameVersion::Lod110f ||
				gameVersion == GameVersion::Lod112 ||
				gameVersion == GameVersion::Lod113c ||
				gameVersion == GameVersion::Lod113d ||
				gameVersion == GameVersion::Lod114d)
			{
				_featureFlags |= (uint32_t)Feature::UnitMotionPrediction;
				D2DX_LOG("  UnitMotionPrediction");
				_featureFlags |= (uint32_t)Feature::WeatherMotionPrediction;
				D2DX_LOG("  WeatherMotionPrediction");
				_featureFlags |= (uint32_t)Feature::TextMotionPrediction;
				D2DX_LOG("  TextMotionPrediction");
			}
		}

		_areFeatureFlagsInitialized = true;
	}

	return (_featureFlags & (uint32_t)feature) != 0;
}
