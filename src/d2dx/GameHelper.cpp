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
	_isPd2(LoadLibraryA("PD2_EXT.dll")),
	_version(GetGameVersion()),
	_hProcess(GetCurrentProcess()),
	_hGameExe(GetModuleHandleA("game.exe")),
	_hD2ClientDll(LoadLibraryA("D2Client.dll")),
	_hD2GfxDll(LoadLibraryA("D2Gfx.dll")),
	_hD2WinDll(LoadLibraryA("D2Win.dll"))
{
	InitializeTextureHashPrefixTable();
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
	case GameVersion::Lod110:
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
		return ReadU32(_hD2ClientDll, 0x115C10);
	case GameVersion::Lod110:
		return ReadU32(_hD2ClientDll, 0x10B9C4);
	case GameVersion::Lod112:
		return ReadU32(_hD2ClientDll, 0x11C1D0);
	case GameVersion::Lod113c:
		return ReadU32(_hD2ClientDll, 0x11C414);
	case GameVersion::Lod113d:
		return ReadU32(_hD2ClientDll, 0x11D070);
	case GameVersion::Lod114d:
		return ReadU32(_hGameExe, 0x3A5210);
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

_Use_decl_annotations_
void GameHelper::SetIngameMousePos(
	Offset pos)
{
	pos.x = max(0, pos.x);
	pos.y = max(0, pos.y);

	switch (_version)
	{
	case GameVersion::Lod109d:
		WriteU32(_hD2ClientDll, 0x12B168, (uint32_t)pos.x);
		WriteU32(_hD2ClientDll, 0x12B16C, (uint32_t)pos.y);
		break;
	case GameVersion::Lod110:
		WriteU32(_hD2ClientDll, 0x121AE4, (uint32_t)pos.x);
		WriteU32(_hD2ClientDll, 0x121AE8, (uint32_t)pos.y);
		break;
	case GameVersion::Lod112:
		WriteU32(_hD2ClientDll, 0x101638, (uint32_t)pos.x);
		WriteU32(_hD2ClientDll, 0x101634, (uint32_t)pos.y);
		break;
	case GameVersion::Lod113c:
		WriteU32(_hD2ClientDll, 0x11B828, (uint32_t)pos.x);
		WriteU32(_hD2ClientDll, 0x11B824, (uint32_t)pos.y);
		break;
	case GameVersion::Lod113d:
		WriteU32(_hD2ClientDll, 0x11C950, (uint32_t)pos.x);
		WriteU32(_hD2ClientDll, 0x11C94C, (uint32_t)pos.y);
		break;
	case GameVersion::Lod114d:
		WriteU32(_hGameExe, 0x3A6AB0, (uint32_t)pos.x);
		WriteU32(_hGameExe, 0x3A6AAC, (uint32_t)pos.y);
		break;
	}
}

uint32_t GameHelper::ReadU32(HANDLE hModule, uint32_t offset) const
{
	uint32_t result;
	DWORD bytesRead;
	ReadProcessMemory(_hProcess, (LPCVOID)((uintptr_t)hModule + offset), &result, sizeof(DWORD), &bytesRead);
	return result;
}

uint16_t GameHelper::ReadU16(HANDLE hModule, uint32_t offset) const
{
	uint16_t result;
	DWORD bytesRead;
	ReadProcessMemory(_hProcess, (LPCVOID)((uintptr_t)hModule + offset), &result, sizeof(WORD), &bytesRead);
	return result;
}

void GameHelper::WriteU32(HANDLE hModule, uint32_t offset, uint32_t value)
{
	DWORD bytesWritten;
	WriteProcessMemory(_hProcess, (LPVOID)((uintptr_t)hModule + offset), &value, 4, &bytesWritten);
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
	0x0050d39f, /* DrawWall1 */
	0x0050d3ae, /* DrawWall2 */
	0x50db03, /* DrawFloor */
	0x50a995, /* DrawShadow */
	0x50abdc,  /* DrawDynamic */
	0x0050c38d, /* DrawSomething1 */
	0x0050c0de, /* DrawSomething2 */
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
	case GameVersion::Lod110:
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

static const uint32_t titleScreenHashes[] = {
	0x0836bff0,	0x0d609152,	0x1df19dd6,	0x2c779942,	0x3a174cb2,	0x3d35f3c5,	0x3d4c8c14,	0x605f521f,	0x6b69636d,
	0x73059f7c,	0x8766b77a,	0x8af2178a,	0x90bdd994,	0x94e77d2d,	0xa66ac09c,	0xbe1a20c3,	0xc158e602,	0xc2625261,
	0xccf7cc94,	0xcee4c170,	0xd38a63df,	0xd4579523,	0xda6e064e,	0xe22a8bc4,	0xe2e6b0c7,	0xe9263199, 0xe1e211f9,
	0x2ac72136, 0x2f15f9de, 0x2dba4381, 0x5bbe76ab, 0x5fa60772, 0x7bb42e90, 0x08b64561, 0x8a09de96, 0x8b255624,
	0x8bceb8c5, 0x8be41271, 0x8c93dc24, 0x38ac989c, 0x42f99404, 0x49d1f478, 0x49f4099d, 0x57ff0b65, 0x87c0b98d,
	0x89aaf047, 0x128bd717, 0x169c4b8e, 0x234cae2e, 0x264fc41c, 0x282fe954, 0x467c7521, 0x614d3948, 0x874fb06e,
	0x968db1ce, 0x969ed6e4, 0x3034acfa, 0x18793782, 0x32561192, 0x23206047, 0xa6a88d0e, 0xa8b86316, 0xa8ba2a4f,
	0xa13c32b5, 0xafaa7b74, 0xb1450cb1, 0xbc7f5ddb, 0xbcf633e8, 0xbe9c50d5, 0xbec03b1c, 0xc0821e4c, 0xc5638e07,
	0xcae3f8e8, 0xd113d34d, 0xd4032a7f, 0xd5206c21, 0xd1149259, 0xe15c8e53, 0xe9174a70, 0xedf6f578, 0xf13dd4fb,
	0xf045cd36, 0xf169106c, 0xc9d4e158
};

static const uint32_t loadingScreenHashes[] = {
	0x0aa1834d, 0x1a7964a9, 0x2f5b86a7, 0x70a8cb14, 0x32965ce1, 0x897794ce, 0x3136b0ee, 0x32965ce1, 0xc2cc7e28,
	0x2a683b29, 0x01c37ff8
};

static const uint32_t mousePointerHashes[] = {
	0xfe34f8b7,	0x5cac0e94,	0x4b661cd1,	3432412206,	2611936918,	2932294163,	1166565234,	77145516, 1264983249,
	4264884407
};

static const uint32_t fontHashes[] = {
	0xff9d7cf8, 0xf7c4ad96, 0xd6af9583, 0xd029a5d5, 0xa90311b2, 0xadea0b9e, 0xa57e1ec0, 0xcf7af660, 0x00aab565,
	0x0c65ba68, 0x0e977727, 0x1f445ffa, 0x2d08bd33, 0x2ca97e69, 0x3c724844, 0x3caef328, 0x3f18b5f3, 0x3ff9c768,
	0x4a819824, 0x4c738a3c, 0x4d25e783, 0x05bf54f3, 0x5a64ee53, 0x5c4804e8, 0x5ce69df3, 0x5ce768fb, 0x5ea96f40,
	0x7dbd6631, 0x8a06f474, 0x9e8e6f10, 0x13cb47b5, 0x24f62232, 0x47b5fbd0, 0x51ede2ae, 0x52c7c4f6, 0x60d9ac94,
	0x62c3ffa3, 0x65bfca36, 0x68dcd701, 0x80fb0c0d, 0x85b3c815, 0x94faf8ad,	0x96c426dc,	0x155b3bb1,	0x245c21b8,
	0x356ee855,	0x460cfabb,	0x855e9a3b,	0x1087bb49, 0x2727c6d9,	0x06347dbb,	0x8835bc6c,	0x15188ed5,	0x19665c92,
	0x30993a13, 0x62656aab, 0x76727e83, 0x102836dc, 0x620706d6, 0x722324f8, 0x885069da, 0x3329672c, 0x4543469f,
	0x8480460c, 0x14180660, 0x25649726, 0x65314024, 0xab719489, 0xaf9ffed3, 0xaf349ec7, 0xb8b73c0a, 0xb10a6b9a,
	0xb38f7483, 0xbed68f8e, 0xc1cdab68, 0xc07afb8b, 0xc460bb6b, 0xc566bc8a, 0xc1570ebc, 0xc205717c, 0xcc8840fe,
	0xd2cd74fc, 0xd9088161, 0xdb69ea24, 0xdc15a3ed, 0xddfc5520, 0xdff6bcd4, 0xe24ba0a5, 0xe091efa4, 0xe286be37,
	0xeae6fa1f, 0xee0b8133, 0xebe9615c, 0xf9ab5f62, 0xf97d8adc, 0xf418ef8b, 0xf82003be, 0xf958391b, 0xfb690655,
	0xfee0039e, 0x928ff333, 0xea69c070, 0x0d947578, 0x1cbd91ad, 0x1fa526bc, 0x1fd46ccf, 0x1fdf5ea8, 0x3b2a1ce3,
	0x3c8de02e, 0x3d2ad5b5, 0x3d60f2dc, 0x3fa20cb4, 0x3fce911b, 0x4bea7b80, 0x4dc94979, 0x4fbf1544, 0x5d0174e2,
	0x6aa87c97, 0x6d14c7af, 0x6d6036ee, 0x7aaa18cc, 0x7ce199c3, 0x7d0461f1, 0x8d3f1a73, 0x09cbde64, 0x9a4b2631,
	0x9ef02de8, 0x23b1de7d, 0x25a23e43, 0x31f5175a, 0x37af945a, 0x38f29bc0, 0x55f63b97, 0x62f52414, 0x66ca38dd,
	0x92ec24b0, 0x387b8aee, 0x428e026c, 0x690bed2b, 0x692e4a4a, 0x847c91ef, 0x877a3b9b, 0x3167ee35, 0x3323bf9e,
	0x36869e78, 0x411028ed, 0x446142ed, 0x848018c0, 0xaf8d3a5c, 0xbf500226, 0xc4eb73ba, 0xcf13c17a, 0xde96ce8a,
	0xe2b0c0d3, 0xe3c5cef7, 0xe6111a39, 0xf1554ddc, 0xfd8f0259, 0xfd00624b, 0x1f383eca, 0x1fd7911a, 0x6aeda9b7,
	0x7ad97a63, 0x7b55503e, 0x9fcbd6fc, 0x73a4d426, 0x77cfbf47, 0x292f9384, 0x0643d1f1, 0x4781b657, 0x122498bf,
	0xc947ac10, 0x3328df8c, 0x3349e905, 0x7437f47f, 0x8045d474, 0xb6e04288, 0xb6fa9b81, 0xdd4cd45e, 0x1c8454a2,
	0x3d79266d, 0x5ca112e7, 0x5db2cc26, 0x7c418268, 0x8a12f6bb, 0x8c6f28ab, 0x57b0ca11, 0x87f9b907, 0x960a9d44,
	0x5226af30, 0x35194776, 0xe5886490, 0x23c5e864, 0xccd53ac2, 0x6b6126f3, 0x8d4a3260, 0x71c5c663, 0x3dac9128,
	0x3dfc33ce, 0x04d447b9, 0x4afc2cf3, 0x4baac923, 0x4c5a4e9f, 0x7f342730, 0x8d761f96, 0x9fa17a31, 0x22b0435d,
	0x43d5debe, 0x46f04050, 0x71f96193, 0x81b64156, 0x094eb2d1,	0x95cabf5d,	0x729fc87a, 0x660724cf, 0x684355a9,
	0x89536279, 0xb2198727, 0xbd566762, 0xc182ea8d, 0xcb569296, 0xd701749f, 0xda33a8e3, 0xebcfc5e7, 0xef6827d7,
	0xf2b185b8, 0xf8787f41, 0xf682657c, 0xf538790e, 0xf6cc1be3, 0xe992347d, 0xe489be84, 0xd5446f30, 0xd190fe55,
	0xc42b6fe3, 0xc7bbf5eb, 0xbe471694, 0xbe5cbd2e, 0xbc628684, 0xaec57ae9, 0xadf284ff, 0xacf053d5, 0xa38462c1,
	0xa253e919, 0xa19d55b1, 0x49091212, 0x16932965, 0x11925128, 0x521153b5, 0x275935d8, 0x331599b9, 0x195237ef,
	0x84975db9, 0x5485b7dd, 0x04496dfc, 0x3390f1cc, 0x0990bc41, 0x1079a332, 0x757f3830, 0x539ff448, 0x248be1d0,
	0x167b8dd1, 0x153ed279, 0x75ce2c45, 0x73d1e193, 0x72dbbd11, 0x72c3b9d1, 0x48fcc13c, 0x23a9a88b, 0x22bc1bc3,
	0x15c78e77, 0x9ff38c9c, 0x9b5464cd, 0x08b7ea6f, 0x6da5fdd9, 0x6c7fc8b2, 0x5cbacb93, 0x3f6d8396, 0x2f421fa3,
	0x2bb13649, 0x1c37c7af, 0x1b09d728, 0x0e107166, 0x01ea580e, 0x2ec3b8ce, 0x2c13208e, 0x9cca88da, 0x9ef9292c,
	0x13d447f4, 0x39b8cc8d, 0x035f2ee3, 0x57bdbc1b, 0x63ecc03a, 0x97ae8499, 0x877de3e8, 0x5376a8fd, 0xac2fa415,
	0xad00702c, 0xae8a3926, 0xcbce092d, 0xd456f540, 0xd0f2af58, 0xdcd00ab8, 0xdc6d8f42, 0xe42e180a, 0xee4e139f,
	0xe7836ebb, 0xe7015e8c, 0xfbd9771a, 0xfdce42e3, 0x4f75b1f9
};

static const uint32_t uiHashes[] = {
	0x2ff1fd61, 0x54cc8b72,	0xfc253c88, 0xabe12614, 0xa22f5459, 0xa0d8fb2a, 0x20526487, 0x8a3b7d58, 0x2ff1fd61, 
	0x54cc8b72, 0x76aa9aac, 0xef8d8978, 0x45e0af79, 0x9a008b35, 0x2a53bd89, 0x13d2c082, 0xab6ab811, 0xee7d31ba,
	0x6d1e37cf, 0xa4e86125, 0xa769824b, 0xb4119f58, 0xc2da4379, 0xdfbf045f, 0x88021112, 0x726eeaa0, 0x49e4e24e,
	0x3b50f3b6, 0x1e623206, 0xae502740, 0xd16d7f9a, 0xf6ec6116, 0x56acd7e4, 0x7656c190, 0xb0d15023, 0xb2c6e5fb,
	0x27d5991a, 0x21d8d615, 0x2bbf74be, 0x9ab19e53, 0x9ba9eeb2, 0x109348c9, 0x0f37086a, 0x10ac28d0, 0x5c121175,
	0x5c4d1125, 0xa1990293, 0xae25bff7, 0xb5855728, 0xc8f9d3f1, 0x2172d939, 0x0bd8d550, 0x62cfb0b8, 0x93e92b00,
	0x815a6925, 0x135190af, 0x3408446d, 0xaa265b2e, 0x316149fe, 0x63556155, 0xa9ba1eb0, 0xa9e34142, 0xa0564010,
	0xb0a058c2, 0xb037844a, 0xbbfee318, 0xc95d3136, 0xceadb1cd, 0xcef62ab8, 0xcfd7f4dd, 0xd8a1f81b, 0xd8df8f4b,
	0xd9dc1bdd, 0xdfe365f3, 0xee4f10d9, 0x4c389b09, 0x4c049e57, 0x4c8bda35, 0x4d234ffb, 0x4b2e9d5b, 0x1ffb1615,
	0x0a90d031, 0x5ed2fc41, 0x6b7e62ef, 0x6d05de67, 0x7acfc435, 0x7a742b36, 0x8d3366ec, 0x9ab19e5e, 0x933ba45c,
	0x977c13be, 0x7820ea79, 0x9643d531, 0x7111312a, 0x25534537, 0xc723c18e, 0xfa170b3f, 0x97c7e7f4, 0x8ce7ef63,
	0x45c78147, 0x5ca62551, 0xf8d429fb, 0xfee40e62, 
};

struct Hashes
{
	int32_t count;
	const uint32_t* hashes;
};

static const Hashes hashesPerCategory[(int)TextureCategory::Count] =
{
	{ 0, nullptr },
	{ ARRAYSIZE(mousePointerHashes), mousePointerHashes },
	{ ARRAYSIZE(fontHashes), fontHashes },
	{ ARRAYSIZE(loadingScreenHashes), loadingScreenHashes },
	{ /* Floor: don't bother keeping a list of hashes */ 0, nullptr },
	{ ARRAYSIZE(titleScreenHashes), titleScreenHashes },
	{ /* Wall: don't bother keeping a list of hashes */ 0, nullptr },
	{ ARRAYSIZE(uiHashes), uiHashes },
};

static bool isInitialized = false;
static Buffer<uint32_t> prefixTable[256];
static Buffer<uint32_t> prefixCounts(256, true);

void GameHelper::InitializeTextureHashPrefixTable()
{
	for (int32_t category = 0; category < ARRAYSIZE(hashesPerCategory); ++category)
	{
		const Hashes& hashes = hashesPerCategory[category];

		for (int32_t hashIndex = 0; hashIndex < hashes.count; ++hashIndex)
		{
			++prefixCounts.items[(hashes.hashes[hashIndex] >> 24)&0xFF];
		}
	}

	for (int32_t prefix = 0; prefix < 256; ++prefix)
	{
		const uint32_t count = prefixCounts.items[prefix];

		if (count > 0)
		{
			prefixTable[prefix] = Buffer<uint32_t>(count, true);
		}
		else
		{
			prefixTable[prefix] = Buffer<uint32_t>();
		}
	}

	for (int32_t category = 0; category < ARRAYSIZE(hashesPerCategory); ++category)
	{
		const Hashes& hashes = hashesPerCategory[category];
		for (int32_t j = 0; j < hashes.count; ++j)
		{
			const uint32_t hash = hashes.hashes[j];
			const uint32_t prefix = hash >> 24;
			const uint32_t position = --prefixCounts.items[prefix];
			prefixTable[prefix].items[position] = (((uint32_t)category & 0xFF) << 24) | (hash & 0x00FFFFFF);
		}
	}
}

_Use_decl_annotations_
TextureCategory GameHelper::GetTextureCategoryFromHash(
	uint32_t textureHash) const
{
	Buffer<uint32_t>& table = prefixTable[textureHash >> 24];

	if (table.capacity == 0)
	{
		return TextureCategory::Unknown;
	}

	for (int32_t i = 0; i < (int32_t)table.capacity; ++i)
	{
		uint32_t entry = table.items[i];

		if ((entry & 0x00FFFFFF) == (textureHash & 0x00FFFFFF))
			return (TextureCategory)(entry >> 24);
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

	if (_isPd2)
	{
		return version;
	}

	auto versionSize = GetFileVersionInfoSizeA("game.exe", nullptr);
	Buffer<uint8_t> verData(versionSize);

	if (!GetFileVersionInfoA("game.exe", NULL, verData.capacity, verData.items))
	{
		D2DX_LOG("Failed to get file version for game.exe.");
		return GameVersion::Unsupported;
	}

	uint32_t size = 0;
	const uint8_t* lpBuffer = nullptr;
	bool success = VerQueryValueA(verData.items, "\\", (VOID FAR* FAR*) &lpBuffer, &size);

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
		version = GameVersion::Lod110;
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

bool GameHelper::TryApplyFpsFix()
{
	/* The offsets taken from The Phrozen Keep: https://d2mods.info/forum/viewtopic.php?t=65239. */

	uint32_t expectedProbe = 0;
	uint32_t patchOffset0 = 0;
	uint32_t patchOffset1 = 0;
	HANDLE hModule = nullptr;

	switch (_version)
	{
	case GameVersion::Lod109d:
		hModule = _hD2ClientDll;
		patchOffset0 = 0x9B5F;
		patchOffset1 = 0x9B63;
		expectedProbe = 0x2B756FBB;
		break;
	case GameVersion::Lod110:
		hModule = _hD2ClientDll;
		patchOffset0 = 0xA2C9;
		expectedProbe = 0x2B75C085;
		break;
	case GameVersion::Lod112:
		hModule = _hD2ClientDll;
		patchOffset0 = 0x7D1E1;
		patchOffset1 = 0x7D1E5;
		expectedProbe = 0x35756FBD;
		break;
	case GameVersion::Lod113c:
		hModule = _hD2ClientDll;
		patchOffset0 = 0x44E51;
		patchOffset1 = 0x44E55;
		expectedProbe = 0x35756FBD;
		break;
	case GameVersion::Lod113d:
		hModule = _hD2ClientDll;
		patchOffset0 = 0x45EA1;
		patchOffset1 = 0x45EA5;
		expectedProbe = 0x35756FBD;
		break;
	case GameVersion::Lod114d:
		hModule = _hGameExe;
		patchOffset0 = 0x4F278;
		patchOffset1 = 0x4F27C;
		expectedProbe = 0x2475007A;
		break;
	}

	if (patchOffset0 == 0)
	{
		D2DX_LOG("Fps fix aborted: unsupported game version.");
		return false;
	}

	const uint32_t probe = ReadU32(hModule, patchOffset1 != 0 ? patchOffset1 : patchOffset0);

	if (probe != expectedProbe)
	{
		D2DX_LOG("Fps fix aborted: location appears to be patched already.");
		return false;
	}

	WriteU32(hModule, patchOffset0, 0x90909090);
	
	if (patchOffset1 != 0)
	{
		WriteU32(hModule, patchOffset1, 0x90909090);
	}

	D2DX_LOG("Fps fix applied.");
	return true;
}

Offset GameHelper::GetPlayerPos()
{
	Offset pos = { 0,0 };
	uint32_t* unit = nullptr;
	uint32_t* path = nullptr;

	static int32_t offset = 0;
	switch (_version)
	{
	case GameVersion::Lod112:
		unit = (uint32_t*)ReadU32(_hD2ClientDll, 0x11C3D0);
		break;
	case GameVersion::Lod113c:
		unit = (uint32_t*)ReadU32(_hD2ClientDll, 0x11BBFC);
		break;
	case GameVersion::Lod113d:
		unit = (uint32_t*)ReadU32(_hD2ClientDll, 0x11D050);
		break;
	case GameVersion::Lod114d:
		unit = (uint32_t*)ReadU32(_hD2ClientDll, 0x3A6A70);
		break;
	default:
		break;
	}
	if (!unit)
	{
		return pos;
	}
	path = (uint32_t*)unit[0x2c / 4];
	//D2DX_LOG("unit id %u %p %p", unit[1], unit, path);
	if (path)
	{
		pos.x = path[0];
		pos.y = path[1];
	}
	return pos;
}

Offset GameHelper::GetPlayerTargetPos() const
{
	Offset pos = { 0,0 };
	return pos;
}

_Use_decl_annotations_
void* GameHelper::GetFunction(
	D2Function function) const
{
	HMODULE hModule = nullptr;
	int32_t ordinal = 0;

	switch (_version)
	{
	case GameVersion::Lod109d:
	case GameVersion::Lod110:
		switch (function)
		{
		case D2Function::D2Gfx_DrawImage:
			hModule = _hD2GfxDll;
			ordinal = 10072;
			break;
		case D2Function::D2Gfx_DrawShadow:
			hModule = _hD2GfxDll;
			ordinal = 10075;
			break;
		case D2Function::D2Win_DrawText:
			hModule = _hD2WinDll;
			ordinal = 10117;
			break;
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
		case D2Function::D2Gfx_DrawShadow:
			hModule = _hD2GfxDll;
			ordinal = 10030;
			break;
		case D2Function::D2Win_DrawText:
			hModule = _hD2WinDll;
			ordinal = 10001;
			break;
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
		case D2Function::D2Gfx_DrawShadow:
			hModule = _hD2GfxDll;
			ordinal = 10011;
			break;
		case D2Function::D2Win_DrawText:
			hModule = _hD2WinDll;
			ordinal = 10096;
			break;
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
		case D2Function::D2Gfx_DrawShadow:
			hModule = _hD2GfxDll;
			ordinal = 10084;
			break;
		case D2Function::D2Win_DrawText:
			hModule = _hD2WinDll;
			ordinal = 10076;
			break;
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

	return GetProcAddress(hModule, MAKEINTRESOURCEA(ordinal));
}
