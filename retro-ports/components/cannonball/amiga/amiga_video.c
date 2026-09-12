#include <stdint.h>
#include <exec/exec.h>
#include <dos/dos.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <libraries/lowlevel.h>
#include <devices/gameport.h>
#include <devices/timer.h>
#include <devices/keymap.h>
#include <devices/input.h>


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
 
 
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <math.h>
#include "video.h"



#define REG(xn, parm) parm __asm(#xn)
#define REGARGS __regargs
#define STDARGS __stdargs
#define SAVEDS __saveds
#define ALIGNED __attribute__ ((aligned(4))
#define FAR
#define CHIP
#define INLINE __inline__

#define PLOG(fmt, args...) do {   fprintf(stdout, fmt, ## args); } while (0)

extern void REGARGS c2p1x1_8_c5_bm_040(
REG(d0, UWORD chunky_x),
REG(d1, UWORD chunky_y),
REG(d2, UWORD offset_x),
REG(d3, UWORD offset_y),
REG(a0, UBYTE *chunky_buffer),
REG(a1, struct BitMap *bitmap));

/*
 * cachemodes
 */

#define CM_IMPRECISE ((1<<6)|(1<<5))
#define CM_PRECISE   (1<<6)
#define CM_COPYBACK  (1<<5)
#define CM_WRITETHROUGH 0
#define CM_MASK      ((1<<6)|(1<<5))


/*
 * functions
 */

extern UBYTE REGARGS mmu_mark (REG(a0, void *start),
                               REG(d0, ULONG length),
                               REG(d1, ULONG cm),
                               REG(a6, struct ExecBase *SysBase));


extern int isRTG;
extern int cpu_type;
extern void mmu_stuff2(void);
extern void mmu_stuff2_cleanup(void);
 

int mmu_chunky = 0;
int mmu_active = 0;

int opengl = 0;
static int width=0, height=0, bpp=0, mode=0;

static  ULONG colorsAGA[770];

/** Hardware window */
struct Window *_hardwareWindow;
/** Hardware screen */
struct Screen *_hardwareScreen;
// Hardware double buffering.
struct ScreenBuffer *_hardwareScreenBuffer[2];
BYTE _currentScreenBuffer;

static unsigned masks[4][4] = {{0x00000000,0x00000000,0x00000000,0x00000000},
							   {0x0000001F,0x000007E0,0x0000F800,0x00000000},
							   {0x000000FF,0x0000FF00,0x00FF0000,0x00000000},
							   {0x000000FF,0x0000FF00,0x00FF0000,0x00000000}};

enum videoMode
{
    VideoModeAGA,
    VideoModeRTG
};


enum videoMode vidMode = VideoModeAGA;

// RTG Stuff

struct Library *CyberGfxBase = NULL;
static APTR video_bitmap_handle = NULL;

static UWORD emptypointer[] = {
  0x0000, 0x0000,    /* reserved, must be NULL */
  0x0000, 0x0000,     /* 1 row of image data */
  0x0000, 0x0000    /* reserved, must be NULL */
};


void video_end(void) {
    
  if (_hardwareWindow) {
        ClearPointer(_hardwareWindow);
        CloseWindow(_hardwareWindow);
        _hardwareWindow = NULL;
    }
 
    if (_hardwareScreenBuffer[0]) { 
        WaitBlit();
        FreeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[0]);
    }

    if (_hardwareScreenBuffer[1]) { 
        WaitBlit();
        FreeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[1]);
    }

    if (_hardwareScreen) { 
        CloseScreen(_hardwareScreen);
        _hardwareScreen = NULL;
    }    	
    
    if (CyberGfxBase)
    {
        CloseLibrary(CyberGfxBase);
        CyberGfxBase = NULL;
    }
    
}

void start_rtg()
{
    ULONG modeId = INVALID_ID;

    if(!CyberGfxBase) CyberGfxBase = (struct Library *) OpenLibrary((UBYTE *)"cybergraphics.library",41L);
 
    
    modeId = BestCModeIDTags(CYBRBIDTG_NominalWidth, 320,
				      CYBRBIDTG_NominalHeight, 240,
				      CYBRBIDTG_Depth,8,
				      TAG_DONE );    
				      
				      
    _hardwareScreen = OpenScreenTags(NULL,
                        SA_Depth, 8,
                        SA_DisplayID, modeId,
                        SA_Width, 320,
                        SA_Height,240,
                        SA_Type, CUSTOMSCREEN,
                        SA_Overscan, OSCAN_TEXT,
                        SA_Quiet,1,
                        SA_ShowTitle, 0,
                        SA_Draggable, 0,
                        SA_Exclusive, 1,
                        SA_AutoScroll, 0,                     
                        TAG_END);
    
    
    _hardwareWindow = OpenWindowTags(NULL,
                  	    WA_Left, 0,
            			WA_Top, 0,
            			WA_Width, 320,
            			WA_Height, 240,
            			WA_Title, NULL,
    					SA_AutoScroll, 0,
            			WA_CustomScreen, (ULONG)_hardwareScreen,
            			WA_Backdrop, 1,
            			WA_Borderless, 1,
            			WA_DragBar, 0,
            			WA_Activate, 1,
            			WA_SimpleRefresh, 1,
            			WA_NoCareRefresh, 1, 
                        WA_IDCMP,           IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE,   
                  	    TAG_END);
    
 
    SetPointer (_hardwareWindow, emptypointer, 0, 0, 0, 0);				      
    
}

