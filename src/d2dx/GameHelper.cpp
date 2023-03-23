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
#include "GameHelper.h"
#include "Buffer.h"
#include "Utils.h"

using namespace d2dx;

GameHelper::GameHelper() :
	_version(GetGameVersion()),
	_hProcess(GetCurrentProcess()),
	_hGameExe(GetModuleHandleA("game.exe")),
	_hD2ClientDll(LoadLibraryA("D2Client.dll")),
	_hD2CommonDll(LoadLibraryA("D2Common.dll")),
	_hD2GfxDll(LoadLibraryA("D2Gfx.dll")),
	_hD2WinDll(LoadLibraryA("D2Win.dll")),
	_isProjectDiablo2(GetModuleHandleA("PD2_EXT.dll") != nullptr)
{
	InitializeTextureHashPrefixTable();

	if (_isProjectDiablo2)
	{
		D2DX_LOG("Detected Project Diablo 2.");
	}
}

GameVersion GameHelper::GetVersion() const
{
	return _version;
}

_Use_decl_annotations_
const char* GameHelper::GetVersionString() const
{
	switch (_version)
	{
	case GameVersion::Lod109d:
		return "Lod109d";
	case GameVersion::Lod110f:
		return "Lod110";
	case GameVersion::Lod112:
		return "Lod112";
	case GameVersion::Lod113c:
		return "Lod113c";
	case GameVersion::Lod113d:
		return "Lod113d";
	case GameVersion::Lod114d:
		return "Lod114d";
	default:
		return "Unhandled";
	}
}

uint32_t GameHelper::ScreenOpenMode() const
{
	switch (_version)
	{
	case GameVersion::Lod109d:
		return *(const uint32_t*)((uint32_t)_hD2ClientDll + 0x115C10);
	case GameVersion::Lod110f:
		return *(const uint32_t*)((uint32_t)_hD2ClientDll + 0x10B9C4);
	case GameVersion::Lod112:
		return *(const uint32_t*)((uint32_t)_hD2ClientDll + 0x11C1D0);
	case GameVersion::Lod113c:
		return *(const uint32_t*)((uint32_t)_hD2ClientDll + 0x11C414);
	case GameVersion::Lod113d:
		return *(const uint32_t*)((uint32_t)_hD2ClientDll + 0x11D070);
	case GameVersion::Lod114d:
		return *(const uint32_t*)((uint32_t)_hGameExe + 0x3A5210);
	default:
		return 0;
	}
}

Size GameHelper::GetConfiguredGameSize() const
{
	HKEY hKey;
	LPCTSTR diablo2Key = TEXT("SOFTWARE\\Blizzard Entertainment\\Diablo II");
	LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, diablo2Key, 0, KEY_READ, &hKey);
	if (openRes != ERROR_SUCCESS)
	{
		return { 800, 600 };
	}

	DWORD type = REG_DWORD;
	DWORD size = 4;
	DWORD value = 0;
	auto queryRes = RegQueryValueExA(hKey, "Resolution", NULL, &type, (LPBYTE)&value, &size);
	assert(queryRes == ERROR_SUCCESS || queryRes == ERROR_MORE_DATA);

	RegCloseKey(hKey);

	if (value == 0)
	{
		return { 640, 480 };
	}
	else
	{
		return { 800, 600 };
	}
}

static const uint32_t gameAddresses_109d[] =
{
	0xFFFFFFFF,
	0x6f818468, /* DrawWall1 */
	0x6f818476, /* DrawWall2 */
	0x6f813e2f, /* DrawFloor */
	0, /* DrawShadow */
	0x6f815efb,  /* DrawDynamic */
	0, /* DrawSomething1 */
	0, /* DrawSomething2 */
};

static const uint32_t gameAddresses_110[] =
{
	0xFFFFFFFF,
	0x6f81840c, /* DrawWall1 */
	0x6f81841a, /* DrawWall2 */
	0x6f813e24, /* DrawFloor */
	0, /* DrawShadow */
	0x6f815ec9,  /* DrawDynamic */
	0, /* DrawSomething1 */
	0, /* DrawSomething2 */
};

static const uint32_t gameAddresses_112[] =
{
	0xFFFFFFFF,
	0x6f85a2f9, /* DrawWall1 */
	0x6f85a2eb, /* DrawWall2 */
	0x6f856d6c, /* DrawFloor */
	0, /* DrawShadow */
	0x6f8587a4,  /* DrawDynamic */
	0, /* DrawSomething1 */
	0, /* DrawSomething2 */
};

static const uint32_t gameAddresses_113c[] =
{
	0xFFFFFFFF,
	0x6f8567ab, /* DrawWall1 */
	0x6f8567b9, /* DrawWall2 */
	0x6f85befc, /* DrawFloor */
	0x50a995, /* DrawShadow */
	0x6f85a344,  /* DrawDynamic */
	0x0050c38d, /* DrawSomething1 */
	0x0050c0de, /* DrawSomething2 */
};

static const uint32_t gameAddresses_113d[] =
{
	0xFFFFFFFF,
	0x6f857199, /* DrawWall1 */
	0x6f85718b, /* DrawWall2 */
	0x6f85c17c, /* DrawFloor */
	0x6f859ef5, /* DrawShadow */
	0x6f859ce4,  /* DrawDynamic */
	0x0050c38d, /* DrawSomething1 */
	0x0050c0de, /* DrawSomething2 */
};

static const uint32_t gameAddresses_114d[] =
{
	0xFFFFFFFF,
	0x50d39f, /* DrawWall1 */
	0x50d3ae, /* DrawWall2 */
	0x50db03, /* DrawFloor */
	0x50a995, /* DrawShadow */
	0x50abdc,  /* DrawDynamic */
	0x50c38d, /* DrawSomething1 */
	0x50c0de, /* DrawSomething2 */
};

