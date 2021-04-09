#include "pch.h"
#include "Metrics.h"

using namespace d2dx;
using namespace DirectX;

struct MetricsTableEntry
{
	Size desktopSize;
	Size gameSize;
	Size gameSizeWide;
};

static const struct MetricsTableEntry metricsTable[] =
{
	{
		.desktopSize { 1280, 720 },
		.gameSize { 960, 720 },
		.gameSizeWide { 1280, 720 },
	},
	{
		.desktopSize { 1360, 768 },
		.gameSize { 1024, 768 },
		.gameSizeWide { 1360, 768 },
	},
	{
		.desktopSize { 1366, 768 },
		.gameSize { 1024, 768 },
		.gameSizeWide { 1366, 768 },
	},
	{
		.desktopSize { 1280, 800 },
		.gameSize { 1066, 800 },
		.gameSizeWide { 1280, 800 },
	},
	{
		.desktopSize { 1536, 864 },
		.gameSize { 1152, 864 },
		.gameSizeWide { 1536, 864 },
	},
	{
		.desktopSize { 1440, 900 },
		.gameSize { 1200, 900 },
		.gameSizeWide { 1440, 900 },
	},
	{
		.desktopSize { 1600, 900 },
		.gameSize { 1200, 900 },
		.gameSizeWide { 1600, 900 },
	},
	{
		.desktopSize { 1280, 1024 },
		.gameSize { 640, 480 },
		.gameSizeWide { 640, 512 },
	},
	{
		.desktopSize { 1400, 1050 },
		.gameSize { 700, 525 },
		.gameSizeWide { 700, 525 },
	},
	{
		.desktopSize { 1680, 1050 },
		.gameSize { 700, 525 },
		.gameSizeWide { 840, 525 },
	},
	{
		.desktopSize { 1920, 1080 },
		.gameSize { 720, 540 },
		.gameSizeWide { 960, 540 },
	},
	{
		.desktopSize { 2560, 1080 },
		.gameSize { 720, 540 },
		.gameSizeWide { 960, 540 }, /* note: doesn't cover entire monitor */
	},
	{
		.desktopSize { 2048, 1152 },
		.gameSize { 768, 576 },
		.gameSizeWide { 1024, 576 },
	},
	{
		.desktopSize { 1600, 1200 },
		.gameSize { 800, 600 },
		.gameSizeWide { 800, 600 },
	},
	{
		.desktopSize { 1920, 1200 },
		.gameSize { 800, 600 },
		.gameSizeWide { 960, 600 },
	},
	{
		.desktopSize { 2560, 1440 },
		.gameSize { 960, 720 },
		.gameSizeWide { 1280, 720 },
	},
	{
		.desktopSize { 3440, 1440 },
		.gameSize { 960, 720 },
		.gameSizeWide { 1280, 720 }, /* note: doesn't cover entire monitor */
	},
	{
		.desktopSize { 2048, 1536 },
		.gameSize { 1024, 768 },
		.gameSizeWide { 1024, 768 },
	},
	{
		.desktopSize { 2560, 1600 },
		.gameSize { 710, 532 },
		.gameSizeWide { 852, 532 },
	},
	{
		.desktopSize { 2560, 2048 },
		.gameSize { 852, 640 },
		.gameSizeWide { 852, 682 },
	},
	{
		.desktopSize { 3200, 2048 },
		.gameSize { 852, 640 },
		.gameSizeWide { 1066, 682 },
	},
	{
		.desktopSize { 3840, 2160 },
		.gameSize { 960, 720 },
		.gameSizeWide { 1280, 720 },
	},
	{
		.desktopSize { 4096, 2160 },
		.gameSize { 960, 720 },
		.gameSizeWide { 1364, 720 },
	},
	{
		.desktopSize { 4320, 2160 },
		.gameSize { 960, 720 },
		.gameSizeWide { 1440, 720 },
	},
	{
		.desktopSize { 3200, 2400 },
		.gameSize { 800, 600 },
		.gameSizeWide { 800, 600 },
	},
	{
		.desktopSize { 3840, 2400 },
		.gameSize { 800, 600 },
		.gameSizeWide { 960, 600 },
	},
};

Size d2dx::Metrics::GetSuggestedGameSize(Size desktopSize, bool wide)
{
	for (int32_t i = 0; i < ARRAYSIZE(metricsTable); ++i)
	{
		if (metricsTable[i].desktopSize.width == desktopSize.width &&
			metricsTable[i].desktopSize.height == desktopSize.height)
		{
			return wide ? metricsTable[i].gameSizeWide : metricsTable[i].gameSize;
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

	Rect rect
	{
		(desktopSize.width - gameSize.width * scaleFactor) / 2,
		(desktopSize.height - gameSize.height * scaleFactor) / 2,
		gameSize.width * scaleFactor,
		gameSize.height * scaleFactor
	};

	/* Allow for a small amount of black margin on all sides. When more than that,
	   rescale the image with a non-integer factor. */
	if (rect.offset.x >= 16 && rect.offset.y >= 16)
	{
		if (rect.offset.x < rect.offset.y)
		{
			float scaleFactorF = (float)desktopSize.width / rect.size.width;
			int32_t scaledHeight = (int32_t)(rect.size.height * scaleFactorF);
			rect.offset.x = 0;
			rect.offset.y = (desktopSize.height - scaledHeight) / 2;
			rect.size.width = desktopSize.width;
			rect.size.height = scaledHeight;
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
		rect.offset.x >= 0 &&
		rect.offset.y >= 0 &&
		rect.size.width > 0 &&
		rect.size.height > 0 &&
		(rect.offset.x + rect.size.width) <= desktopSize.width &&
		(rect.offset.y + rect.size.height) <= desktopSize.height);

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
