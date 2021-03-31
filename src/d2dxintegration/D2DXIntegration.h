#pragma once
#ifndef D2DX_INTEGRATION_H
#define D2DX_INTEGRATION_H

#include <stdint.h>

namespace d2dx
{
	bool IsD2DXLoaded();
	void SetCustomResolution(int32_t width, int32_t height);
	void GetSuggestedCustomResolution(int32_t* width, int32_t* height);
}

#endif
