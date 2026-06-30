#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rg_memory.h"
#include "retrogo/rg_psram.h"

extern "C" {
#include <rg_system.h>
}

#include "osdepend.h"
#include "mame.h"
#include "driver.h"
#include "unzip.h"
#include "zlib.h"

static rg_surface_t *screen = NULL;
static uint16_t *palette = NULL;
static int screen_width, screen_height;
static int visible_min_x, visible_max_x, visible_min_y, visible_max_y;
static int audio_is_stereo = 0;
static uint32_t frame_count = 0;

/* Globals for vector.cpp linkage */
static char dummy_dirty[1024];
char *dirty_new = dummy_dirty;
char *dirty_old = dummy_dirty;

static char g_rom_dir[256] = RG_BASE_PATH_ROMS "/mame";

extern "C" {
void osd_set_rom_path(const char *path)
{
    if (path && path[0]) {
        strncpy(g_rom_dir, path, sizeof(g_rom_dir) - 1);
        g_rom_dir[sizeof(g_rom_dir) - 1] = 0;
        char *p = strrchr(g_rom_dir, '/');
        if (p) *p = 0;
        RG_LOGI("ROM directory set to: %s\n", g_rom_dir);
    }
}
}

extern "C" {
int soundcard = 1;
int usestereo = 1;

// Dummy drivers for linkage
extern "C" const struct GameDriver driver_0 = {
    __FILE__,
    0,
    "0",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0x4000 // NOT_A_DRIVER
};

// Dummy drivers for linkage

typedef enum
{
	kPlainFile,
	kRAMFile,
	kZippedFile
} eFileType;

typedef struct
{
	FILE *file;
	unsigned char *data;
	unsigned int offset;
	unsigned int length;
	eFileType type;
} FakeFileHandle;

static struct KeyboardInfo key_list[] =
{
	{ "A",          KEYCODE_A,          KEYCODE_A },
	{ "B",          KEYCODE_B,          KEYCODE_B },
	{ "1",          KEYCODE_1,          KEYCODE_1 },
	{ "5",          KEYCODE_5,          KEYCODE_5 },
	{ "O",          KEYCODE_O,          KEYCODE_O },
	{ "K",          KEYCODE_K,          KEYCODE_K },
	{ "UP",         KEYCODE_UP,         KEYCODE_UP },
	{ "DOWN",       KEYCODE_DOWN,       KEYCODE_DOWN },
	{ "LEFT",       KEYCODE_LEFT,       KEYCODE_LEFT },
	{ "RIGHT",      KEYCODE_RIGHT,      KEYCODE_RIGHT },
	{ 0, 0, 0 }
};

static struct JoystickInfo joy_list[] =
{
	{ "UP",         0, JOYCODE_1_UP },
	{ "DOWN",       1, JOYCODE_1_DOWN },
	{ "LEFT",       2, JOYCODE_1_LEFT },
	{ "RIGHT",      3, JOYCODE_1_RIGHT },
	{ "A",          4, JOYCODE_1_BUTTON1 },
	{ "B",          5, JOYCODE_1_BUTTON2 },
	{ "SELECT",     6, KEYCODE_5 },
	{ "START",      7, KEYCODE_1 },
	{ 0, 0, 0 }
};

int osd_init(void)
{
	if (!rg_psram) {
		rg_psram = (struct rg_psram_master *)malloc(sizeof(struct rg_psram_master));

		if (rg_psram) {
			memset(rg_psram, 0, sizeof(struct rg_psram_master));
		}
	}
    return 0;
}

void osd_exit(void)
{
}

struct osd_bitmap *osd_alloc_bitmap(int width, int height, int depth)
{
    struct osd_bitmap *bitmap = (struct osd_bitmap *)malloc(sizeof(struct osd_bitmap));
    if (!bitmap) return NULL;

    bitmap->width = width;
    bitmap->height = height;
    bitmap->depth = depth;
    
    int stride = width + 32;
    int total_height = height + 32;
    int bpp = (depth == 16) ? 2 : 1;
    
    unsigned char *data = (unsigned char *)malloc(stride * total_height * bpp);
    if (!data) {
        free(bitmap);
        return NULL;
    }
    memset(data, 0, stride * total_height * bpp);
    
    bitmap->_private = data;
    bitmap->line = (unsigned char **)rg_alloc(height * sizeof(unsigned char *), MEM_SLOW);
    if (!bitmap->line) {
        mame_free(data);
        free(bitmap);
        return NULL;
    }
    for (int i = 0; i < height; i++) {
        bitmap->line[i] = data + (16 * stride + 16 + i * stride) * bpp;
    }
    
    return bitmap;
}

void osd_free_bitmap(struct osd_bitmap *bitmap)
{
    if (bitmap) {
        if (bitmap->_private) mame_free(bitmap->_private);
        if (bitmap->line) free(bitmap->line);
        free(bitmap);
    }
}

void osd_clearbitmap(struct osd_bitmap *bitmap)
{
    if (!bitmap || !bitmap->_private) return;
    int stride = bitmap->width + 32;
    int total_height = bitmap->height + 32;
    int bpp = (bitmap->depth == 16) ? 2 : 1;
    memset(bitmap->_private, 0, stride * total_height * bpp);
}

int osd_create_display(int width, int height, int depth, int fps, int attributes, int orientation)
{
    screen_width = width;
    screen_height = height;

    visible_min_x = 0;
    visible_max_x = width - 1;
    visible_min_y = 0;
    visible_max_y = height - 1;

    RG_LOGI("Creating display: %dx%d %dbpp %dfps (orient %d)\n", width, height, depth, fps, orientation);

    rg_pixel_format_t fmt = (depth == 8) ? RG_PIXEL_PAL565_LE : RG_PIXEL_565_LE;

    if (screen) rg_surface_free(screen);
    screen = rg_surface_create(width, height, fmt, MEM_FAST);

    if (!screen) {
        RG_LOGE("Failed to create surface in fast RAM, falling back to PSRAM\n");
        screen = rg_surface_create(width, height, fmt, MEM_SLOW);
    }

    if (!screen) {
        RG_LOGE("Failed to create surface!\n");
        return 1;
    }

    if (!palette) {
        palette = (uint16_t *)malloc(1024 * 2); // Allocate for max possible colors
        if (palette) {
            // Initialize basic UI colors: 0=black, rest=RED (for visibility)
            palette[0] = 0x0000;
            for (int i = 1; i < 1024; i++) palette[i] = 0xF800;
        }
    }

    if (fmt == RG_PIXEL_PAL565_LE) {
        screen->palette = palette;
    }

    frame_count = 0;

    return 0;
}
int osd_set_display(int width, int height, int depth, int attributes, int orientation)
{
    return osd_create_display(width, height, depth, 60, attributes, orientation);
}

void osd_close_display(void)
{
    if (screen) {
        rg_surface_free(screen);
        screen = NULL;
    }
}

void osd_set_visible_area(int min_x, int max_x, int min_y, int max_y)
{
    visible_min_x = min_x;
    visible_max_x = max_x;
    visible_min_y = min_y;
    visible_max_y = max_y;
}

int osd_allocate_colors(unsigned int totalcolors, const unsigned char *palette_data, unsigned short *pens, int modifiable)
{
    for (unsigned int i = 0; i < totalcolors; i++) {
        int r = palette_data[i * 3 + 0];
        int g = palette_data[i * 3 + 1];
        int b = palette_data[i * 3 + 2];
        unsigned short color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

        if (pens) pens[i] = i; 

        if (palette && i < 1024) {
            palette[i] = color;
        }
    }
    return 0;
}

void osd_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue)
{
    unsigned short color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
    if (palette && pen < 1024) {
        palette[pen] = color;
    }
}