_Use_decl_annotations_
GameAddress GameHelper::IdentifyGameAddress(
	uint32_t returnAddress) const
{
	const uint32_t* gameAddresses = nullptr;
	uint32_t gameAddressCount = 0;

	switch (_version)
	{
	case GameVersion::Lod109d:
		gameAddresses = gameAddresses_109d;
		gameAddressCount = ARRAYSIZE(gameAddresses_109d);
		break;
	case GameVersion::Lod110f:
		gameAddresses = gameAddresses_110;
		gameAddressCount = ARRAYSIZE(gameAddresses_110);
		break;
	case GameVersion::Lod112:
		gameAddresses = gameAddresses_112;
		gameAddressCount = ARRAYSIZE(gameAddresses_112);
		break;
	case GameVersion::Lod113c:
		gameAddresses = gameAddresses_113c;
		gameAddressCount = ARRAYSIZE(gameAddresses_113c);
		break;
	case GameVersion::Lod113d:
		gameAddresses = gameAddresses_113d;
		gameAddressCount = ARRAYSIZE(gameAddresses_113d);
		break;
	case GameVersion::Lod114d:
		gameAddresses = gameAddresses_114d;
		gameAddressCount = ARRAYSIZE(gameAddresses_114d);
		break;
	}

	for (uint32_t i = 0; i < gameAddressCount; ++i)
	{
		if (gameAddresses[i] == returnAddress)
		{
			return (GameAddress)i;
		}
	}

	return GameAddress::Unknown;
}

static const uint64_t titleScreenHashes[] = {
	0xfc8f2eed371285b5, 0x96907cc971b21ee3, 0x7d849f6aa1447d31, 0x5e9c709869b8316d,
	0xbd45ae397020fc67, 0x23a390772b0222ea, 0x865f8346abb2b0d9, 0xe31c4cb3055e3a8d,
	0xdb4cd90a53248698, 0x2a3cc9a9d20d4db3, 0xca3be3239284f32f, 0x80c099eecf2a6942,
	0xec7012136b17117e, 0x74699d85f6187279, 0x9379d39d021ac250, 0x7de0e4c0daba8542,
	0xe9aa4bb40736c8de, 0x7a24842f873ff168, 0x50bba78dd3f32584, 0xd77c9406226f6956,
	0x87a494bf4e93e74f, 0xf4f499036d979cb7, 0x8acdbc41c219a6df, 0xb57b2cd6041b68f4,
	0x5066159e442bc227, 0x681a52b2951c2c38, 0xe7b54cf2f378880c, 0xf100a1df6dc613a8,
	0x56ce19ae67641c36, 0xd8e13b24338643f2, 0xdd57aa7b10b67404, 0x33cf71dece646746,
	0xcd5c413221984a2e, 0xea7566c3e002bd8e, 0x8e77520ff40ee4c2, 0x8fdc4a2a0fd631bd,
	0x238bea9815c3f973, 0xf2907136c3125d2c, 0x7230ff3dbca44fbb, 0xcb209eca11be4d14,
	0xb2563bf765d557bf, 0x8bcb0357fa8a9b08, 0xa5bda937a5bd2209, 0x2aa76ba776154b4e,
	0x89c68a8be4b140af, 0x6edaa930b0c0660f, 0x7b74f7fcd5545638, 0x03d26b3ad461f39f,
	0xaf3746a91374320c, 0x0f4cadccb1f3404d, 0xbfc810cec696cd2e, 0x432b20b9e1a2046b,
	0x18d721d91120d3e4, 0x93e7bc560692d279, 0xb630b0a252c9bca2, 0x3e702bc6db567e65,
	0x91bde8087ee8006d, 0x7acd30b6b4511bb7, 0x6f47fc2dcd50a97a, 0x4418e2e797b7ce52,
	0x4090bb2e5e6a693c, 0x6d570442d10faca9, 0xbfcc7a58c2ee639e, 0x82e4d812f0f3aed6,
	0xfad69328f46f1738, 0x20422f1052e2bd4c, 0x450937139a448ffa, 0x4c7fb11fdfb0f873,
	0xc19528c54185bc4a, 0x10e91abd767b6fa3, 0x862f282e4ddcd5b1, 0x637c08360f67a901,
	0x54da45b8d5abad3e, 0x61ca773cb62d80a2, 0xf3d8ca94a0bafdd9, 0x6871b887d9f0146a,
	0x345bc9f91667504e, 0xeda9fc46b3df5107, 0x32e7488d3b13ba6c, 0x5253d2a17f52de8f,
	0xb0224c8217b62785, 0xba48c447dd8658f3, 0x925eae01a2dd92da, 0x8b71533a1a83ffeb
};

static const uint64_t loadingScreenHashes[] = {
	0x58270212c5a4768d, 0x4477bbee1ea4b8a2, 0xd4e3c6a3bfd4ce1b, 0x8ca6ced7c6c0d734,
	0x606c7a4a496e2b2c, 0x7dcf019cb6c74404, 0x6e253c1ae8ea21cc, 0x2066d0bcb096d2b3,
	0xdb8e862ccd712f80, 0xde3f10dcf01955e5
};

static const uint64_t mousePointerHashes[] = {
	0x0bdc9347694341cb, 0xd1fee6ed16a5783e, 0x341060b68b792690, 0x62deb2c42b814e7a,
	0xcc18928425ed76a4, 0xadf4c562e863e034, 0x849ed13b1099f3d2, 0xb7ec5a798379c98a,
	0x341060b68b792690, 0x0bdc9347694341cb
};

