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
#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN
#define __MSC__

#include <algorithm>
#include <array>
#include <stdexcept>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <filesystem>
#include <memory>
#include <system_error>
#include <type_traits>
#include <vector>

#include <windows.h>
#include <windowsx.h>
#include <lm.h>
#include <CommCtrl.h>
#include <combaseapi.h>
#include <wrl/implements.h>
#include <wrl.h>
#include <comdef.h>
#include <emmintrin.h>
#include <string.h>

#include "../../thirdparty/fnv/fnv.h"
#include "../../thirdparty/detours/detours.h"
#include <glide.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_2.h>
#include <d3d11_3.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

using EventHandle = Microsoft::WRL::Wrappers::HandleT<Microsoft::WRL::Wrappers::HandleTraits::EventTraits>;

#endif //PCH_H