void osd_get_pen(int pen, unsigned char *red, unsigned char *green, unsigned char *blue)
{
}

void osd_mark_dirty(int xmin, int ymin, int xmax, int ymax, int ui)
{
}

int osd_skip_this_frame(void)
{
    return 0;
}

static rg_surface_t current_screen = {0};

void osd_update_video_and_audio(struct osd_bitmap *bitmap)
{
    if (!bitmap || !bitmap->line) return;
    
    int width = visible_max_x - visible_min_x + 1;
    int height = visible_max_y - visible_min_y + 1;
    
    if (width <= 0 || height <= 0) return;

    if (visible_min_y + height > bitmap->height) height = bitmap->height - visible_min_y;

    if (height <= 0 || width <= 0) return;

    current_screen.width = width;
    current_screen.height = height;
    current_screen.stride = bitmap->width + 32;
    if (bitmap->depth == 16) current_screen.stride *= 2;
    
    current_screen.format = (bitmap->depth == 8) ? RG_PIXEL_PAL565_LE : RG_PIXEL_565_LE;
    current_screen.palette = palette;
    
    if (bitmap->depth == 8) {
        current_screen.data = &bitmap->line[visible_min_y][visible_min_x];
    } else {
        current_screen.data = &bitmap->line[visible_min_y][visible_min_x * 2];
    }

    rg_display_submit(&current_screen, 0);

    if ((frame_count++ % 60) == 0) {
        RG_LOGI("OSD: Submitted frame %d (%dx%d %dbpp)\n", frame_count, width, height, bitmap->depth);
    }
}
void osd_set_gamma(float _gamma) {}
float osd_get_gamma(void) { return 1.0f; }
void osd_set_brightness(int brightness) {}
int osd_get_brightness(void) { return 100; }
void osd_save_snapshot(struct osd_bitmap *bitmap) {}

