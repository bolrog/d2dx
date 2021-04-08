#pragma once

#include "Buffer.h"
#include "Types.h"

namespace d2dx
{
	namespace Metrics
	{
		Size GetSuggestedGameSize(Size desktopSize, bool wide);
		Rect GetRenderRect(Size gameSize, Size desktopSize, bool wide);
		Buffer<Size> GetStandardDesktopSizes();
	}
}
