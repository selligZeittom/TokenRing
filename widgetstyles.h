#ifndef _WIDGETSTYLES_H
#define _WIDGETSTYLES_H

#include "gfx.h"

// WidgetStyle: red_green
const GWidgetStyle red_green = {
	HTML2COLOR(0xFFFFFF),              // background
	HTML2COLOR(0x2A8FCD),              // focus

	// Enabled color set
	{
		HTML2COLOR(0x000000),         // text
		HTML2COLOR(0x404040),         // edge
		HTML2COLOR(0xFF0000),         // fill
		HTML2COLOR(0x00E000)          // progress (active area)
	},

	// Disabled color set
	{
		HTML2COLOR(0xC0C0C0),         // text
		HTML2COLOR(0x808080),         // edge
		HTML2COLOR(0xE0E0E0),         // fill
		HTML2COLOR(0xC0E0C0)          // progress (active area)
	},

	// Pressed color set
	{
		HTML2COLOR(0x404040),         // text
		HTML2COLOR(0x404040),         // edge
		HTML2COLOR(0x00FF00),         // fill
		HTML2COLOR(0x00E000)          // progress (active area)
	}
};

// WidgetStyle: white_on_gray
const GWidgetStyle white_on_gray = {
	HTML2COLOR(0x606060),              // background
	HTML2COLOR(0x2A8FCD),              // focus

	// Enabled color set
	{
		HTML2COLOR(0xFFFFFF),         // text
		HTML2COLOR(0x404040),         // edge
		HTML2COLOR(0xE0E0E0),         // fill
		HTML2COLOR(0x00E000)          // progress (active area)
	},

	// Disabled color set
	{
		HTML2COLOR(0xC0C0C0),         // text
		HTML2COLOR(0x808080),         // edge
		HTML2COLOR(0xE0E0E0),         // fill
		HTML2COLOR(0xC0E0C0)          // progress (active area)
	},

	// Pressed color set
	{
		HTML2COLOR(0x00FF00),         // text
		HTML2COLOR(0x404040),         // edge
		HTML2COLOR(0x00FF00),         // fill
		HTML2COLOR(0x00E000)          // progress (active area)
	}
};

#endif // _WIDGETSTYLES_H