// Sound
static rg_audio_frame_t *stereo_buf = NULL;

int osd_start_audio_stream(int stereo)
{
    audio_is_stereo = stereo;
    if (!stereo_buf) stereo_buf = (rg_audio_frame_t *)malloc(1024 * sizeof(rg_audio_frame_t));
    return 11025 / 60;
}

int osd_update_audio_stream(INT16 *buffer)
{
    int count = 11025 / 60;
    if (audio_is_stereo) {
        rg_audio_submit((const rg_audio_frame_t *)buffer, count);
    } else {
        if (stereo_buf) {
            for (int i = 0; i < count; i++) {
                stereo_buf[i].left = buffer[i];
                stereo_buf[i].right = buffer[i];
            }
            rg_audio_submit(stereo_buf, count);
        }
    }
    return count;
}

extern "C" {
inline void m65c02_take_irq(void) {}
// n2a03_irq moved to core
}

void osd_stop_audio_stream(void)
{
    if (stereo_buf) mame_free(stereo_buf);
    stereo_buf = NULL;
}
void osd_set_mastervolume(int attenuation) {}
int osd_get_mastervolume(void) { return 0; }
void osd_sound_enable(int enable) {}
void osd_opl_control(int chip, int reg) {}
void osd_opl_write(int chip, int data) {}

// Input
static uint32_t gamepad_state = 0;

const struct KeyboardInfo *osd_get_key_list(void) { return key_list; }

int osd_is_key_pressed(int keycode) 
{ 
    switch (keycode) {
        case KEYCODE_UP:    return (gamepad_state & RG_KEY_UP);
        case KEYCODE_DOWN:  return (gamepad_state & RG_KEY_DOWN);
        case KEYCODE_LEFT:  return (gamepad_state & RG_KEY_LEFT);
        case KEYCODE_RIGHT: return (gamepad_state & RG_KEY_RIGHT);
        case KEYCODE_A:     return (gamepad_state & RG_KEY_A);
        case KEYCODE_B:     return (gamepad_state & RG_KEY_B);
        case KEYCODE_1:     return (gamepad_state & RG_KEY_START);
        case KEYCODE_5:     return (gamepad_state & RG_KEY_SELECT);
        case KEYCODE_O:     return (gamepad_state & RG_KEY_A);
        case KEYCODE_K:     return (gamepad_state & RG_KEY_B);
    }
    return 0; 
}

int osd_wait_keypress(void) { return 0; }
int osd_readkey_unicode(int flush) { return 0; }

const struct JoystickInfo *osd_get_joy_list(void) { return joy_list; }

void osd_poll_joysticks(void)
{
    gamepad_state = rg_input_read_gamepad();
    if (gamepad_state & RG_KEY_MENU) {
        rg_gui_game_menu();
    }
}

