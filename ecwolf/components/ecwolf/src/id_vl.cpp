// ID_VL.C

#include <string.h>
#include "c_cvars.h"
#include "wl_def.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"
#include "r_2d/r_main.h"
#include "r_data/colormaps.h"
#include "v_font.h"
#include "v_video.h"
#include "v_palette.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_main.h"
#include "wl_play.h"
#include <rg_system.h>


// Uncomment the following line, if you get destination out of bounds
// assertion errors and want to ignore them during debugging
//#define IGNORE_BAD_DEST

#ifdef IGNORE_BAD_DEST
#undef assert
#define assert(x) if(!(x)) return
#define assert_ret(x) if(!(x)) return 0
#else
#define assert_ret(x) assert(x)
#endif

bool fullscreen = true;
bool usedoublebuffering = true;

unsigned screenWidth = 320;
unsigned screenHeight = 240;
unsigned fullScreenWidth = 320;
unsigned fullScreenHeight = 240;
unsigned windowedScreenWidth = 320;
unsigned windowedScreenHeight = 240;
unsigned screenBits = 8;
float screenGamma = 1.0f;

SDL_Surface *curSurface = NULL;
unsigned curPitch;

unsigned scaleFactorX, scaleFactorY;

bool	 screenfaded;

static struct
{
	uint8_t r,g,b;
	int amount;
} currentBlend;

//===========================================================================

void VL_ToggleFullscreen()
{
	VL_SetFullscreen(!fullscreen);
}

void VL_SetFullscreen(bool isFull)
{
	vid_fullscreen = fullscreen = isFull;

	if (fullscreen)
	{
		screenWidth = fullScreenWidth;
		screenHeight = fullScreenHeight;
	}
	else
	{
		screenWidth = windowedScreenWidth;
		screenHeight = windowedScreenHeight;
	}

	// Recalculate the aspect ratio, because this can change from fullscreen to windowed now
	r_ratio = static_cast<Aspect>(CheckRatio(screenWidth, screenHeight));
	VL_SetVGAPlaneMode();
	if(playstate)
	{
		DrawPlayScreen();
	}
	IN_AdjustMouse();
}

//===========================================================================

void VL_ReadPalette(const char* lump)
{
	InitPalette(lump);
	if(currentBlend.amount)
		V_SetBlend(currentBlend.r, currentBlend.g, currentBlend.b, currentBlend.amount);
	R_InitColormaps();
	TexMan.InvalidatePalette();
	V_RetranslateFonts();
}

/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void I_InitGraphics ();
void	VL_SetVGAPlaneMode (bool forSignon)
{
	if(!forSignon && screen)
		screen->Unlock();

	I_InitGraphics();

    if (!Video) {
        RG_LOGE("Video is NULL after I_InitGraphics!\n");
        return;
    }

	Video->SetResolution(screenWidth, screenHeight, 8);

    if (!screen) {
        RG_LOGE("screen is NULL after Video->SetResolution!\n");
        return;
    }

	screen->Lock(true);
	R_SetupBuffer ();
	screen->Unlock();

	scaleFactorX = CleanXfac;
	scaleFactorY = CleanYfac;

	pixelangle = new short[SCREENWIDTH];
	wallheight = new int[SCREENWIDTH];

	NewViewSize(viewsize);

	screen->Lock(false);
}

/*
=============================================================================

						PALETTE OPS

		To avoid snow, do a WaitVBL BEFORE calling these

=============================================================================
*/

/*
=================
=
= VL_FadeOut
=
= Fades the current palette to the given color in the given number of steps
=
=================
*/

static int fadeR = 0, fadeG = 0, fadeB = 0;
void VL_Fade (int start, int end, int red, int green, int blue, int steps)
{
	end <<= FRACBITS;
	start <<= FRACBITS;

	const fixed aStep = (end-start)/steps;

	VL_WaitVBL(1);

//
// fade through intermediate frames
//
	for (int a = start;(aStep < 0 ? a > end : a < end);a += aStep)
	{
		if(!usedoublebuffering || screenBits == 8) VL_WaitVBL(1);
		V_SetBlend(red, green, blue, a>>FRACBITS);
		VH_UpdateScreen();
	}

//
// final color
//
	V_SetBlend (red,green,blue,end>>FRACBITS);
	// Not quite sure why I need to call this twice.
	VH_UpdateScreen();
	VH_UpdateScreen();

	screenfaded = end != 0;

	// Clear out any input at this point that may be stored up. This solves
	// issues such as starting facing the wrong angle in super 3d noah's ark.
	IN_ProcessEvents();
}

void VL_FadeOut (int start, int end, int red, int green, int blue, int steps)
{
	fadeR = red;
	fadeG = green;
	fadeB = blue;
	VL_Fade(start, end, red, green, blue, steps);
}


/*
=================
=
= VL_FadeIn
=
=================
*/

void VL_FadeIn (int start, int end, int steps)
{
	if(screenfaded)
		VL_Fade(end, start, fadeR, fadeG, fadeB, steps);
}

/*
=============================================================================

							PIXEL OPS

=============================================================================
*/

byte *VL_LockSurface()
{
	screen->Lock(false);
	return (byte *) screen->GetBuffer();
}

void VL_UnlockSurface()
{
	screen->Unlock();
}
