#ifndef _RENDERING_FUNCTIONS_H
#define _RENDERING_FUNCTIONS_H

#include "gfx.h"
#include "resources_manager.h"

// background_Display Background
void background_Display_Background(GWidgetObject* wo, void* param)
{
	(void)param;

	gdispGFillArea(wo->g.display, wo->g.x, wo->g.y, wo->g.width, wo->g.height, White);
	gdispGFillArea(wo->g.display, 0, 0, 480, 272, HTML2COLOR(0xFFFFFF));
}

// background_gray
void background_gray(GWidgetObject* wo, void* param)
{
	(void)param;

	gdispGFillArea(wo->g.display, wo->g.x, wo->g.y, wo->g.width, wo->g.height, White);
	gdispGFillArea(wo->g.display, 0, 0, 480, 272, HTML2COLOR(0x606060));
}

#endif // _RENDERING_FUNCTIONS_H