int osd_is_joy_pressed(int joycode)
{
    switch (joycode) {
        case 0: return (gamepad_state & RG_KEY_UP);
        case 1: return (gamepad_state & RG_KEY_DOWN);
        case 2: return (gamepad_state & RG_KEY_LEFT);
        case 3: return (gamepad_state & RG_KEY_RIGHT);
        case 4: return (gamepad_state & RG_KEY_A);
        case 5: return (gamepad_state & RG_KEY_B);
        case 6: return (gamepad_state & RG_KEY_SELECT);
        case 7: return (gamepad_state & RG_KEY_START);
    }
    return 0;
}

int osd_joystick_needs_calibration(void) { return 0; }
void osd_joystick_start_calibration(void) {}
char *osd_joystick_calibrate_next(void) { return NULL; }
void osd_joystick_calibrate(void) {}
void osd_joystick_end_calibration(void) {}
void osd_trak_read(int player, int *deltax, int *deltay) {}
void osd_analogjoy_read(int player, int *analog_x, int *analog_y) {}
void osd_customize_inputport_defaults(struct ipd *defaults) {}

// File I/O
int osd_faccess(const char *filename, int filetype) { return 0; }

extern "C" int gUnzipQuiet;

void *osd_fopen(const char *gamename, const char *filename, int filetype, int read_or_write)
{
    if (!gamename) gamename = "";
    if (!filename) filename = "";

    FakeFileHandle *f = (FakeFileHandle *)malloc(sizeof(FakeFileHandle));
    if (!f) return NULL;
    memset(f, 0, sizeof(FakeFileHandle));

    char path[256];
    const char *ext = "";

    if (filetype == OSD_FILETYPE_ROM || filetype == OSD_FILETYPE_SAMPLE) {
        if (read_or_write) {
            free(f);
            return NULL;
        }
        
        // Suppress errors during probing
        int old_quiet = gUnzipQuiet;
        gUnzipQuiet = 1;

        bool found = false;

        // 1. Try as plain file in current game dir
        if (filename[0]) {
            sprintf(path, "%s/%s/%s", g_rom_dir, gamename, filename);
            f->file = fopen(path, "rb");
            if (f->file) {
                f->type = kPlainFile;
                found = true;
            }
        }

        // 2. Try in game zip file (in ROM directory)
        if (!found && gamename[0]) {
            sprintf(path, "%s/%s.zip", g_rom_dir, gamename);
            if (load_zipped_file(path, filename, &f->data, &f->length) == 0) {
                f->type = kZippedFile;
                found = true;
            }
        }
        
        // 3. Try in parent zip file (in ROM directory)
        if (!found && Machine && Machine->gamedrv && Machine->gamedrv->clone_of && Machine->gamedrv->clone_of->name) {
            sprintf(path, "%s/%s.zip", g_rom_dir, Machine->gamedrv->clone_of->name);
            if (load_zipped_file(path, filename, &f->data, &f->length) == 0) {
                f->type = kZippedFile;
                found = true;
            }
        }

        // 4. Try explicitly for neogeo.zip if filename looks like a Neo Geo BIOS ROM
        if (!found && (strncmp(filename, "ng-", 3) == 0 || strstr(filename, "neo-") != NULL)) {
             sprintf(path, "%s/neogeo.zip", g_rom_dir);
             if (load_zipped_file(path, filename, &f->data, &f->length) == 0) {
                 f->type = kZippedFile;
                 found = true;
             }
             if (!found) {
                 sprintf(path, RG_BASE_PATH_BIOS "/neogeo.zip");
                 if (load_zipped_file(path, filename, &f->data, &f->length) == 0) {
                     f->type = kZippedFile;
                     found = true;
                 }
             }
        }

        // 5. Try in Retro-Go BIOS directory (ONLY for the driver name, NOT the game)
        if (!found && (strcmp(gamename, "neogeo") == 0 || strcmp(gamename, "cpis") == 0)) {
            sprintf(path, RG_BASE_PATH_BIOS "/%s.zip", gamename);
            if (load_zipped_file(path, filename, &f->data, &f->length) == 0) {
                f->type = kZippedFile;
                found = true;
            }
        }

        // 6. Try samples
        if (!found && filetype == OSD_FILETYPE_SAMPLE && filename[0]) {
            sprintf(path, "%s/samples/%s", g_rom_dir, filename);
            f->file = fopen(path, "rb");
            if (f->file) {
                f->type = kPlainFile;
                found = true;
            } else {
                sprintf(path, "%s/samples/%s.zip", g_rom_dir, gamename);
                if (load_zipped_file(path, filename, &f->data, &f->length) == 0) {
                    f->type = kZippedFile;
                    found = true;
                }
            }
        }

        gUnzipQuiet = old_quiet;

        if (found) {
            f->offset = 0;
            return f;
        }

        free(f);
        return NULL;

    } else {
        const char *base_path = RG_BASE_PATH_SAVES "/mame";
        
        switch (filetype) {
            case OSD_FILETYPE_NVRAM:     ext = ".nv"; break;
            case OSD_FILETYPE_HIGHSCORE: ext = ".hi"; break;
            case OSD_FILETYPE_CONFIG:    ext = ".cfg"; base_path = RG_BASE_PATH_CONFIG "/mame"; break;
            case OSD_FILETYPE_INPUTLOG:  ext = ".inp"; break;
            case OSD_FILETYPE_STATE:     ext = ".sta"; break;
            case OSD_FILETYPE_MEMCARD:   ext = ".mcd"; break;
            case OSD_FILETYPE_SCREENSHOT:ext = ".png"; break;
            default: break;
        }

        rg_storage_mkdir(base_path);

        if (filename && filename[0]) {
            sprintf(path, "%s/%s", base_path, filename);
        } else {
            sprintf(path, "%s/%s%s", base_path, gamename, ext);
        }

        f->file = fopen(path, read_or_write ? "wb" : "rb");
        if (f->file) {
            f->type = kPlainFile;
            return f;
        }
        
        free(f);
        return NULL;
    }
}