static const uint64_t uiHashes[] = {
	0x61ea9339e47727cf, 0xebc436a1c9e7bd5c, 0x4776be200f252bfd, 0x6c97577b3427100e,
	0x909c3af232ec9862, 0x3072fd4554e3046d, 0xae67f6a51d8fbff9, 0xb8739a28c42f00cb,
	0x61ea9339e47727cf, 0xebc436a1c9e7bd5c, 0xe81f8a7753876b95, 0x90d5cb92d6728d54,
	0x1f86a45f0bf54ea2, 0x310577a9b9a828b1, 0x7c413f4fba0b21c7, 0x1a95d1819c2b43c8,
	0xddce4937693d1332, 0xe4b169b7349434cd, 0xdfab97e82d455355, 0xd805ab4831c51074,
	0x9982d719155a044a, 0x204f62cb2d1c285f, 0x5b6024bd14cd9e01, 0xf842fc082ed6fe4e,
	0x1c18a27092003174, 0xcfacaed2230190bd, 0xbf48185cb63fd3d2, 0xb4f126d671a639ce,
	0x125689dde109287a, 0x7aa2beb25068cc89, 0x4425e14b4ae3cf24, 0x822cc1078c13d492,
	0x0f46d52959d1d13e, 0xf4e536daddd19b51, 0x965859ed83ba43a4, 0xfada6a55fc3c9cb5,
	0x995d86044fa94a6d, 0xd35e53cb229a0821, 0x6fa425b25d2b7431, // 0x9ab19e53,
	0x02d9cfc298985f66, 0x20bec7e59ea72486, 0x53889e76e90425cc, 0xb8020e6fdaf3fa4e,
	0x26f48ea9913e3960, 0xc6b7c2662c89e69b, 0x8570a07d3bfdd552, 0xb9c4a5dab253cd24,
	0x9ba6fe4da4e4da3a, 0x41338dad07d0eff5, 0x87da5ecdaa7bf227, 0xfb9bb96fb37c6e72,
	0x5bd09cf9c2bf6d0b, 0x70599592f44ddd8e, 0xa9d86f028892e7d4, 0x9fe6f5585dcc5912,
	0xaae9844524173ccd, 0x38773f495a68cd78, 0xddff89f6293bed91, 0xffc644d63a52de50,
	0x1ec082485c3bcec1, 0x1acc82ab691aad80, 0x13b4e6089de39b2f, 0xc10c6faaf6a5c8c1,
	0xd8ac60e0c232ff3f, 0x3181169544173cca, 0xd7b6d6b3c17e47d5, 0xd9e803bbe66a9af6,
	0x079b6dd4f8b130d1, 0xabf81493ab8ed123, 0x5d1bbdafbf9273db, 0x0d58079c62d8ae0f,
	0x4a8564a6a61ac5ba, 0x1bdde15b74038ead, 0x14b5477a1081758a, 0xb67b235ef048ce77,
	0x93bd201ea9d54ae8, 0xb540c7813054a2d0, 0x2cf8b1237fe8964f, 0x5f61548466cbe18f,
	0xd61d211d4ae4ff17, 0x1cb148c09def586c, 0x935474ab2dde5c52, 0x9a765f736f8d4e4b,
	0x18d6348b2985509b, 0xceb1daaeb9d11407, 0x749673a482d9448d, 0xd7c6a5f92297b42a,
	0xcfa21d1b7b6c9ee4, 0x28662b3ccc58adab, 0x5c55461a7c3d3882, 0xf5cb72ad42436d6d,
	0xbaeb21121d9d78d5, 0x0de17ace2b416aaa, 0x1c4b042d87da3a51, 0x178e89564991fd9d,
	0xe50890802978d7a5, 0x50586fe7613c60fb, 0x19b236606b253645, 0xb52e1ec8b4d081c6,
	0x777d28620ac31160, 0x24f0d5312460cded, 0x44c1f0be0d8c71f5
};

struct Hashes
{
	int32_t count;
	const uint64_t* hashes;
};

static const Hashes hashesPerCategory[(int)TextureCategory::Count] =
{
	{ 0, nullptr },
	{ ARRAYSIZE(mousePointerHashes), mousePointerHashes },
	{ 0, nullptr },
	{ ARRAYSIZE(loadingScreenHashes), loadingScreenHashes },
	{ /* Floor: don't bother keeping a list of hashes */ 0, nullptr },
	{ ARRAYSIZE(titleScreenHashes), titleScreenHashes },
	{ /* Wall: don't bother keeping a list of hashes */ 0, nullptr },
	{ ARRAYSIZE(uiHashes), uiHashes },
};

static bool isInitialized = false;
static Buffer<uint64_t> prefixTable[256];
static Buffer<uint32_t> prefixCounts(256, true);

