#include "Core.h"
#if defined CC_BUILD_RETROGO
#include <sdkconfig.h>

#include "Window.h"
#include "Graphics.h"
#include "String.h"
#include "Input.h"
#include "Event.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "Errors.h"
#include <rg_system.h>

static rg_surface_t* fb_surfaces[2];
static int active_fb_idx = 0;

struct _DisplayData DisplayInfo;
struct cc_window WindowInfo;

void Window_PreInit(void) { }

void Window_Init(void) {
    DisplayInfo.Width  = rg_display_get_width();
    DisplayInfo.Height = rg_display_get_height();
    DisplayInfo.Depth  = 16;
    DisplayInfo.ScaleX = 0.5f;
    DisplayInfo.ScaleY = 0.5f;

    Window_Main.Width    = DisplayInfo.Width;
    Window_Main.Height   = DisplayInfo.Height;
    Window_Main.Exists   = true;
    Window_Main.Focused  = true;
    Window_Main.UIScaleX = DEFAULT_UI_SCALE_X;
    Window_Main.UIScaleY = DEFAULT_UI_SCALE_Y;
}

void Window_Free(void) {
    if (fb_surfaces[0]) rg_surface_free(fb_surfaces[0]);
    if (fb_surfaces[1]) rg_surface_free(fb_surfaces[1]);
    fb_surfaces[0] = NULL;
    fb_surfaces[1] = NULL;
}

void Window_Create3D(int width, int height) {
#if defined(CONFIG_IDF_TARGET_ESP32P4)
    Window_Main.Width  = 320;
    Window_Main.Height = 240;
#else
    Window_Main.Width  = DisplayInfo.Width;
    Window_Main.Height = DisplayInfo.Height;
#endif

    fb_surfaces[0] = rg_surface_create(Window_Main.Width, Window_Main.Height, RG_PIXEL_565_LE, MEM_SLOW);
    fb_surfaces[1] = rg_surface_create(Window_Main.Width, Window_Main.Height, RG_PIXEL_565_LE, MEM_SLOW);
    if (!fb_surfaces[0] || !fb_surfaces[1]) RG_PANIC("Failed to create framebuffer surfaces");

    active_fb_idx = 0;

    rg_display_set_scaling(RG_DISPLAY_SCALING_FULL);

    Event_RaiseVoid(&WindowEvents.Resized);
}

void Window_SetTitle(const cc_string* title) { }

void Window_Show(void) { }
void Window_Show2(void) { }

void Window_ProcessEvents(float delta) {
    // Bypassed the built-in retro-go launcher overlay to allow the in-game menu to open instead
}

void Window_Terminate(void) {
    WindowInfo.Exists = false;
    Event_RaiseVoid(&WindowEvents.Closing);
}

void Window_Close(void) {
    Window_Terminate();
}

void Window_AllocFramebuffer(struct Bitmap* bmp, int width, int height) {
    bmp->scan0  = (BitmapCol*)fb_surfaces[active_fb_idx]->data;
    bmp->width  = fb_surfaces[active_fb_idx]->width;
    bmp->height = fb_surfaces[active_fb_idx]->height;
}

void Window_DrawFramebuffer(Rect2D r, struct Bitmap* bmp) {
    // Submit the surface asynchronously to the display task queue (Zero-Copy!)
    rg_display_submit(fb_surfaces[active_fb_idx], 0);

    // Swap active framebuffer
    active_fb_idx = 1 - active_fb_idx;
    bmp->scan0 = (BitmapCol*)fb_surfaces[active_fb_idx]->data;
}

void Window_FreeFramebuffer(struct Bitmap* bmp) { }

cc_bool Window_IsVisible(void) { return true; }
cc_bool Window_IsVisible2(void) { return true; }

void Window_SetSize(int width, int height) { }
void Window_SetSize2(int width, int height) { }

void Window_RequestClose(void) {
    Window_Terminate();
}

int Window_GetWindowState(void) { return 0; }
cc_result Window_EnterFullscreen(void) { return 0; }
cc_result Window_ExitFullscreen(void) { return 0; }

void Window_ShowDialog(const char* title, const char* msg) {
    RG_LOGI("DIALOG: %s - %s\n", title, msg);
}

void Window_UpdateRawMouse(void) { }
cc_result Window_OpenFileDialog(const struct OpenFileDialogArgs* args) { return ERR_NOT_SUPPORTED; }
cc_result Window_SaveFileDialog(const struct SaveFileDialogArgs* args) { return ERR_NOT_SUPPORTED; }


/*########################################################################################################################*
*-------------------------------------------------------Cursor/Touch------------------------------------------------------*
*#########################################################################################################################*/
void Cursor_SetVisible(cc_bool visible) { }
void Cursor_SetPosition(int x, int y) { }
void Window_EnableRawMouse(void) { }
void Window_DisableRawMouse(void) { }

void OnscreenKeyboard_Open(struct OpenKeyboardArgs* args) { }
void OnscreenKeyboard_SetText(const cc_string* text) { }
void OnscreenKeyboard_Close(void) { }

cc_bool Window_GetInertia(float* speed, float* friction) { return false; }

/*########################################################################################################################*
*-------------------------------------------------------Clipboard---------------------------------------------------------*
*#########################################################################################################################*/
void Clipboard_GetText(cc_string* value) { }
void Clipboard_SetText(const cc_string* value) { }

#endif