int osd_fread(void *file, void *buffer, int length) 
{
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return 0;
    if (f->type == kZippedFile || f->type == kRAMFile) {
        if (f->offset + length > f->length) length = f->length - f->offset;
        if (length > 0) {
            memcpy(buffer, f->data + f->offset, length);
            f->offset += length;
        } else {
            length = 0;
        }
        return length;
    }
    return f->file ? fread(buffer, 1, length, f->file) : 0; 
}

int osd_fwrite(void *file, const void *buffer, int length) 
{ 
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return 0;
    if (f->type == kZippedFile || f->type == kRAMFile) return 0;
    return f->file ? fwrite(buffer, 1, length, f->file) : 0; 
}

int osd_fread_swap(void *file, void *buffer, int length) { return osd_fread(file, buffer, length); }
int osd_fwrite_swap(void *file, const void *buffer, int length) { return osd_fwrite(file, buffer, length); }
int osd_fread_scatter(void *file, void *buffer, int length, int increment) 
{ 
    unsigned char *buf = (unsigned char*)buffer;
    FakeFileHandle *f = (FakeFileHandle *) file;
    unsigned char tempbuf[4096];
    int totread = 0, r, i;

    if (!f) return 0;

    if (f->type == kPlainFile)
    {
        while (length > 0)
        {
            r = length > 4096 ? 4096 : length;
            r = fread(tempbuf, 1, r, f->file);
            if (r == 0) return totread;
            for(i = 0; i < r; i++)
            {
                *buf = tempbuf[i];
                buf += increment;
            }
            totread += r;
            length -= r;
        }
        return totread;
    }
    else if (f->type == kZippedFile || f->type == kRAMFile)
    {
        if (f->data)
        {
            if (length + f->offset > f->length)
                length = f->length - f->offset;
            
            unsigned char *src = f->data + f->offset;
            for(i = 0; i < length; i++)
            {
                *buf = src[i];
                buf += increment;
            }
            f->offset += length;
            return length;
        }
    }

    return 0; 
}

int osd_fseek(void *file, int offset, int whence) 
{ 
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return -1;
    if (f->type == kZippedFile || f->type == kRAMFile) {
        switch (whence) {
            case SEEK_SET: f->offset = offset; break;
            case SEEK_CUR: f->offset += offset; break;
            case SEEK_END: f->offset = f->length + offset; break;
        }
        if (f->offset > f->length) f->offset = f->length;
        return 0;
    }
    return f->file ? fseek(f->file, offset, whence) : -1; 
}