void GameHelper::InitializeTextureHashPrefixTable()
{
	for (int32_t category = 0; category < ARRAYSIZE(hashesPerCategory); ++category)
	{
		const Hashes& hashes = hashesPerCategory[category];

		for (int32_t hashIndex = 0; hashIndex < hashes.count; ++hashIndex)
		{
			++prefixCounts.items[(uint32_t)(hashes.hashes[hashIndex] >> 56) & 0xFF];
		}
	}

	for (int32_t prefix = 0; prefix < 256; ++prefix)
	{
		const uint32_t count = prefixCounts.items[prefix];

		if (count > 0)
		{
			prefixTable[prefix] = Buffer<uint64_t>(count, true);
		}
		else
		{
			prefixTable[prefix] = Buffer<uint64_t>();
		}
	}

	for (int32_t category = 0; category < ARRAYSIZE(hashesPerCategory); ++category)
	{
		const Hashes& hashes = hashesPerCategory[category];
		for (int32_t j = 0; j < hashes.count; ++j)
		{
			const uint64_t hash = hashes.hashes[j];
			const uint32_t prefix = static_cast<uint32_t>(hash >> 56);
			const uint32_t position = --prefixCounts.items[prefix];
			prefixTable[prefix].items[position] = (((uint64_t)category & 0xFF) << 56) | (hash & 0x00FFFFFF'FFFFFFFFULL);
		}
	}
}

_Use_decl_annotations_
TextureCategory GameHelper::GetTextureCategoryFromHash(
	uint64_t textureHash) const
{
	Buffer<uint64_t>& table = prefixTable[static_cast<uint32_t>(textureHash >> 56)];

	if (table.capacity == 0)
	{
		return TextureCategory::Unknown;
	}

	for (int32_t i = 0; i < (int32_t)table.capacity; ++i)
	{
		uint64_t entry = table.items[i];

		if ((entry & 0x00FFFFFF'FFFFFFFFULL) == (textureHash & 0x00FFFFFF'FFFFFFFFULL))
			return (TextureCategory)(entry >> 56);
	}

	return TextureCategory::Unknown;
}

_Use_decl_annotations_
TextureCategory GameHelper::RefineTextureCategoryFromGameAddress(
	TextureCategory previousCategory,
	GameAddress gameAddress) const
{
	if (previousCategory != TextureCategory::Unknown)
	{
		return previousCategory;
	}

	switch (gameAddress)
	{
	case GameAddress::DrawFloor:
		return TextureCategory::Floor;
		break;
	case GameAddress::DrawWall1:
	case GameAddress::DrawWall2:
		return TextureCategory::Wall;
		break;
	default:
		return previousCategory;
	}
}

GameVersion GameHelper::GetGameVersion()
{
	GameVersion version = GameVersion::Unsupported;

	auto versionSize = GetFileVersionInfoSizeA("game.exe", nullptr);
	Buffer<uint8_t> verData(versionSize);

	if (!GetFileVersionInfoA("game.exe", NULL, verData.capacity, verData.items))
	{
		D2DX_LOG("Failed to get file version for game.exe.");
		return GameVersion::Unsupported;
	}

	uint32_t size = 0;
	const uint8_t* lpBuffer = nullptr;
	bool success = VerQueryValueA(verData.items, "\\", (VOID FAR * FAR*) & lpBuffer, &size);

	if (!(success && size > 0))
	{
		D2DX_LOG("Failed to query version info for game.exe.");
		return GameVersion::Unsupported;
	}

	VS_FIXEDFILEINFO* vsFixedFileInfo = (VS_FIXEDFILEINFO*)lpBuffer;
	if (vsFixedFileInfo->dwSignature != 0xfeef04bd)
	{
		D2DX_LOG("Unexpected signature in version info for game.exe.");
		return GameVersion::Unsupported;
	}

	const int32_t a = vsFixedFileInfo->dwFileVersionMS >> 16;
	const int32_t b = vsFixedFileInfo->dwFileVersionMS & 0xffff;
	const int32_t c = vsFixedFileInfo->dwFileVersionLS >> 16;
	const int32_t d = vsFixedFileInfo->dwFileVersionLS & 0xffff;

	if (a == 1 && b == 0 && c == 9 && d == 22)
	{
		version = GameVersion::Lod109d;
	}
	else if (a == 1 && b == 0 && c == 10 && d == 9)
	{
		version = GameVersion::Lod110f;
	}
	else if (a == 1 && b == 0 && c == 12 && d == 49)
	{
		version = GameVersion::Lod112;
	}
	else if (a == 1 && b == 0 && c == 13 && d == 60)
	{
		version = GameVersion::Lod113c;
	}
	else if (a == 1 && b == 0 && c == 13 && d == 64)
	{
		version = GameVersion::Lod113d;
	}
	else if (a == 1 && b == 14 && c == 3 && d == 68)
	{
		D2DX_FATAL_ERROR("This version (1.14b) of Diablo II will not work with D2DX. Please upgrade to version 1.14d.");
	}
	else if (a == 1 && b == 14 && c == 3 && d == 71)
	{
		version = GameVersion::Lod114d;
	}

	if (version == GameVersion::Unsupported)
	{
		MessageBoxA(NULL, "This version of Diablo II is not supported by D2DX. Please upgrade or downgrade to a supported version.", "D2DX", MB_OK);
	}

	D2DX_LOG("Game version: %d.%d.%d.%d (%s)\n", a, b, c, d, version == GameVersion::Unsupported ? "unsupported" : "supported");

	return version;
}

bool GameHelper::TryApplyInGameFpsFix()
{
	/* The offsets taken from The Phrozen Keep: https://d2mods.info/forum/viewtopic.php?t=65239. */

	switch (_version)
	{
	case GameVersion::Lod109d:
		if (ProbeUInt32(_hD2ClientDll, 0x9B63, 0x2B756FBB))
		{
			PatchUInt32(_hD2ClientDll, 0x9B5F, 0x90909090);
			PatchUInt32(_hD2ClientDll, 0x9B63, 0x90909090);
		}
		break;
	case GameVersion::Lod110f:
		if (ProbeUInt32(_hD2ClientDll, 0xA2C9, 0x2B75C085))
		{
			PatchUInt32(_hD2ClientDll, 0xA2C9, 0x90909090);
		}
		break;
	case GameVersion::Lod112:
		if (ProbeUInt32(_hD2ClientDll, 0x7D1E5, 0x35756FBD))
		{
			PatchUInt32(_hD2ClientDll, 0x7D1E1, 0x90909090);
			PatchUInt32(_hD2ClientDll, 0x7D1E5, 0x90909090);
		}
		break;
	case GameVersion::Lod113c:
		if (ProbeUInt32(_hD2ClientDll, 0x44E4D, 0xFFFC8455))
		{
			PatchUInt32(_hD2ClientDll, 0x44E51, 0x90909090);
			PatchUInt32(_hD2ClientDll, 0x44E55, 0x90909090);
		}
		break;
	case GameVersion::Lod113d:
		if (ProbeUInt32(_hD2ClientDll, 0x45E9D, 0xFFFC738B))
		{
			PatchUInt32(_hD2ClientDll, 0x45EA1, 0x90909090);
			PatchUInt32(_hD2ClientDll, 0x45EA5, 0x90909090);
		}
		break;
	case GameVersion::Lod114d:
		if (ProbeUInt32(_hGameExe, 0x4F274, 0x000C6A68))
		{
			PatchUInt32(_hGameExe, 0x4F278, 0x90909090);
			PatchUInt32(_hGameExe, 0x4F27C, 0x90909090);
		}
		break;
	default:
		D2DX_LOG("Fps fix aborted: unsupported game version.");
		return false;
	}

	D2DX_LOG("Fps fix applied.");
	return true;
}

bool GameHelper::TryApplyMenuFpsFix()
{
	/* Patches found using 1.10 lead from D2Tweaks: https://github.com/Revan600/d2tweaks/. */

	switch (_version) 
	{
	case GameVersion::Lod109d:
		if (ProbeUInt32(_hD2WinDll, 0xEC0C, 0x5051196A))
		{
			PatchUInt32(_hD2WinDll, 0xEC0C, 0x50517F6A);
		}
		break;
	case GameVersion::Lod110f:
		if (ProbeUInt32(_hD2WinDll, 0xD029, 0x8128C783))
		{
			PatchUInt32(_hD2WinDll, 0xD029, 0x81909090);
		}
		break;
	case GameVersion::Lod112:
		if (ProbeUInt32(_hD2WinDll, 0xD949, 0x8128C783))
		{
			PatchUInt32(_hD2WinDll, 0xD949, 0x81909090);
		}
		break;
	case GameVersion::Lod113c:
		if (ProbeUInt32(_hD2WinDll, 0x18A19, 0x8128C783))
		{
			PatchUInt32(_hD2WinDll, 0x18A19, 0x81909090);
		}
		break;
	case GameVersion::Lod113d:
		if (ProbeUInt32(_hD2WinDll, 0xED69, 0x8128C783))
		{
			PatchUInt32(_hD2WinDll, 0xED69, 0x81909090);
		}
		break;
	case GameVersion::Lod114d:
		if (ProbeUInt32(_hGameExe, 0xFA62B, 0x8128C783))
		{
			PatchUInt32(_hGameExe, 0xFA62B, 0x81909090);
		}
		break;
	default:
		D2DX_LOG("Menu fps fix aborted: unsupported game version.");
		return false;
	}

	D2DX_LOG("Menu fps fix applied.");
	return true;
}

bool GameHelper::TryApplyInGameSleepFixes()
{
	switch (_version)
	{
	case GameVersion::Lod110f: 
		if (ProbeUInt32(_hD2ClientDll, 0x2684, 0x15FF0A6A))
		{
			PatchUInt32(_hD2ClientDll, 0x2684, 0x90909090);
			PatchUInt32(_hD2ClientDll, 0x2688, 0x90909090);
		}
		if (ProbeUInt32(_hD2ClientDll, 0x9E68, 0x83D7FF53))
		{
			PatchUInt32(_hD2ClientDll, 0x9E68, 0x83909090);
		}
		if (ProbeUInt32(_hD2ClientDll, 0x9E8C, 0x83D7FF53))
		{
			PatchUInt32(_hD2ClientDll, 0x9E8C, 0x83909090);
		}
		break;
	case GameVersion::Lod112:
		if (ProbeUInt32(_hD2ClientDll, 0x6CFD4, 0x15FF0A6A))
		{
			PatchUInt32(_hD2ClientDll, 0x6CFD4, 0x90909090);
		}

		if (ProbeUInt32(_hD2ClientDll, 0x6CFD8, 0x6FB7EF7C))
		{
			PatchUInt32(_hD2ClientDll, 0x6CFD8, 0x90909090);
		}

		if (ProbeUInt32(_hD2ClientDll, 0x7BD18, 0xD3FF006A))
		{
			PatchUInt32(_hD2ClientDll, 0x7BD18, 0x90909090);
		}

		if (ProbeUInt32(_hD2ClientDll, 0x7BD3D, 0xD3FF006A))
		{
			PatchUInt32(_hD2ClientDll, 0x7BD3D, 0x90909090);
		}
		break;
	case GameVersion::Lod113c:
		if (ProbeUInt32(_hD2WinDll, 0x18A63, 0xC815FF50) &&
			ProbeUInt32(_hD2WinDll, 0x18A67, 0xA16F8FB2))
		{
			PatchUInt32(_hD2WinDll, 0x18A63, 0x90909090);
			PatchUInt32(_hD2WinDll, 0x18A67, 0xA1909090);
		}

		if (ProbeUInt32(_hD2ClientDll, 0x3CB92, 0x0A6A0874))
		{
			PatchUInt32(_hD2ClientDll, 0x3CB92, 0x0A6A08EB);
		}

		if (ProbeUInt32(_hD2ClientDll, 0x43988, 0xD3FF006A))
		{
			PatchUInt32(_hD2ClientDll, 0x43988, 0x90909090);
		}

		if (ProbeUInt32(_hD2ClientDll, 0x439AD, 0xD3FF006A))
		{
			PatchUInt32(_hD2ClientDll, 0x439AD, 0x90909090);
		}

		break;
	case GameVersion::Lod113d:
		if (ProbeUInt32(_hD2WinDll, 0xEDB3, 0xB815FF50))
		{
			PatchUInt32(_hD2WinDll, 0xEDB3, 0x90909090);
		}

		if (ProbeUInt32(_hD2WinDll, 0xEDB7, 0xA16F8FB2))
		{
			PatchUInt32(_hD2WinDll, 0xEDB7, 0xA1909090);
		}

		if (ProbeUInt32(_hD2ClientDll, 0x27724, 0x15FF0A6A))
		{
			PatchUInt32(_hD2ClientDll, 0x27724, 0x90909090);
		}

		if (ProbeUInt32(_hD2ClientDll, 0x27728, 0x6FB7FF6C))
		{
			PatchUInt32(_hD2ClientDll, 0x27728, 0x90909090);
		}

		if (ProbeUInt32(_hD2ClientDll, 0x4494D, 0xD3FF006A))
		{
			PatchUInt32(_hD2ClientDll, 0x4494D, 0x90909090);
		}

		if (ProbeUInt32(_hD2ClientDll, 0x44928, 0xD3FF006A))
		{
			PatchUInt32(_hD2ClientDll, 0x44928, 0x90909090);
		}

		break;
	case GameVersion::Lod114d:
		if (ProbeUInt32(_hGameExe, 0x51C42, 0x15FF0A6A))
		{
			PatchUInt32(_hGameExe, 0x51C42, 0x90909090);
		}

		if (ProbeUInt32(_hGameExe, 0x51C46, 0x006CC258))
		{
			PatchUInt32(_hGameExe, 0x51C46, 0x90909090);
		}

		if (ProbeUInt32(_hGameExe, 0x4C711, 0xD7FF006A))
		{
			PatchUInt32(_hGameExe, 0x4C711, 0x90909090);
		}

		if (ProbeUInt32(_hGameExe, 0x4C740, 0xD7FF006A))
		{
			PatchUInt32(_hGameExe, 0x4C740, 0x90909090);
		}
		break;
	default:
		D2DX_LOG("In-game sleep fixes aborted: unsupported game version.");
		return false;
	}

	D2DX_LOG("In-game sleep fixes applied.");
	return true;
}

typedef D2::UnitAny* (__stdcall* GetClientPlayerFunc)();

D2::UnitAny* GameHelper::GetPlayerUnit() const
{
	GetClientPlayerFunc getClientPlayerFunc;

	switch (_version)
	{
	case GameVersion::Lod109d:
		getClientPlayerFunc = (GetClientPlayerFunc)((uintptr_t)_hD2ClientDll + 0x8CFC0);
		return getClientPlayerFunc();
	case GameVersion::Lod110f:
		getClientPlayerFunc = (GetClientPlayerFunc)((uintptr_t)_hD2ClientDll + 0x883D0);
		return getClientPlayerFunc();
	case GameVersion::Lod112:
		return (D2::UnitAny*)*(const uint32_t*)((uint32_t)_hD2ClientDll + 0x11C3D0);
	case GameVersion::Lod113c:
		return (D2::UnitAny*)*(const uint32_t*)((uint32_t)_hD2ClientDll + 0x11BBFC);
	case GameVersion::Lod113d:
		return (D2::UnitAny*)*(const uint32_t*)((uint32_t)_hD2ClientDll + 0x11D050);
	case GameVersion::Lod114d:
		return (D2::UnitAny*)*(const uint32_t*)((uint32_t)_hGameExe + 0x3A6A70);
	default:
		return nullptr;
	}
}

_Use_decl_annotations_
bool GameHelper::ProbeUInt32(
	HANDLE hModule, 
	uint32_t offset, 
	uint32_t probeValue)
{ 
	uint32_t* patchLocation = (uint32_t*)((uint32_t)hModule + offset);

	if (*patchLocation != probeValue)
	{
		//D2DX_LOG("Probe failed at %#010x, expected %#010x but found %#010x.", offset, probeValue, *patchLocation);
		return false;
	}

	return true;
}

_Use_decl_annotations_
void GameHelper::PatchUInt32(
	HANDLE hModule,
	uint32_t offset,
	uint32_t value)
{
	uint32_t* patchLocation = (uint32_t*)((uint32_t)hModule + offset);

	DWORD dwOldPage;
	VirtualProtect(patchLocation, 4, PAGE_EXECUTE_READWRITE, &dwOldPage);
	
	*patchLocation = value;

	VirtualProtect(patchLocation, 4, dwOldPage, &dwOldPage);
}

_Use_decl_annotations_
D2::UnitType GameHelper::GetUnitType(
	const D2::UnitAny* unit) const
{
	if (_version == GameVersion::Lod109d)
	{
		return unit->u.v109.dwType;
	}
	else
	{
		return unit->u.v112.dwType;
	}
}

_Use_decl_annotations_
uint32_t GameHelper::GetUnitId(
	const D2::UnitAny* unit) const
{
	if (_version == GameVersion::Lod109d)
	{
		return unit->u.v109.dwUnitId;
	}
	else
	{
		return unit->u.v112.dwUnitId;
	}
}

_Use_decl_annotations_
Offset GameHelper::GetUnitPos(
	const D2::UnitAny* unit) const
{
	auto unitType = GetUnitType(unit);

	if (unitType == D2::UnitType::Player ||
		unitType == D2::UnitType::Monster ||
		unitType == D2::UnitType::Missile)
	{
		D2::Path* path = _version == GameVersion::Lod109d ? unit->u.v109.path2 : unit->u.v112.path;
		return { (int32_t)path->x, (int32_t)path->y };
	}
	else
	{
		D2::StaticPath* path = _version == GameVersion::Lod109d ? unit->u.v109.staticPath2 : unit->u.v112.staticPath;
		return { (int32_t)path->xPos * 65536 + (int32_t)path->xOffset, (int32_t)path->yPos * 65536 + (int32_t)path->yOffset };
	}
}

typedef D2::UnitAny* (__fastcall* FindUnitFunc)(DWORD dwId, DWORD dwType);

static D2::UnitAny* __fastcall FindClientSideUnit109d(DWORD unitId, DWORD unitType)
{
	uint32_t** unitPtrTable = (uint32_t**)0x6FBC4BF8;
	uint32_t* unit = unitPtrTable[unitType * 128 + (unitId & 127)];

	while (unit)
	{
		if (unit[0] == unitType && unit[2] == unitId)
		{
			return (D2::UnitAny*)unit;
		}

		unit = (uint32_t*)unit[66];
	}

	return nullptr;
}

static D2::UnitAny* __fastcall FindServerSideUnit109d(DWORD unitId, DWORD unitType)
{
	uint32_t** unitPtrTable = (uint32_t**)0x6FBC57F8;
	uint32_t* unit = unitPtrTable[unitType * 128 + (unitId & 127)];

	while (unit)
	{
		if (unit[0] == unitType && unit[2] == unitId)
		{
			return (D2::UnitAny*)unit;
		}

		unit = (uint32_t*)unit[66];
	}

	return nullptr;
}

_Use_decl_annotations_
D2::UnitAny* GameHelper::FindUnit(
	uint32_t unitId,
	D2::UnitType unitType) const
{
	FindUnitFunc findClientSideUnit = (FindUnitFunc)GetFunction(D2Function::D2Client_FindClientSideUnit);
	FindUnitFunc findServerSideUnit = (FindUnitFunc)GetFunction(D2Function::D2Client_FindServerSideUnit);

	if (findClientSideUnit)
	{
		auto unit = findClientSideUnit((DWORD)unitId, (DWORD)unitType);

		if (unit)
		{
			return unit;
		}
	}

	return findServerSideUnit ? findServerSideUnit((DWORD)unitId, (DWORD)unitType) : nullptr;
}

_Use_decl_annotations_
void* GameHelper::GetFunction(
	D2Function function) const
{
	HANDLE hModule = nullptr;
	int32_t ordinal = 0;

	switch (_version)
	{
	case GameVersion::Lod109d:
		switch (function)
		{
		case D2Function::D2Gfx_DrawImage:
			hModule = _hD2GfxDll;
			ordinal = 10072;
			break;
		case D2Function::D2Gfx_DrawShiftedImage:
			hModule = _hD2GfxDll;
			ordinal = 10073;
			break;
		case D2Function::D2Gfx_DrawVerticalCropImage:
			hModule = _hD2GfxDll;
			ordinal = 10074;
			break;
		case D2Function::D2Gfx_DrawClippedImage:
			hModule = _hD2GfxDll;
			ordinal = 10077;
			break;
		case D2Function::D2Gfx_DrawImageFast:
			hModule = _hD2GfxDll;
			ordinal = 10076;
			break;
		case D2Function::D2Gfx_DrawShadow:
			hModule = _hD2GfxDll;
			ordinal = 10075;
			break;
		case D2Function::D2Win_DrawText:
			hModule = _hD2WinDll;
			ordinal = 10117;
			break;
		case D2Function::D2Client_DrawUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0xB8350);
		case D2Function::D2Client_FindClientSideUnit:
			return (void*)FindClientSideUnit109d;
		case D2Function::D2Client_DrawWeatherParticles:
			return (void*)((uintptr_t)_hD2ClientDll + 0x07BC0);
		case D2Function::D2Client_FindServerSideUnit:
			return (void*)FindServerSideUnit109d;
		default:
			break;
		}
		break;
	case GameVersion::Lod110f:
		switch (function)
		{
		case D2Function::D2Gfx_DrawImage:
			hModule = _hD2GfxDll;
			ordinal = 10072;
			break;
		case D2Function::D2Gfx_DrawShiftedImage:
			hModule = _hD2GfxDll;
			ordinal = 10073;
			break;
		case D2Function::D2Gfx_DrawVerticalCropImage:
			hModule = _hD2GfxDll;
			ordinal = 10074;
			break;
		case D2Function::D2Gfx_DrawClippedImage:
			hModule = _hD2GfxDll;
			ordinal = 10077;
			break;
		case D2Function::D2Gfx_DrawImageFast:
			hModule = _hD2GfxDll;
			ordinal = 10076;
			break;
		case D2Function::D2Gfx_DrawShadow: 
			hModule = _hD2GfxDll;
			ordinal = 10075;
			break; 
		case D2Function::D2Win_DrawText:
			hModule = _hD2WinDll;
			ordinal = 10117;
			break;
		case D2Function::D2Client_DrawUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0xBA720);
		case D2Function::D2Client_FindClientSideUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0x86BE0);
		case D2Function::D2Client_DrawWeatherParticles:
			return (void*)((uintptr_t)_hD2ClientDll + 0x08690);
		case D2Function::D2Client_FindServerSideUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0x86C70);
		default:
			break;
		}
		break;
	case GameVersion::Lod112:
		switch (function)
		{
		case D2Function::D2Gfx_DrawImage:
			hModule = _hD2GfxDll;
			ordinal = 10024;
			break;
		case D2Function::D2Gfx_DrawShiftedImage:
			hModule = _hD2GfxDll;
			ordinal = 10044;
			break;
		case D2Function::D2Gfx_DrawVerticalCropImage:
			hModule = _hD2GfxDll;
			ordinal = 10046;
			break;
		case D2Function::D2Gfx_DrawClippedImage:
			hModule = _hD2GfxDll;
			ordinal = 10061;
			break;
		case D2Function::D2Gfx_DrawImageFast:
			hModule = _hD2GfxDll;
			ordinal = 10012;
			break;
		case D2Function::D2Gfx_DrawShadow:
			hModule = _hD2GfxDll;
			ordinal = 10030;
			break;
		case D2Function::D2Win_DrawText:
			hModule = _hD2WinDll;
			ordinal = 10001;
			break;
		//case D2Function::D2Win_DrawFramedText:
		//	hModule = _hD2WinDll;
		//	ordinal = 10137;
		//	break;
		//case D2Function::D2Win_DrawRectangledText:
		//	hModule = _hD2WinDll;
		//	ordinal = 10078;
		//	break;
		case D2Function::D2Client_DrawUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0x94250);
		case D2Function::D2Client_DrawMissile:
			return (void*)((uintptr_t)_hD2ClientDll + 0x949C0);
		case D2Function::D2Client_DrawWeatherParticles:
			return (void*)((uintptr_t)_hD2ClientDll + 0x14210);
		case D2Function::D2Client_FindClientSideUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0x1F1A0);
		case D2Function::D2Client_FindServerSideUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0x1F1C0);
		default:
			break;
		}
		break;
	case GameVersion::Lod113c:
		switch (function)
		{
		case D2Function::D2Gfx_DrawImage:
			hModule = _hD2GfxDll;
			ordinal = 10041;
			break;
		case D2Function::D2Gfx_DrawShiftedImage:
			hModule = _hD2GfxDll;
			ordinal = 10019;
			break;
		case D2Function::D2Gfx_DrawVerticalCropImage:
			hModule = _hD2GfxDll;
			ordinal = 10074;
			break;
		case D2Function::D2Gfx_DrawClippedImage:
			hModule = _hD2GfxDll;
			ordinal = 10079;
			break;
		case D2Function::D2Gfx_DrawImageFast:
			hModule = _hD2GfxDll;
			ordinal = 10046;
			break;
		case D2Function::D2Gfx_DrawShadow:
			hModule = _hD2GfxDll;
			ordinal = 10011;
			break;
		case D2Function::D2Win_DrawText:
			hModule = _hD2WinDll;
			ordinal = 10096;
			break;
		case D2Function::D2Win_DrawFramedText:
			hModule = _hD2WinDll; 
			ordinal = 10085;
			break;
		case D2Function::D2Win_DrawRectangledText:
			hModule = _hD2WinDll;
			ordinal = 10013;
			break;
		case D2Function::D2Client_DrawUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0x6C490);
		case D2Function::D2Client_DrawMissile:
			return (void*)((uintptr_t)_hD2ClientDll + 0x6CC00);
		case D2Function::D2Client_DrawWeatherParticles:
			return (void*)((uintptr_t)_hD2ClientDll + 0x7FE80);
		case D2Function::D2Client_FindClientSideUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0xA5B20);
		case D2Function::D2Client_FindServerSideUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0xA5B40);
		default:
			break;
		}
		break;
	case GameVersion::Lod113d:
		switch (function)
		{
		case D2Function::D2Gfx_DrawImage:
			hModule = _hD2GfxDll;
			ordinal = 10042;
			break;
		case D2Function::D2Gfx_DrawShiftedImage:
			hModule = _hD2GfxDll;
			ordinal = 10067;
			break;
		case D2Function::D2Gfx_DrawVerticalCropImage:
			hModule = _hD2GfxDll;
			ordinal = 10082;
			break;
		case D2Function::D2Gfx_DrawClippedImage:
			hModule = _hD2GfxDll;
			ordinal = 10015;
			break;
		case D2Function::D2Gfx_DrawImageFast:
			hModule = _hD2GfxDll;
			ordinal = 10006;
			break;
		case D2Function::D2Gfx_DrawShadow:
			hModule = _hD2GfxDll;
			ordinal = 10084;
			break;
		case D2Function::D2Win_DrawText:
			hModule = _hD2WinDll;
			ordinal = 10076;
			break;
		case D2Function::D2Win_DrawTextEx:
			hModule = _hD2WinDll;
			ordinal = 10084;
			break;
		case D2Function::D2Win_DrawFramedText:
			hModule = _hD2WinDll;
			ordinal = 10137;
			break;
		case D2Function::D2Win_DrawRectangledText:
			hModule = _hD2WinDll;
			ordinal = 10078;
			break;
		case D2Function::D2Client_DrawUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0x605b0);
		case D2Function::D2Client_DrawMissile:
			return (void*)((uintptr_t)_hD2ClientDll + 0x60C70);
		case D2Function::D2Client_DrawWeatherParticles:
			return (void*)((uintptr_t)_hD2ClientDll + 0x4AD90);
		case D2Function::D2Client_FindClientSideUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0x620B0);
		case D2Function::D2Client_FindServerSideUnit:
			return (void*)((uintptr_t)_hD2ClientDll + 0x620D0);
		default:
			break;
		}
		break;
	case GameVersion::Lod114d:
		switch (function)
		{
		case D2Function::D2Gfx_DrawImage:
			return (void*)((uintptr_t)_hGameExe + 0xF6480);
		case D2Function::D2Gfx_DrawShiftedImage:
			return (void*)((uintptr_t)_hGameExe + 0xF64B0);
		case D2Function::D2Gfx_DrawVerticalCropImage:
			return (void*)((uintptr_t)_hGameExe + 0xF64E0);
		case D2Function::D2Gfx_DrawClippedImage:
			return (void*)((uintptr_t)_hGameExe + 0xF6510);
		case D2Function::D2Gfx_DrawImageFast:
			return (void*)((uintptr_t)_hGameExe + 0xF6570);
		case D2Function::D2Gfx_DrawShadow:
			return (void*)((uintptr_t)_hGameExe + 0xF6540);
		case D2Function::D2Win_DrawText:
			return (void*)((uintptr_t)_hGameExe + 0x102320);
		case D2Function::D2Win_DrawTextEx:
			return (void*)((uintptr_t)_hGameExe + 0x102360);
		case D2Function::D2Win_DrawFramedText:
			return (void*)((uintptr_t)_hGameExe + 0x102280);
		case D2Function::D2Win_DrawRectangledText:
			return (void*)((uintptr_t)_hGameExe + 0x1023B0);
		case D2Function::D2Client_DrawUnit:
			return (void*)((uintptr_t)_hGameExe + 0x70EC0);
		case D2Function::D2Client_DrawMissile:
			return (void*)((uintptr_t)_hGameExe + 0x71EC0);
		case D2Function::D2Client_DrawWeatherParticles:
			return (void*)((uintptr_t)_hGameExe + 0x73470);
		case D2Function::D2Client_FindClientSideUnit:
			return (void*)((uintptr_t)_hGameExe + 0x63990);
		case D2Function::D2Client_FindServerSideUnit:
			return (void*)((uintptr_t)_hGameExe + 0x639B0);
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (!hModule || !ordinal)
	{
		return nullptr;
	}

	return GetProcAddress((HMODULE)hModule, MAKEINTRESOURCEA(ordinal));
}

