#include "pch.h"
#include "Metrics.h"

using namespace d2dx;
using namespace DirectX;

struct MetricsTableEntry
{
	Size desktopSize;
	Rect recGameRect;
	Rect recGameRectWide;
};

static const struct MetricsTableEntry metricsTable[] =
{
	{
		.desktopSize { 1280, 720 },
		.recGameRect { 160, 0, 960, 720 },
		.recGameRectWide { 0, 0, 1280, 720 },
	},
	{
		.desktopSize { 1280, 800 },
		.recGameRect { 107, 0, 1066, 800 },
		.recGameRectWide { 0, 0, 1280, 800 },
	},
	{
		.desktopSize { 1360, 768 },
		.recGameRect { 168, 0, 1024, 768 },
		.recGameRectWide { 0, 0, 1360, 768 },
	},
	{
		.desktopSize { 1366, 768 },
		.recGameRect { 171, 0, 1024, 768 },
		.recGameRectWide { 0, 0, 1366, 768 },
	},
	{
		.desktopSize { 1536, 864 },
		.recGameRect { 192, 0, 1152, 864 },
		.recGameRectWide { 0, 0, 1536, 864 },
	},
	{
		.desktopSize { 1440, 900 },
		.recGameRect { 120, 0, 1200, 900 },
		.recGameRectWide { 0, 0, 1440, 900 },
	},
	{
		.desktopSize { 1600, 900 },
		.recGameRect { 200, 0, 1200, 900 },
		.recGameRectWide { 0, 0, 1600, 900 },
	},
	{
		.desktopSize { 1280, 1024 },
		.recGameRect { 0, 32, 640, 480 },
		.recGameRectWide { 0, 0, 640, 512 },
	},
	{
		.desktopSize { 1400, 1050 },
		.recGameRect { 0, 0, 700, 525 },
		.recGameRectWide { 0, 0, 700, 525 },
	},
	{
		.desktopSize { 1680, 1050 },
		.recGameRect { 140, 0, 700, 525 },
		.recGameRectWide { 0, 0, 840, 525 },
	},
	{
		.desktopSize { 1920, 1080 },
		.recGameRect { 240, 0, 720, 540 },
		.recGameRectWide { 0, 0, 960, 540 },
	},
	{
		.desktopSize { 2560, 1080 },
		.recGameRect { 560, 0, 720, 540 },
		.recGameRectWide { 320, 0, 960, 540 }, /* note: doesn't cover entire monitor */
	},
	{
		.desktopSize { 2048, 1152 },
		.recGameRect { 256, 0, 768, 576 },
		.recGameRectWide { 0, 0, 1024, 576 },
	},
	{
		.desktopSize { 1600, 1200 },
		.recGameRect { 0, 0, 800, 600 },
		.recGameRectWide { 0, 0, 800, 600 },
	},
	{
		.desktopSize { 1920, 1200 },
		.recGameRect { 160, 0, 800, 600 },
		.recGameRectWide { 0, 0, 960, 600 },
	},
	{
		.desktopSize { 2560, 1440 },
		.recGameRect { 320, 0, 960, 720 },
		.recGameRectWide { 0, 0, 1280, 720 },
	},
	{
		.desktopSize { 3440, 1440 },
		.recGameRect { 760, 0, 960, 720 },
		.recGameRectWide { 440, 0, 1280, 720 }, /* note: doesn't cover entire monitor */
	},
	{
		.desktopSize { 3840, 2160 },
		.recGameRect { 480, 0, 960, 720 },
		.recGameRectWide { 0, 0, 1280, 720 },
	},
	{
		.desktopSize { 4096, 2160 },
		.recGameRect { 608, 0, 960, 720 },
		.recGameRectWide { 2, 0, 1364, 720 },
	},
};

Size d2dx::Metrics::GetSuggestedGameSize(Size desktopSize, bool wide)
{
	for (int32_t i = 0; i < ARRAYSIZE(metricsTable); ++i)
	{
		if (metricsTable[i].desktopSize.width == desktopSize.width &&
			metricsTable[i].desktopSize.height == desktopSize.height)
		{
			return wide ? metricsTable[i].recGameRectWide.size : metricsTable[i].recGameRect.size;
		}
	}

	Size size = { 0, 0 };
	for (int32_t scaleFactor = 1; scaleFactor <= 8; ++scaleFactor)
	{
		size.width = desktopSize.width / scaleFactor;
		size.height = desktopSize.height / scaleFactor;
		if ((desktopSize.height / (scaleFactor + 1)) <= 480)
		{
			break;
		}
	}
	return size;
}

Rect d2dx::Metrics::GetRenderRect(Size gameSize, Size desktopSize, bool wide)
{
	int32_t scaleFactor = 1;

	while (
		gameSize.width * (scaleFactor + 1) <= desktopSize.width &&
		gameSize.height * (scaleFactor + 1) <= desktopSize.height)
	{
		++scaleFactor;
	}

	Rect rect{ 0, 0, gameSize.width * scaleFactor, gameSize.height * scaleFactor };
	rect.offset.x = (desktopSize.width - rect.size.width) / 2;
	rect.offset.y = (desktopSize.height - rect.size.height) / 2;

	if (rect.offset.x > 0 && rect.offset.y > 0)
	{
		if (rect.offset.x < rect.offset.y)
		{
			float scaleFactorF = (float)desktopSize.width / rect.size.width;
			rect.offset.x = 0;
			rect.offset.y = 0;
			rect.size.width = desktopSize.width;
			rect.size.height *= scaleFactorF;
		}
		else
		{
			float scaleFactorF = (float)desktopSize.height / rect.size.height;
			int32_t scaledWidth = (int32_t)(rect.size.width * scaleFactorF);
			rect.offset.x = (desktopSize.width - scaledWidth) / 2;
			rect.offset.y = 0;
			rect.size.width = scaledWidth;
			rect.size.height = desktopSize.height;
		}
	}

	assert(
		rect.size.width > 0 && 
		rect.size.height > 0 &&
		rect.size.width <= desktopSize.width &&
		rect.size.height <= desktopSize.height);
	
	return rect;
}

Buffer<Size> d2dx::Metrics::GetStandardDesktopSizes()
{
	Buffer<Size> standardDesktopSizes(ARRAYSIZE(metricsTable));

	for (int32_t i = 0; i < ARRAYSIZE(metricsTable); ++i)
	{
		standardDesktopSizes.items[i] = metricsTable[i].desktopSize;
	}

	return standardDesktopSizes;
}
