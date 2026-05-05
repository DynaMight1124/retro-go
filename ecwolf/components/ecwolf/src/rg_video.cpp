#include "thingdef.h"
#include "v_video.h"
#include "v_palette.h"
#include "m_classes.h"
#include "r_2d/r_draw.h"
#include "r_2d/r_things.h"
#include "wl_draw.h"
#include <rg_system.h>
#include <rg_display.h>
#include <rg_surface.h>
#include <rg_utils.h>
#include <rg_gui.h>

#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#include <esp_system.h>
#endif

extern DWORD (*Col2RGB8)[256];
extern DWORD (*Col2RGB8_Inverse)[256];
extern BYTE (*RGB32k)[32][32];

extern "C" {
int *ylookup = NULL;
short *spanend = NULL;
BYTE *dc_tempbuff = NULL;
unsigned int (*dc_tspans)[MAXHEIGHT] = NULL;
short *zeroarray = NULL;
short *bottomclipper = NULL;
short *topclipper = NULL;
short *mfloorclip = NULL;
short *mceilingclip = NULL;
}

// Global math tables from wl_draw.cpp
fixed *finetangent = NULL;
fixed *finesine = NULL;
fixed *finecosine = NULL;

extern DFrameBuffer *screen;
IVideo *Video = NULL;

class RGFB : public DFrameBuffer
{
    DECLARE_CLASS(RGFB, DFrameBuffer)
public:
    RGFB(int width, int height);
    virtual ~RGFB();

    bool Lock(bool buffer);
    void Unlock();
    void Update();
    PalEntry *GetPalette();
    void GetFlashedPalette(PalEntry pal[256]);
    void UpdatePalette();
    bool SetGamma(float gamma);
    bool SetFlash(PalEntry rgb, int amount);
    void GetFlash(PalEntry &rgb, int &amount);
    int GetPageCount();
    bool IsFullscreen();

    void PaletteChanged() { NeedPalUpdate = true; }
    int QueryNewPalette() { return 0; }
    bool Is8BitMode() { return true; }

private:
    PalEntry SourcePalette[256];
    uint16_t cached_pal[256];
    PalEntry Flash;
    int FlashAmount;
    float Gamma;
    rg_surface_t *rg_surface;
    bool NeedPalUpdate;
};

IMPLEMENT_INTERNAL_CLASS(RGFB)

static int frame_count = 0;
static int64_t last_fps_time = 0;

RGFB::RGFB(int width, int height) : DFrameBuffer(width, height)
{
    Width = width;
    Height = height;
    Pitch = width; 
    Buffer = MemBuffer;
    rg_surface = rg_surface_create(width, height, RG_PIXEL_565_LE, MEM_SLOW);
    Gamma = 1.0f;
    FlashAmount = 0;
    Flash.d = 0;
    NeedPalUpdate = true;
    memset(cached_pal, 0, sizeof(cached_pal));
    memcpy(SourcePalette, GPalette.BaseColors, 256 * sizeof(PalEntry));
}

RGFB::~RGFB()
{
    if (rg_surface) rg_surface_free(rg_surface);
}

bool RGFB::Lock(bool buffer)
{
    return DSimpleCanvas::Lock(buffer);
}

void RGFB::Unlock()
{
    DSimpleCanvas::Unlock();
}

#define EC_RGB565(r, g, b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