_Use_decl_annotations_
DrawParameters GameHelper::GetDrawParameters(
	const D2::CellContext* cellContext) const
{
	return
	{
		.unitId = GetVersion() == GameVersion::Lod109d || GetVersion() == GameVersion::Lod110f ? cellContext->_8 : cellContext->dwClass,
		.unitType = GetVersion() == GameVersion::Lod109d || GetVersion() == GameVersion::Lod110f ? cellContext->_9 : cellContext->dwUnit,
		.unitToken = GetVersion() == GameVersion::Lod109d || GetVersion() == GameVersion::Lod110f ? cellContext->_11 : cellContext->dwPlayerType
	};
}

int32_t GameHelper::GetCurrentAct() const
{
	auto unit = GetPlayerUnit();

	if (!unit)
	{
		return -1;
	}

	return (int32_t)unit->u.v112.dwAct;
}

bool GameHelper::IsGameMenuOpen() const
{
	switch (_version)
	{
	case GameVersion::Lod109d:
		return *((uint32_t*)((uint32_t)_hD2ClientDll + 0x1248D8)) != 0;
	case GameVersion::Lod110f:
		return *((uint32_t*)((uint32_t)_hD2ClientDll + 0x11A6CC)) != 0;
	case GameVersion::Lod112:
		return *((uint32_t*)((uint32_t)_hD2ClientDll + 0x102B7C)) != 0;
	case GameVersion::Lod113c:
		return *((uint32_t*)((uint32_t)_hD2ClientDll + 0xFADA4)) != 0;
	case GameVersion::Lod113d:
		return *((uint32_t*)((uint32_t)_hD2ClientDll + 0x11C8B4)) != 0;
	case GameVersion::Lod114d:
		return *((uint32_t*)((uint32_t)_hGameExe + 0x3A27E4)) != 0;
	default:
		return false;
	}
}

