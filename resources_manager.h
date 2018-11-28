#ifndef _RESOURCES_MANAGER_H
#define _RESOURCES_MANAGER_H

#include "gfx.h"

// Image indexes
#define token 0
#define token_w 1

// Font indexes
#define arial_12 0
#define arial__14 1

#ifdef __cplusplus
extern "C" {
#endif

	bool_t guiResourcesManagerInit(void);
	gdispImage* gstudioGetImage(int imageIndex);
	const char* gstudioGetImageFilePath(int imageIndex);
	font_t gstudioGetFont(int fontIndex);

#ifdef __cplusplus
}
#endif

#endif // _RESOURCES_MANAGER_H
