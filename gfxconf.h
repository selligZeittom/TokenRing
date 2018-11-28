#ifndef _GFXCONF_H
#define _GFXCONF_H


///////////////////////////////////////////////////////////////////////////
// GOS                                                                   //
///////////////////////////////////////////////////////////////////////////
#define GFX_USE_OS_UNKNOWN TRUE
#define GFX_OS_HEAP_SIZE 0

#define GFX_COMPILER GFX_COMPILER_KEIL
#define GFX_CPU GFX_CPU_CORTEX_M7_FP
#define GFX_CPU_ENDIAN GFX_CPU_ENDIAN_UNKNOWN


///////////////////////////////////////////////////////////////////////////
// GDISP                                                                 //
///////////////////////////////////////////////////////////////////////////
#define GFX_USE_GDISP TRUE

#define GDISP_NEED_MULTITHREAD TRUE
#define GDISP_NEED_CONTROL TRUE
#define GDISP_NEED_STARTUP_LOGO FALSE
#define GDISP_STARTUP_COLOR HTML2COLOR(0x000000)
#define GDISP_DEFAULT_ORIENTATION GDISP_ROTATE_0

#define GDISP_NEED_TEXT TRUE
#define GDISP_NEED_TEXT_WORDWRAP TRUE
	#define GDISP_INCLUDE_USER_FONTS TRUE

#define GDISP_NEED_IMAGE TRUE
	#define GDISP_NEED_IMAGE_BMP TRUE


///////////////////////////////////////////////////////////////////////////
// GWIN                                                                  //
///////////////////////////////////////////////////////////////////////////
#define GFX_USE_GWIN TRUE

#define GWIN_NEED_WINDOWMANAGER TRUE

#define GWIN_NEED_WIDGET TRUE
	#define GWIN_NEED_CHECKBOX TRUE
	#define GWIN_NEED_IMAGE TRUE
	#define GWIN_NEED_LABEL TRUE
	#define GWIN_NEED_BUTTON TRUE
	#define GWIN_WIDGET_TAGS TRUE

#define GWIN_NEED_CONTAINERS TRUE
	#define GWIN_NEED_CONTAINER TRUE


///////////////////////////////////////////////////////////////////////////
// GEVENT                                                                //
///////////////////////////////////////////////////////////////////////////
#define GFX_USE_GEVENT TRUE


///////////////////////////////////////////////////////////////////////////
// GTIMER                                                                //
///////////////////////////////////////////////////////////////////////////
#define GFX_USE_GTIMER TRUE


///////////////////////////////////////////////////////////////////////////
// GQUEUE                                                                //
///////////////////////////////////////////////////////////////////////////
#define GFX_USE_GQUEUE TRUE

#define GQUEUE_NEED_ASYNC TRUE


///////////////////////////////////////////////////////////////////////////
// GINPUT                                                                //
///////////////////////////////////////////////////////////////////////////
#define GFX_USE_GINPUT TRUE

#define GINPUT_NEED_MOUSE TRUE


///////////////////////////////////////////////////////////////////////////
// GFILE                                                                 //
///////////////////////////////////////////////////////////////////////////
#define GFX_USE_GFILE TRUE

#define GFILE_NEED_ROMFS TRUE
#define GFILE_MAX_GFILES 50

#endif // _GFXCONF_H
