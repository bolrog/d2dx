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
#include "SimdSse2.h"

using namespace d2dx;
using namespace std;

_Use_decl_annotations_
int32_t SimdSse2::IndexOfUInt32(
	const uint32_t* items,
	uint32_t itemsCount,
	uint32_t item)
{
	assert(items && ((uintptr_t)items & 63) == 0);
	assert(!(itemsCount & 0x3F));

	const __m128i key4 = _mm_set1_epi32(item);

	int32_t findIndex = -1;
	uint32_t i = 0;
	uint64_t res = 0;

	/* Note: don't tweak this loop. It manages to fit within the XMM registers on x86
	   and any change could cause temporaries to spill onto the stack. */

	for (; i < itemsCount; i += 64)
	{
		const __m128i ck0 = _mm_load_si128((const __m128i*) & items[i + 0]);
		const __m128i ck1 = _mm_load_si128((const __m128i*) & items[i + 4]);
		const __m128i ck2 = _mm_load_si128((const __m128i*) & items[i + 8]);
		const __m128i ck3 = _mm_load_si128((const __m128i*) & items[i + 12]);
		const __m128i ck4 = _mm_load_si128((const __m128i*) & items[i + 16]);
		const __m128i ck5 = _mm_load_si128((const __m128i*) & items[i + 20]);
		const __m128i ck6 = _mm_load_si128((const __m128i*) & items[i + 24]);
		const __m128i ck7 = _mm_load_si128((const __m128i*) & items[i + 28]);
		
		const __m128i cmp0 = _mm_cmpeq_epi32(key4, ck0);
		const __m128i cmp1 = _mm_cmpeq_epi32(key4, ck1);
		const __m128i cmp2 = _mm_cmpeq_epi32(key4, ck2);
		const __m128i cmp3 = _mm_cmpeq_epi32(key4, ck3);
		const __m128i cmp4 = _mm_cmpeq_epi32(key4, ck4);
		const __m128i cmp5 = _mm_cmpeq_epi32(key4, ck5);
		const __m128i cmp6 = _mm_cmpeq_epi32(key4, ck6);
		const __m128i cmp7 = _mm_cmpeq_epi32(key4, ck7);
		
		const __m128i pack01 = _mm_packs_epi32(cmp0, cmp1);
		const __m128i pack23 = _mm_packs_epi32(cmp2, cmp3);
		const __m128i pack45 = _mm_packs_epi32(cmp4, cmp5);
		const __m128i pack67 = _mm_packs_epi32(cmp6, cmp7);
		
		const __m128i pack0123 = _mm_packs_epi16(pack01, pack23);
		const __m128i pack4567 = _mm_packs_epi16(pack45, pack67);

		const __m128i ck8 = _mm_load_si128((const __m128i*) & items[i + 32]);
		const __m128i ck9 = _mm_load_si128((const __m128i*) & items[i + 36]);
		const __m128i ckA = _mm_load_si128((const __m128i*) & items[i + 40]);
		const __m128i ckB = _mm_load_si128((const __m128i*) & items[i + 44]);
		const __m128i ckC = _mm_load_si128((const __m128i*) & items[i + 48]);
		const __m128i ckD = _mm_load_si128((const __m128i*) & items[i + 52]);
		const __m128i ckE = _mm_load_si128((const __m128i*) & items[i + 56]);
		const __m128i ckF = _mm_load_si128((const __m128i*) & items[i + 60]);

		const __m128i cmp8 = _mm_cmpeq_epi32(key4, ck8);
		const __m128i cmp9 = _mm_cmpeq_epi32(key4, ck9);
		const __m128i cmpA = _mm_cmpeq_epi32(key4, ckA);
		const __m128i cmpB = _mm_cmpeq_epi32(key4, ckB);
		const __m128i cmpC = _mm_cmpeq_epi32(key4, ckC);
		const __m128i cmpD = _mm_cmpeq_epi32(key4, ckD);
		const __m128i cmpE = _mm_cmpeq_epi32(key4, ckE);
		const __m128i cmpF = _mm_cmpeq_epi32(key4, ckF);

		const __m128i pack89 = _mm_packs_epi32(cmp8, cmp9);
		const __m128i packAB = _mm_packs_epi32(cmpA, cmpB);
		const __m128i packCD = _mm_packs_epi32(cmpC, cmpD);
		const __m128i packEF = _mm_packs_epi32(cmpE, cmpF);

		const __m128i pack89AB = _mm_packs_epi16(pack89, packAB);
		const __m128i packCDEF = _mm_packs_epi16(packCD, packEF);

		const uint32_t res01234567 = (uint32_t)_mm_movemask_epi8(pack0123) | ((uint32_t)_mm_movemask_epi8(pack4567) << 16);
		const uint32_t res89ABCDEF = (uint32_t)_mm_movemask_epi8(pack89AB) | ((uint32_t)_mm_movemask_epi8(packCDEF) << 16);

		res = res01234567 | ((uint64_t)res89ABCDEF << 32);
		if (res > 0) {
			break;
		}		
	}

	if (res > 0)
	{
		DWORD bitIndex = 0;
		if (BitScanReverse64(&bitIndex, res))
		{
			findIndex = i + bitIndex;
			assert(findIndex >= 0 && findIndex < (int32_t)itemsCount);
			assert(items[findIndex] == item);
			return findIndex;
		}
	}

	return -1;
}