bool GameHelper::IsInGame() const
{
	auto playerUnit = GetPlayerUnit();

	switch (_version)
	{
	case GameVersion::Lod109d:
		return *((uint32_t*)((uint32_t)_hD2ClientDll + 0x1109FC)) != 0 && playerUnit != 0 && playerUnit->u.v109.path != 0;
	case GameVersion::Lod110f:
		return *((uint32_t*)((uint32_t)_hD2ClientDll + 0x1077C4)) != 0 && playerUnit != 0 && playerUnit->u.v109.path != 0;
	case GameVersion::Lod112:
		return *((uint32_t*)((uint32_t)_hD2ClientDll + 0x11BCC4)) != 0 && playerUnit != 0 && playerUnit->u.v112.path != 0;
	case GameVersion::Lod113c:
		return *((uint32_t*)((uint32_t)_hD2ClientDll + 0xF8C9C)) != 0 && playerUnit != 0 && playerUnit->u.v112.path != 0;
	case GameVersion::Lod113d:
		return *((uint32_t*)((uint32_t)_hD2ClientDll + 0xF79E0)) != 0 && playerUnit != 0 && playerUnit->u.v112.path != 0;
	case GameVersion::Lod114d:
		return *((uint32_t*)((uint32_t)_hGameExe + 0x3A27C0)) != 0 && playerUnit != 0 && playerUnit->u.v112.path != 0;
	default:
		return false;
	}
}

bool GameHelper::IsProjectDiablo2() const
{
	return _isProjectDiablo2;
}
