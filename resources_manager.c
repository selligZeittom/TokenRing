#include "resources_manager.h"

typedef struct ImageInfo {
	gdispImage* pointer;
	const char* filePath;
} ImageInfo;

gdispImage imgtoken;
gdispImage imgtoken_w;

static ImageInfo _imagesArray[2];
static font_t _fontsArray[2];

bool_t guiResourcesManagerInit(void)
{
	size_t i;

	// Fill images array
	_imagesArray[0].pointer = &imgtoken;
	_imagesArray[0].filePath = "token.bmp";
	_imagesArray[1].pointer = &imgtoken_w;
	_imagesArray[1].filePath = "token_w.bmp";

	// Open images
	for (i = 0; i < 2; i++) {
		gdispImageOpenFile(gstudioGetImage(i), gstudioGetImageFilePath(i));
	}

	// Cache images

	// Open fonts
	_fontsArray[0] = gdispOpenFont("arial_12_arial12_aa");
	_fontsArray[1] = gdispOpenFont("arial__14_arial14_aa");

	return TRUE;
}

GFXINLINE gdispImage* gstudioGetImage(int imageIndex)
{
	return _imagesArray[imageIndex].pointer;
}

GFXINLINE const char* gstudioGetImageFilePath(int imageIndex)
{
	return _imagesArray[imageIndex].filePath;
}

GFXINLINE font_t gstudioGetFont(int fontIndex)
{
	return _fontsArray[fontIndex];
}