void osd_fclose(void *file) 
{ 
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return;
    if (f->type == kPlainFile && f->file) fclose(f->file);
    if ((f->type == kZippedFile || f->type == kRAMFile) && f->data) free(f->data);
    free(f);
}

int osd_fchecksum(const char *gamename, const char *filename, unsigned int *length, unsigned int *sum) 
{ 
    void *f = osd_fopen(gamename, filename, OSD_FILETYPE_ROM, 0);
    if (!f) return -1;
    *length = osd_fsize(f);
    FakeFileHandle *fh = (FakeFileHandle *)f;
    if (fh->type == kZippedFile || fh->type == kRAMFile) {
        *sum = crc32(0L, fh->data, fh->length);
    } else {
        unsigned char *buf = (unsigned char *)malloc(*length);
        if (buf) {
            osd_fread(f, buf, *length);
            *sum = crc32(0L, buf, *length);
            rg_free(buf);
        } else {
            *sum = 0;
        }
    }
    osd_fclose(f);
    return 0;
}

int osd_fsize(void *file) 
{ 
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return 0;
    if (f->type == kZippedFile || f->type == kRAMFile) return f->length;
    if (!f->file) return 0;
    long pos = ftell(f->file); 
    fseek(f->file, 0, SEEK_END); 
    long size = ftell(f->file); 
    fseek(f->file, pos, SEEK_SET); 
    return size; 
}

unsigned int osd_fcrc(void *file) 
{ 
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return 0;
    if (f->type == kZippedFile || f->type == kRAMFile) return crc32(0L, f->data, f->length);
    
    int size = osd_fsize(file);
    if (size <= 0) return 0;
    unsigned char *buf = (unsigned char *)malloc(size);
    unsigned int crc = 0;
    if (buf) {
        long pos = ftell(f->file);
        fseek(f->file, 0, SEEK_SET);
        fread(buf, 1, size, f->file);
        crc = crc32(0L, buf, size);
        fseek(f->file, pos, SEEK_SET);
        rg_free(buf);
    }
    return crc;
}

int osd_fgetc(void *file) 
{ 
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return EOF;
    if (f->type == kZippedFile || f->type == kRAMFile) {
        if (f->offset >= f->length) return EOF;
        return f->data[f->offset++];
    }
    return f->file ? fgetc(f->file) : EOF; 
}

int osd_ungetc(int c, void *file) 
{ 
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return EOF;
    if (f->type == kZippedFile || f->type == kRAMFile) {
        if (f->offset > 0) f->offset--;
        return c;
    }
    return f->file ? ungetc(c, f->file) : EOF; 
}

char *osd_fgets(char *s, int n, void *file) 
{ 
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return NULL;
    if (f->type == kZippedFile || f->type == kRAMFile) {
        if (f->offset >= f->length) return NULL;
        int i = 0;
        while (i < n - 1 && f->offset < f->length) {
            s[i++] = f->data[f->offset++];
            if (s[i-1] == '\n') break;
        }
        s[i] = 0;
        return s;
    }
    return f->file ? fgets(s, n, f->file) : NULL; 
}

int osd_feof(void *file) 
{ 
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return 1;
    if (f->type == kZippedFile || f->type == kRAMFile) return f->offset >= f->length;
    return f->file ? feof(f->file) : 1; 
}

int osd_ftell(void *file) 
{ 
    FakeFileHandle *f = (FakeFileHandle *)file;
    if (!f) return 0;
    if (f->type == kZippedFile || f->type == kRAMFile) return f->offset;
    return f->file ? ftell(f->file) : 0; 
}

int osd_display_loading_rom_message(const char *name, int current, int total) { return 0; }
void osd_pause(int paused) {}
void osd_led_w(int led, int on) {}

void odx_clear_video() {}

unsigned long odx_timer_read(void)
{
    return (unsigned long)rg_system_timer();
}

void odx_timer_delay(unsigned int ticks)
{
    rg_task_delay(ticks);
}

void odx_printf(const char* fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    vprintf(fmt, argptr);
    va_end(argptr);
}

void logerror(const char *text, ...)
{
    va_list argptr;
    va_start(argptr, text);
    vprintf(text, argptr);
    va_end(argptr);
}

} // extern "C"