void RGFB::Update()
{
    if (NeedPalUpdate)
    {
        UpdatePalette();
    }

    uint16_t *dest = (uint16_t *)rg_surface->data;
    uint8_t *src_row = MemBuffer;

    for (int y = 0; y < Height; y++) {
        uint8_t *src = src_row;
        for (int x = 0; x < Width; x++) {
            *dest++ = cached_pal[*src++];
        }
        src_row += Pitch;
    }

    int timeout = 100; // 100ms timeout
    while (rg_display_is_busy() && timeout-- > 0) {
        rg_task_delay(1);
        rg_system_tick(0);
    }

    rg_display_submit(rg_surface, 0);
    rg_system_tick(0);

    frame_count++;
    int64_t now = rg_system_timer();
    if (now - last_fps_time >= 5000000) { 
#ifdef ESP_PLATFORM
        float fps = (float)frame_count * 1000000.0f / (now - last_fps_time);
        RG_LOGW("FPS: %.2f, Busy: %d, View: %dx%d, Free PSRAM: %d KB\n", 
               fps, rg_display_is_busy(), Width, Height, 
               (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
#endif
        frame_count = 0;
        last_fps_time = now;
    }
}

PalEntry *RGFB::GetPalette() { return SourcePalette; }
void RGFB::GetFlashedPalette(PalEntry pal[256]) { memcpy(pal, SourcePalette, 256 * sizeof(PalEntry)); }

void RGFB::UpdatePalette()
{
    if (FlashAmount < 0) FlashAmount = 0;
    if (FlashAmount > 256) FlashAmount = 256;

    for (int i = 0; i < 256; i++) {
        PalEntry p = SourcePalette[i]; // Use SourcePalette updated by InitPalette
        if (FlashAmount > 0) {
            p.r = (p.r * (256 - FlashAmount) + Flash.r * FlashAmount) >> 8;
            p.g = (p.g * (256 - FlashAmount) + Flash.g * FlashAmount) >> 8;
            p.b = (p.b * (256 - FlashAmount) + Flash.b * FlashAmount) >> 8;
        }
        cached_pal[i] = EC_RGB565(p.r, p.g, p.b);
    }
    NeedPalUpdate = false;
}

bool RGFB::SetGamma(float gamma) { Gamma = gamma; NeedPalUpdate = true; return true; }
bool RGFB::SetFlash(PalEntry rgb, int amount) { 
    Flash = rgb; 
    FlashAmount = amount; 
    if (FlashAmount < 0) FlashAmount = 0;
    if (FlashAmount > 256) FlashAmount = 256;
    NeedPalUpdate = true; 
    return true; 
}
void RGFB::GetFlash(PalEntry &rgb, int &amount) { rgb = Flash; amount = FlashAmount; }
int RGFB::GetPageCount() { return 1; }
bool RGFB::IsFullscreen() { return true; }

class RGVideo : public IVideo
{
public:
    RGVideo() {}
    virtual ~RGVideo() {}

    EDisplayType GetDisplayType() { return DISPLAY_FullscreenOnly; }
    void SetWindowedScale(float scale) {}

    DFrameBuffer *CreateFrameBuffer(int width, int height, bool fs, DFrameBuffer *old)
    {
        if (old) delete old;
        return new RGFB(width, height);
    }

    void StartModeIterator (int bits, bool fs) { m_Iterator = 0; }
    bool NextMode(int *width, int *height, bool *letterbox)
    {
        static const int res[][2] = {
            {280, 200},
            {320, 200},
            {320, 240},
            {640, 480}
        };
        if (m_Iterator >= (int)(sizeof(res)/sizeof(res[0]))) return false;
        *width = res[m_Iterator][0];
        *height = res[m_Iterator][1];
        if (letterbox) *letterbox = false;
        m_Iterator++;
        return true;
    }

    bool SetResolution(int width, int height, int bits)
    {
        if (screen)
        {
            screen->ObjectFlags |= OF_YesReallyDelete;
            delete screen;
        }

        screen = CreateFrameBuffer(width, height, true, NULL);
        GC::WriteBarrier(screen);

        // Initialize renderer row offsets and pitch
        extern void R_SetupBuffer();
        R_SetupBuffer();

        // Initialize Clean factors to fix invisible menu text
        int cx1, cx2;
        V_CalcCleanFacs(320, 200, width, height, &CleanXfac, &CleanYfac, &cx1, &cx2);
        CleanWidth = width / CleanXfac;
        CleanHeight = height / CleanYfac;
        CleanXfac_1 = MAX(CleanXfac - 1, 1);
        CleanYfac_1 = MAX(CleanYfac - 1, 1);
        CleanWidth_1 = width / CleanXfac_1;
        CleanHeight_1 = height / CleanYfac_1;

        return true;
    }

    void DumpAdapters() {}

private:
    int m_Iterator;
};

void I_InitGraphics()
{
    if (Video == NULL)
        Video = new RGVideo();

    RG_LOGI("PSRAM before tables: %d KB\n", (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
    if (Col2RGB8 == NULL)
        Col2RGB8 = (DWORD (*)[256])rg_alloc(65 * 256 * sizeof(DWORD), MEM_SLOW);
    
    if (Col2RGB8_Inverse == NULL)
        Col2RGB8_Inverse = (DWORD (*)[256])rg_alloc(65 * 256 * sizeof(DWORD), MEM_SLOW);
    if (RGB32k == NULL)
        RGB32k = (BYTE (*)[32][32])rg_alloc(32 * 32 * 32 * sizeof(BYTE), MEM_SLOW);

    RG_LOGI("PSRAM after tables: %d KB\n", (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);

    if (ylookup == NULL)
        ylookup = (int *)rg_alloc(MAXHEIGHT * sizeof(int), MEM_SLOW);
    if (spanend == NULL)
        spanend = (short *)rg_alloc(MAXHEIGHT * sizeof(short), MEM_SLOW);
    if (dc_tempbuff == NULL)
        dc_tempbuff = (BYTE *)rg_alloc(MAXHEIGHT * 4, MEM_SLOW);
    if (dc_tspans == NULL)
        dc_tspans = (unsigned int (*)[MAXHEIGHT])rg_alloc(4 * MAXHEIGHT * sizeof(unsigned int), MEM_SLOW);
    if (zeroarray == NULL) {
        zeroarray = (short *)rg_alloc(MAXWIDTH * sizeof(short), MEM_SLOW);
        memset(zeroarray, 0, MAXWIDTH * sizeof(short));
    }
    if (bottomclipper == NULL) {
        bottomclipper = (short *)rg_alloc(MAXWIDTH * sizeof(short), MEM_SLOW);
        memset(bottomclipper, 0, MAXWIDTH * sizeof(short));
    }
    if (topclipper == NULL) {
        topclipper = (short *)rg_alloc(MAXWIDTH * sizeof(short), MEM_SLOW);
        memset(topclipper, 0, MAXWIDTH * sizeof(short));
    }

    if (finesine == NULL)
    {
        finesine = (fixed *)rg_alloc((FINEANGLES + FINEANGLES / 4) * sizeof(fixed), MEM_SLOW);
        finecosine = finesine + ANG90;
    }
    if (finetangent == NULL)
        finetangent = (fixed *)rg_alloc((FINEANGLES / 2 + ANG180) * sizeof(fixed), MEM_SLOW);
    
    RG_LOGI("PSRAM after math: %d KB\n", (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
}

void I_ShutdownGraphics()
{
    if (Col2RGB8) { free(Col2RGB8); Col2RGB8 = NULL; }
    if (Col2RGB8_Inverse) { free(Col2RGB8_Inverse); Col2RGB8_Inverse = NULL; }
    if (RGB32k) { free(RGB32k); RGB32k = NULL; }
    
    if (ylookup) { free(ylookup); ylookup = NULL; }
    if (spanend) { free(spanend); spanend = NULL; }
    if (dc_tempbuff) { free(dc_tempbuff); dc_tempbuff = NULL; }
    if (dc_tspans) { free(dc_tspans); dc_tspans = NULL; }
    if (zeroarray) { free(zeroarray); zeroarray = NULL; }
    if (bottomclipper) { free(bottomclipper); bottomclipper = NULL; }
    if (topclipper) { free(topclipper); topclipper = NULL; }

    if (finesine) { free(finesine); finesine = NULL; }
    if (finetangent) { free(finetangent); finetangent = NULL; }
}

extern "C" void RGFB_Init()
{
    I_InitGraphics();
}