void start_aga()
{
    ULONG modeId = INVALID_ID;
    
    modeId = BestModeID(BIDTAG_NominalWidth, 320,
                        BIDTAG_NominalHeight, 256,
            	        BIDTAG_DesiredWidth, 320,
            	        BIDTAG_DesiredHeight, 256,
            	        BIDTAG_Depth, 8,
            	        BIDTAG_MonitorID, PAL_MONITOR_ID,
            	        TAG_END);
    
    
    _hardwareScreen = OpenScreenTags(NULL,
                     SA_Depth, 8,
                     SA_DisplayID, modeId,
                     SA_Width, 320,
                     SA_Height,256,
                     SA_Type, CUSTOMSCREEN,
                     SA_Overscan, OSCAN_TEXT,
                     SA_Quiet,1,
                     SA_ShowTitle, 0,
                     SA_Draggable, 0,
                     SA_Exclusive, 1,
                     SA_AutoScroll, 0,
                     TAG_END);
    
    
    // Create the hardware screen.
    
    
    _hardwareScreenBuffer[0] = AllocScreenBuffer(_hardwareScreen, NULL, SB_SCREEN_BITMAP);
    _hardwareScreenBuffer[1] = AllocScreenBuffer(_hardwareScreen, NULL, 0);
    
    _currentScreenBuffer = 1;
    
    _hardwareWindow = OpenWindowTags(NULL,
                  	    WA_Left, 0,
            			WA_Top, 0,
            			WA_Width, 320,
            			WA_Height, 256,
            			WA_Title, NULL,
    					SA_AutoScroll, 0,
            			WA_CustomScreen, (ULONG)_hardwareScreen,
            			WA_Backdrop, 1,
            			WA_Borderless, 1,
            			WA_DragBar, 0,
            			WA_Activate, 1,
            			WA_SimpleRefresh, 1,
            			WA_NoCareRefresh, 1,
            			WA_ReportMouse, 1,
            			WA_RMBTrap, 1,
                  	    WA_IDCMP,  IDCMP_RAWKEY | IDCMP_MOUSEMOVE | IDCMP_DELTAMOVE | IDCMP_MOUSEBUTTONS,
                  	    TAG_END);
    
    SetPointer (_hardwareWindow, emptypointer, 1, 16, 0, 0);
}




int video_copy_screen() {

    UBYTE *base_address;
  
    if (vidMode == VideoModeAGA)
    {    
        //c2p1x1_8_c5_bm_040(320,240,0,0,src->data,_hardwareScreenBuffer[_currentScreenBuffer]->sb_BitMap);
        //ChangeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[_currentScreenBuffer]); 
        //_currentScreenBuffer = _currentScreenBuffer ^ 1;	 
    }
    else
    {
//        video_bitmap_handle = LockBitMapTags (_hardwareScreen->ViewPort.RasInfo->BitMap,
//                                              LBMI_BASEADDRESS, &base_address,
//                                              TAG_DONE);
        if (video_bitmap_handle) {
//            CopyMem (src->data, base_address, 320 * 240);
 //           UnLockBitMap (video_bitmap_handle);
//            video_bitmap_handle = NULL;        
        } 
        
    }

	return 1;
}

void video_clearscreen() {

}


void video_fullscreen_flip() {
 
}


void video_stretch(int enable) {
 
}


void vga_setpalette(unsigned char *palette) {
    int i; 
    int j = 1;  
     
    if(bpp>1) return;

    for (i=0; i<256; ++i) 
    {
        colorsAGA[j]   = palette[0] << 24;
        colorsAGA[j+1] = palette[1] << 24;
        colorsAGA[j+2] = palette[2] << 24;
        
        palette+=3;
        j+=3;
    } 
    
    colorsAGA[0]=((256)<<16) ;
    colorsAGA[((256*3)+1)]=0x00000000;
    LoadRGB32(&_hardwareScreen->ViewPort, &colorsAGA); 
}

// TODO: give this function a boolean (int) return type denoting success/failure
void vga_set_color_correction(int gm, int br) {
 
}
