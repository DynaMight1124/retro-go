#include <rg_system.h>
#include <snes9x.h>
#include <memmap.h>
#include <apu.h>
#include <gfx.h>
#include <soundux.h>
#include <snapshot.h>
#include <cheats.h>
#include <sa1.h>
#include <srtc.h>
#include <spc7110.h>
#include <spc7110dec.h>
#include <dsp1.h>
#include <math.h>

#include <new>
#include <stdio.h>

#define AUDIO_SAMPLE_RATE   (32040)
#define AUDIO_BUFFER_LENGTH (AUDIO_SAMPLE_RATE / 50 + 1)

typedef struct
{
	char name[16];
	struct {
		uint16_t snes_mask;
		uint16_t local_mask;
		uint16_t mod_mask;
	} keys[16];
} keymap_t;

enum {
    KEYMAP_TYPE_A = 0,
    KEYMAP_TYPE_B,
    KEYMAP_TYPE_C,
    KEYMAP_REGULAR
};

static const keymap_t KEYMAPS[] = {
	{"Type A", {
		{SNES_A_MASK, (uint16_t)RG_KEY_A, 0},
		{SNES_B_MASK, (uint16_t)RG_KEY_B, 0},
		{SNES_X_MASK, (uint16_t)RG_KEY_START, 0},
		{SNES_Y_MASK, (uint16_t)RG_KEY_SELECT, 0},
		{SNES_TL_MASK, (uint16_t)RG_KEY_B, RG_KEY_MENU},
		{SNES_TR_MASK, (uint16_t)RG_KEY_A, RG_KEY_MENU},
		{SNES_START_MASK, (uint16_t)RG_KEY_START, RG_KEY_MENU},
		{SNES_SELECT_MASK, (uint16_t)RG_KEY_SELECT, RG_KEY_MENU},
		{SNES_UP_MASK, (uint16_t)RG_KEY_UP, 0},
		{SNES_DOWN_MASK, (uint16_t)RG_KEY_DOWN, 0},
		{SNES_LEFT_MASK, (uint16_t)RG_KEY_LEFT, 0},
		{SNES_RIGHT_MASK, (uint16_t)RG_KEY_RIGHT, 0},
	}},
	{"Regular", {
		{SNES_A_MASK, (uint16_t)RG_KEY_A, 0},
		{SNES_B_MASK, (uint16_t)RG_KEY_B, 0},
		{SNES_X_MASK, (uint16_t)RG_KEY_X, 0},
		{SNES_Y_MASK, (uint16_t)RG_KEY_Y, 0},
		{SNES_TL_MASK, (uint16_t)RG_KEY_L, 0},
		{SNES_TR_MASK, (uint16_t)RG_KEY_R, 0},
		{SNES_START_MASK, (uint16_t)RG_KEY_START, 0},
		{SNES_SELECT_MASK, (uint16_t)RG_KEY_SELECT, 0},
		{SNES_UP_MASK, (uint16_t)RG_KEY_UP, 0},
		{SNES_DOWN_MASK, (uint16_t)RG_KEY_DOWN, 0},
		{SNES_LEFT_MASK, (uint16_t)RG_KEY_LEFT, 0},
		{SNES_RIGHT_MASK, (uint16_t)RG_KEY_RIGHT, 0},
	}},
};

static const size_t KEYMAPS_COUNT = (sizeof(KEYMAPS) / sizeof(keymap_t));

static rg_app_t *app;
static rg_surface_t *updates[2];
static rg_surface_t *currentUpdate;
static rg_surface_t *lastCompleteUpdate;
static rg_audio_sample_t *audioBuffers[2];
static rg_audio_sample_t *currentAudioBuffer;
static rg_task_t *audioTask;
static constexpr size_t AUDIO_TASK_QUEUE_DEPTH = 2;

static unsigned int audioTaskPaused;

enum {
    AUDIO_TASK_SILENCE = 0,
    AUDIO_TASK_MIX = 1,
    AUDIO_TASK_PAUSE = 2,
};

static bool sound_enabled = true;
static int keymap_id = 0;
static keymap_t keymap;
static uint32_t gamepadState;
static uint32_t suppressedKeys;

static const char *SETTING_KEYMAP = "keymap";
static const char *SETTING_SOUND_EMULATION = "apu";

extern uint8 (*OBJOnLinePtr)[128];
extern SRTC_DATA *rtcPtr;
extern struct SCheatData *CheatPtr;
extern struct SLineData *LineData;
extern struct SLineMatrixData *LineMatrixData;
extern uint16 (*DirectColourMapsPtr) [256];
extern int32 *Echo;
extern int32 *MixBuffer;
extern int32 *EchoBuffer;
#ifdef SDD1_DECOMP
extern uint8 *SDD1Buffer;
#endif

static bool save_sram_if_dirty(void)
{
    if (!app || !app->romPath || !CPUPtr || !MemoryPtr || !CPU.SRAMModified)
        return true;

    char *path = rg_emu_get_path(RG_PATH_SAVE_SRAM, app->romPath);
    const bool saved = path && Memory.SaveSRAM(path);
    free(path);

    if (saved)
    {
        CPU.SRAMModified = FALSE;
        CPU.AutoSaveTimer = 0;
    }
    return saved;
}

static size_t get_rom_file_size(const char *path, bool is_zip)
{
    if (!is_zip)
        return rg_storage_stat(path).size;

    // Match Retro-Go's ZIP loader: locate the first local file header and use
    // its uncompressed size so Memory.Init can reserve a non-truncating buffer.
    struct __attribute__((packed)) zip_local_header_t
    {
        uint32_t magic;
        uint16_t version;
        uint16_t flags;
        uint16_t compression;
        uint16_t modified_time;
        uint16_t modified_date;
        uint32_t checksum;
        uint32_t compressed_size;
        uint32_t uncompressed_size;
        uint16_t filename_size;
        uint16_t extra_field_size;
    } header;

    FILE *file = fopen(path, "rb");
    if (!file) return 0;

    size_t size = 0;
    for (long position = 0; position < 0x10000; ++position)
    {
        if (fseek(file, position, SEEK_SET) != 0 ||
            fread(&header, sizeof(header), 1, file) != 1)
            break;
        if (header.magic == 0x04034b50)
        {
            size = header.uncompressed_size;
            break;
        }
    }
    fclose(file);
    return size;
}

// --- Snes9x Callbacks

START_EXTERN_C

void S9xMessage (int type, int number, const char *message)
{
    RG_LOGI("Snes9x Message: %s\n", message);
}

bool8 S9xInitUpdate (void)
{
    GFX.Screen = (uint8*)currentUpdate->data;
    return TRUE;
}

bool8 S9xDeinitUpdate (int Width, int Height, bool8 sixteen_bit)
{
    return TRUE;
}

void S9xSyncSpeed (void)
{
}

const char* S9xGetSnapshotDirectory (void)
{
    static char buffer[RG_PATH_MAX];
    char *path = rg_emu_get_path(RG_PATH_SAVE_STATE, NULL);
    snprintf(buffer, sizeof(buffer), "%s", path ? path : "");
    free(path);
    return buffer;
}

const char* S9xGetFilename (const char* ex)
{
    static char buffer[RG_PATH_MAX];
    char *path = rg_emu_get_path(RG_PATH_SAVE_SRAM, app ? app->romPath : NULL);
    snprintf(buffer, sizeof(buffer), "%s", path ? path : "");
    free(path);

    char *suffix = strrchr(buffer, '.');
    if (suffix && strcmp(suffix, ".sram") == 0)
        *suffix = 0;
    if (ex)
        strncat(buffer, ex, sizeof(buffer) - strlen(buffer) - 1);
    return buffer;
}

const char* S9xGetFilenameInc (const char* ex)
{
    return S9xGetFilename(ex);
}

bool8 S9xOpenSnapshotFile (const char *fname, bool8 read_only, STREAM *file)
{
    if (read_only)
    {
        if ((*file = OPEN_STREAM(fname, "rb")))
            return TRUE;
    }
    else
    {
        if ((*file = OPEN_STREAM(fname, "wb")))
            return TRUE;
    }
    return FALSE;
}

void S9xCloseSnapshotFile (STREAM file)
{
    CLOSE_STREAM(file);
}

void S9xExit ()
{
    rg_system_exit();
}

void S9xGenerateSound (void)
{
}

void S9xSetPalette () {}
void S9xExtraUsage () {}
void S9xParseArg (char **argv, int &index, int argc) {}

uint32 S9xReadJoypad(int port)
{
    if (port != 0) return 0;

    const uint32_t joystick = gamepadState;
    uint32_t joypad = 0;

    for (int i = 0; i < 16; ++i)
    {
        uint32_t bitmask = keymap.keys[i].local_mask | keymap.keys[i].mod_mask;
        if (bitmask && (joystick & bitmask) == bitmask)
        {
            joypad |= keymap.keys[i].snes_mask;
        }
    }

    return joypad | 0x80000000;
}

bool8 S9xReadMousePosition (int which1, int &x, int &y, uint32 &buttons) { return FALSE; }
bool8 S9xReadSuperScopePosition (int &x, int &y, uint32 &buttons) { return FALSE; }

void S9xLoadSDD1Data (void) {}

void S9xAutoSaveSRAM (void)
{
    save_sram_if_dirty();
}

const char *S9xBasename (const char *f)
{
    const char *p;
    if ((p = strrchr (f, '/')) != NULL)
        return (p + 1);
#ifdef __WIN32__
    if ((p = strrchr (f, '\\')) != NULL)
        return (p + 1);
#endif
    return (f);
}

END_EXTERN_C

void S9xProcessSound (unsigned int samples)
{
}

bool8 S9xOpenSoundDevice (int, bool8, int) { return TRUE; }

bool JustifierOffscreen (void) { return true; }
void JustifierButtons (uint32&) {}

void _makepath (char *path, const char *, const char *dir,
	const char *fname, const char *ext)
{
	if (dir && *dir)
	{
		strcpy (path, dir);
		strcat (path, "/");
	}
	else
	*path = 0;
	strcat (path, fname);
	if (ext && *ext)
	{
		strcat (path, ".");
		strcat (path, ext);
	}
}

void _splitpath (const char *path, char *drive, char *dir, char *fname,
	char *ext)
{
	if (drive) *drive = 0;

	const char *slash = strrchr (path, '/');
	if (!slash)
		slash = strrchr (path, '\\');

	const char *dot = strrchr (path, '.');

	if (dot && slash && dot < slash)
		dot = NULL;

	if (!slash)
	{
		if (dir) strcpy (dir, "");
		if (fname) {
            strcpy (fname, path);
            if (dot) fname[dot - path] = 0;
        }
		if (ext) {
            if (dot) strcpy (ext, dot + 1);
            else strcpy (ext, "");
        }
	}
	else
	{
		if (dir) {
            strcpy (dir, path);
            dir[slash - path] = 0;
        }
		if (fname) {
            strcpy (fname, slash + 1);
            if (dot) fname[dot - slash - 1] = 0;
        }
		if (ext) {
            if (dot) strcpy (ext, dot + 1);
            else strcpy (ext, "");
        }
	}
}

// --- Retro-Go Implementation

static void update_keymap(int id)
{
    keymap_id = id % (int)KEYMAPS_COUNT;
    keymap = KEYMAPS[keymap_id];
}

static bool screenshot_handler(const char *filename, int width, int height)
{
    rg_surface_t *surface = lastCompleteUpdate ? lastCompleteUpdate : currentUpdate;
    return surface && rg_surface_save_image_file(surface, filename, width, height);
}

static bool save_state_handler(const char *filename)
{
    return S9xFreezeGame(filename);
}

static bool load_state_handler(const char *filename)
{
    return S9xUnfreezeGame(filename);
}

static bool reset_handler(bool hard)
{
    S9xReset();
    return true;
}

static void event_handler(int event, void *arg)
{
    if (event == RG_EVENT_REDRAW)
    {
        rg_surface_t *surface = lastCompleteUpdate ? lastCompleteUpdate : currentUpdate;
        if (surface) rg_display_submit(surface, 0);
    }
    else if (event == RG_EVENT_SHUTDOWN)
    {
        save_sram_if_dirty();
        S9xSetSoundMute(TRUE);
        rg_audio_set_mute(true);
    }
}

static rg_gui_event_t apu_toggle_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        sound_enabled = !sound_enabled;
        rg_settings_set_number(NS_APP, SETTING_SOUND_EMULATION, sound_enabled);
    }
    strcpy(option->value, sound_enabled ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t echo_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        Settings.DisableSoundEcho = !Settings.DisableSoundEcho;
        S9xSetEchoEnable(SoundData.echo_channel_enable);
        rg_settings_set_number(NS_APP, "s9x_echo", Settings.DisableSoundEcho);
    }
    strcpy(option->value, !Settings.DisableSoundEcho ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t interp_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        Settings.InterpolatedSound = !Settings.InterpolatedSound;
        rg_settings_set_number(NS_APP, "s9x_interp", Settings.InterpolatedSound);
    }
    strcpy(option->value, Settings.InterpolatedSound ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t transp_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        Settings.Transparency = !Settings.Transparency;
        rg_settings_set_number(NS_APP, "s9x_transp", Settings.Transparency);
    }
    strcpy(option->value, Settings.Transparency ? _("On") : _("Off"));
    return RG_DIALOG_VOID;
}

static rg_gui_event_t change_keymap_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_PREV || event == RG_DIALOG_NEXT)
    {
        if (event == RG_DIALOG_PREV && --keymap_id < 0) keymap_id = KEYMAPS_COUNT - 1;
        if (event == RG_DIALOG_NEXT && ++keymap_id > (int)KEYMAPS_COUNT - 1) keymap_id = 0;
        update_keymap(keymap_id);
        rg_settings_set_number(NS_APP, SETTING_KEYMAP, keymap_id);
        return RG_DIALOG_REDRAW;
    }
    if (event == RG_DIALOG_ENTER) return RG_DIALOG_CANCEL;
    if (option->arg == -1) strcat(strcat(strcpy(option->value, "< "), keymap.name), " >");
    return RG_DIALOG_VOID;
}

static rg_gui_event_t menu_keymap_cb(rg_gui_option_t *option, rg_gui_event_t event)
{
    if (event == RG_DIALOG_ENTER)
    {
        const rg_gui_option_t options[] = {
            {-1, _("Profile"), (char *)"-", RG_DIALOG_FLAG_NORMAL, &change_keymap_cb},
            RG_DIALOG_END,
        };
        rg_gui_dialog(option->label, options, 0);
        return RG_DIALOG_REDRAW;
    }
    strcpy(option->value, keymap.name);
    return RG_DIALOG_VOID;
}

static void mix_samples(int count)
{
    if (!SoundDataPtr || !SettingsPtr) return;
    currentAudioBuffer = audioBuffers[currentAudioBuffer == audioBuffers[0]];
    S9xMixSamples((uint16 *)currentAudioBuffer, count);
}

static void audio_task(void *arg)
{
    rg_task_msg_t msg;
    while (rg_task_receive(&msg, -1))
    {
        if (msg.type == RG_TASK_MSG_STOP) break;

        const bool pause = msg.type == AUDIO_TASK_PAUSE;
        if (msg.type == AUDIO_TASK_MIX)
        {
            mix_samples(msg.dataInt << 1);
        }
        else
        {
            currentAudioBuffer = audioBuffers[currentAudioBuffer == audioBuffers[0]];
            memset(currentAudioBuffer, 0, msg.dataInt * sizeof(*currentAudioBuffer));
        }
        rg_audio_submit(currentAudioBuffer, msg.dataInt);
        if (pause)
        {
            __atomic_store_n(&audioTaskPaused, 1, __ATOMIC_RELEASE);
        }
    }
}

static void pause_audio_for_menu(int samplesPerFrame)
{
    __atomic_store_n(&audioTaskPaused, 0, __ATOMIC_RELAXED);
    const rg_task_msg_t msg = {
        .type = AUDIO_TASK_PAUSE,
        .dataInt = (uint32_t)samplesPerFrame,
    };
    RG_ASSERT(rg_task_send(audioTask, &msg, -1), "Failed to pause audio task!");

    // Wait until every earlier audio job is drained and a silent buffer has
    // reached the sink. The Retro-Go menu can then mute without racing the
    // independent audio worker.
    while (!__atomic_load_n(&audioTaskPaused, __ATOMIC_ACQUIRE))
        rg_task_yield();

    memset(audioBuffers[0], 0, AUDIO_BUFFER_LENGTH * sizeof(*audioBuffers[0]));
    memset(audioBuffers[1], 0, AUDIO_BUFFER_LENGTH * sizeof(*audioBuffers[1]));
}

static void options_handler(rg_gui_option_t *dest)
{
    *dest++ = (rg_gui_option_t){0, _("Audio enable"), (char *)"-", RG_DIALOG_FLAG_NORMAL, &apu_toggle_cb};
    *dest++ = (rg_gui_option_t){0, _("Sound Echo"), (char *)"-", RG_DIALOG_FLAG_NORMAL, &echo_cb};
    *dest++ = (rg_gui_option_t){0, _("Sound Interpolation"), (char *)"-", RG_DIALOG_FLAG_NORMAL, &interp_cb};
    *dest++ = (rg_gui_option_t){0, _("Transparency"), (char *)"-", RG_DIALOG_FLAG_NORMAL, &transp_cb};
    *dest++ = (rg_gui_option_t){0, _("Controls"), (char *)"-", RG_DIALOG_FLAG_NORMAL, &menu_keymap_cb};
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

extern "C" void app_main(void)
{
    rg_config_t config;
    memset(&config, 0, sizeof(config));
    config.sampleRate = AUDIO_SAMPLE_RATE;
    config.frameRate = 60;
    config.storageRequired = true;
    config.romRequired = true;
    config.handlers.loadState = &load_state_handler;
    config.handlers.saveState = &save_state_handler;
    config.handlers.reset = &reset_handler;
    config.handlers.screenshot = &screenshot_handler;
    config.handlers.event = &event_handler;
    config.handlers.options = &options_handler;

    app = rg_system_init(&config);

    // Allocate timing-sensitive structures in Internal DRAM
    CPUPtr = (struct SCPUState *)rg_alloc(sizeof(struct SCPUState), MEM_FAST);
    PPUPtr = (struct SPPU *)rg_alloc(sizeof(struct SPPU), MEM_FAST);
    IPPUPtr = (struct InternalPPU *)rg_alloc(sizeof(struct InternalPPU), MEM_FAST);
    SettingsPtr = (struct SSettings *)rg_alloc(sizeof(struct SSettings), MEM_FAST);
    SoundDataPtr = (SSoundData *)rg_alloc(sizeof(SSoundData), MEM_FAST);

    // Allocate other structures in external RAM
    MemoryPtr = (CMemory *)rg_alloc(sizeof(CMemory), MEM_SLOW);
    DMAPtr = (struct SDMA *)rg_alloc(sizeof(struct SDMA) * 8, MEM_FAST);
    SA1Ptr = (struct SSA1 *)rg_alloc(sizeof(struct SSA1), MEM_SLOW);
    rtcPtr = (SRTC_DATA *)rg_alloc(sizeof(SRTC_DATA), MEM_SLOW);
    GFXPtr = (struct SGFX *)rg_alloc(sizeof(struct SGFX), MEM_FAST);
    LineData = (struct SLineData *)rg_alloc(sizeof(struct SLineData) * 240, MEM_FAST);
    LineMatrixData = (struct SLineMatrixData *)rg_alloc(sizeof(struct SLineMatrixData) * 240, MEM_FAST);
    CheatPtr = (struct SCheatData *)rg_alloc(sizeof(struct SCheatData), MEM_SLOW);
    Echo = (int32 *)rg_alloc(sizeof(int32) * 24000, MEM_SLOW);
    MixBuffer = (int32 *)rg_alloc(sizeof(int32) * SOUND_BUFFER_SIZE, MEM_SLOW);
    EchoBuffer = (int32 *)rg_alloc(sizeof(int32) * SOUND_BUFFER_SIZE, MEM_SLOW);
    DirectColourMapsPtr = (uint16 (*)[256])rg_alloc(sizeof(uint16) * 8 * 256, MEM_FAST);
    OBJOnLinePtr = (uint8 (*)[128])rg_alloc(SNES_HEIGHT_EXTENDED * 128, MEM_SLOW);
    s7rPtr = (SPC7110Regs *)rg_alloc(sizeof(SPC7110Regs), MEM_SLOW);
    decompPtr = (SPC7110Decomp *)rg_alloc(sizeof(SPC7110Decomp), MEM_SLOW);
    CosTable2 = (double *)rg_alloc(sizeof(double) * 2048, MEM_SLOW);
    SinTable2 = (double *)rg_alloc(sizeof(double) * 2048, MEM_SLOW);
    SA1_DMABuffer = (uint8 *)rg_alloc(0x10000, MEM_SLOW);
#ifdef SDD1_DECOMP
    // S-DD1 decompression is only active for a handful of cartridges. Keep its
    // scratch buffer in PSRAM so the renderer can use the reclaimed internal RAM.
    SDD1Buffer = (uint8 *)rg_alloc(0x10000, MEM_SLOW);
#endif

    // Zero everything
    memset(SettingsPtr, 0, sizeof(struct SSettings));
    memset(CPUPtr, 0, sizeof(struct SCPUState));
    memset(PPUPtr, 0, sizeof(struct SPPU));
    memset(IPPUPtr, 0, sizeof(struct InternalPPU));
    memset(SoundDataPtr, 0, sizeof(SSoundData));

    memset(DMAPtr, 0, sizeof(struct SDMA) * 8);
    memset(SA1Ptr, 0, sizeof(struct SSA1));
    memset(rtcPtr, 0, sizeof(SRTC_DATA));
    memset(GFXPtr, 0, sizeof(struct SGFX));
    memset(LineData, 0, sizeof(struct SLineData) * 240);
    memset(LineMatrixData, 0, sizeof(struct SLineMatrixData) * 240);
    memset(CheatPtr, 0, sizeof(struct SCheatData));
    memset(Echo, 0, sizeof(int32) * 24000);
    memset(MixBuffer, 0, sizeof(int32) * SOUND_BUFFER_SIZE);
    memset(EchoBuffer, 0, sizeof(int32) * SOUND_BUFFER_SIZE);
    memset(DirectColourMapsPtr, 0, sizeof(uint16) * 8 * 256);
    memset(OBJOnLinePtr, 0, SNES_HEIGHT_EXTENDED * 128);
    memset(s7rPtr, 0, sizeof(SPC7110Regs));
    memset(decompPtr, 0, sizeof(SPC7110Decomp));
    memset(CosTable2, 0, sizeof(double) * 2048);
    memset(SinTable2, 0, sizeof(double) * 2048);
    memset(SA1_DMABuffer, 0, 0x10000);

    // GFX buffers
    GFXPtr->SubScreen = (uint8 *)rg_alloc(SNES_HEIGHT_EXTENDED * SNES_WIDTH * 2, MEM_SLOW);
    GFXPtr->ZBuffer = (uint8 *)rg_alloc(SNES_HEIGHT_EXTENDED * SNES_WIDTH, MEM_FAST);
    GFXPtr->SubZBuffer = (uint8 *)rg_alloc(SNES_HEIGHT_EXTENDED * SNES_WIDTH, MEM_SLOW);
    GFXPtr->Pitch = SNES_WIDTH * 2;

    // Reserve asynchronous display ownership before the ROM allocator consumes
    // the largest remaining PSRAM block. Graphics initialization also requires
    // a valid main screen when it calculates GFX.Delta.
    updates[0] = rg_surface_create(SNES_WIDTH, SNES_HEIGHT_EXTENDED, RG_PIXEL_565_LE, 0);
    updates[1] = rg_surface_create(SNES_WIDTH, SNES_HEIGHT_EXTENDED, RG_PIXEL_565_LE, 0);
    if (!updates[0] || !updates[1]) RG_PANIC("Display buffer allocation failed!");
    updates[0]->height = SNES_HEIGHT;
    updates[1]->height = SNES_HEIGHT;
    currentUpdate = updates[0];
    lastCompleteUpdate = NULL;
    GFXPtr->Screen = (uint8 *)currentUpdate->data;

    audioBuffers[0] = (rg_audio_sample_t *)calloc(AUDIO_BUFFER_LENGTH, sizeof(rg_audio_sample_t));
    audioBuffers[1] = (rg_audio_sample_t *)calloc(AUDIO_BUFFER_LENGTH, sizeof(rg_audio_sample_t));
    if (!audioBuffers[0] || !audioBuffers[1]) RG_PANIC("Audio buffer allocation failed!");
    currentAudioBuffer = audioBuffers[0];

    // Match the stock SNES9x integration: audio mixing and sink pacing run on
    // the second core instead of blocking the emulator hot path.
    audioTask = rg_task_create("pocketsnes_audio", &audio_task, NULL,
                               2048, AUDIO_TASK_QUEUE_DEPTH, RG_TASK_PRIORITY_6, 1);
    RG_ASSERT(audioTask, "Failed to create audio task!");

    const bool romIsZip = rg_extension_match(app->romPath, "zip");
    const size_t romFileSize = get_rom_file_size(app->romPath, romIsZip);
    if (!romFileSize || romFileSize > CMemory::MAX_ROM_SIZE + 0x200)
        RG_PANIC("Invalid or unsupported ROM size!");

    // Initialize core. CRITICAL: S9xInitAPU must come before Memory.Init to get Internal RAM.
    if (!S9xInitAPU()) RG_PANIC("APU init failed!");
    new (MemoryPtr) CMemory();
    if (!Memory.Init((uint32)romFileSize))
    {
        RG_LOGE("Not enough contiguous memory for ROM (%u bytes).\n", (unsigned)romFileSize);
        rg_gui_alert(_("Not enough memory"),
                     _("This ROM is too large for the available contiguous memory on this device."));
        rg_system_exit();
    }
    if (!S9xInitSound(7, TRUE, AUDIO_BUFFER_LENGTH * 4)) RG_PANIC("Sound init failed!");
    if (!S9xGraphicsInit()) RG_PANIC("Graphics init failed!");

    S9xSetPlaybackRate(AUDIO_SAMPLE_RATE);
    S9xSetSoundMute(FALSE);

    sound_enabled = rg_settings_get_number(NS_APP, SETTING_SOUND_EMULATION, 1);
    update_keymap(rg_settings_get_number(NS_APP, SETTING_KEYMAP, (rg_input_key_is_present(RG_KEY_X) ? KEYMAP_REGULAR : KEYMAP_TYPE_A)));

    Settings.JoystickEnabled = FALSE;
    Settings.SoundPlaybackRate = AUDIO_SAMPLE_RATE;
    Settings.CyclesPercentage = 100;
    Settings.APUEnabled = TRUE;
    Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
    Settings.SkipFrames = AUTO_FRAMERATE;
    // Keep idle-loop shutdown enabled on every target. ROM-specific compatibility
    // fixes can still disable Settings.Shutdown after the ROM has been identified.
    Settings.ShutdownMaster = TRUE;
    Settings.Shutdown = TRUE;
    Settings.FrameTimePAL = 20000;
    Settings.FrameTimeNTSC = 16667;
    Settings.FrameTime = Settings.FrameTimeNTSC;
    Settings.DisableMasterVolume = FALSE;
    Settings.DisableSoundEcho = rg_settings_get_number(NS_APP, "s9x_echo", 0);
    Settings.InterpolatedSound = rg_settings_get_number(NS_APP, "s9x_interp", 1);
    Settings.Transparency = rg_settings_get_number(NS_APP, "s9x_transp", 1);
    Settings.SupportHiRes = FALSE;
    Settings.AutoSaveDelay = 1;
    Settings.ApplyCheats = TRUE;
    Settings.SoundSync = 0;

    Settings.SuperFX = TRUE;
    Settings.DSP1Master = TRUE;
    Settings.SA1Main = TRUE;
    Settings.C4 = TRUE;
    Settings.SDD1 = TRUE;

    const char *romPath = app->romPath;

    if (romIsZip)
    {
        size_t unzip_size = Memory.ROM_AllocSize - 0x8000;
        if (!rg_storage_unzip_file(romPath, NULL, (void **)&Memory.ROM, &unzip_size, RG_FILE_USER_BUFFER))
        {
            RG_PANIC("ROM unzip failed!");
        }
        Memory.ROM_FileSize = unzip_size;
        romPath = NULL;
    }

    if (!Memory.LoadROM(romPath))
    {
        RG_PANIC("ROM load failed!");
    }

    S9xSetPlaybackRate(AUDIO_SAMPLE_RATE);
    S9xReset();
    S9xResetSound(1);
    S9xSetSoundMute(FALSE);

    char *sramPath = rg_emu_get_path(RG_PATH_SAVE_SRAM, app->romPath);
    Memory.LoadSRAM(sramPath);
    free(sramPath);

    if (app->bootFlags & RG_BOOT_RESUME)
        rg_emu_load_state(app->saveSlot);

    rg_system_set_tick_rate(Memory.ROMFramesPerSecond);
    app->frameskip = 0;

    const int samplesPerFrame = (int)roundf((float)app->sampleRate / app->tickRate);
    int skipFrames = 0;
    RG_LOGI("Starting main loop...\n");

    while (1)
    {
        const int64_t startTime = rg_system_timer();
        const uint32_t rawJoystick = rg_input_read_gamepad();
        suppressedKeys &= rawJoystick;
        gamepadState = rawJoystick & ~suppressedKeys;
        const uint32_t joystick = gamepadState;
        bool drawFrame = (skipFrames == 0);

        if (joystick & (RG_KEY_MENU | RG_KEY_OPTION))
        {
            pause_audio_for_menu(samplesPerFrame);
            save_sram_if_dirty();
            if (joystick & RG_KEY_MENU) rg_gui_game_menu();
            else rg_gui_options_menu();
            suppressedKeys = rg_input_read_gamepad();
            gamepadState = 0;
            continue;
        }

        IPPU.RenderThisFrame = drawFrame;
        GFX.Screen = (uint8*)currentUpdate->data;

        GFX.RealPitch = GFX.Pitch = SNES_WIDTH * 2;
        GFX.PPL = SNES_WIDTH;
        GFX.PPLx2 = SNES_WIDTH * 2;
        GFX.ZPitch = SNES_WIDTH;

        IAPU.APUExecuting = Settings.APUEnabled;

        S9xMainLoop();

        if (drawFrame)
        {
            rg_display_submit(currentUpdate, 0);
            lastCompleteUpdate = currentUpdate;
            currentUpdate = updates[currentUpdate == updates[0]];
        }

        // Audio submission paces emulation and may block waiting for the sink.
        // Report only actual emulation work to Retro-Go's load monitor.
        const unsigned int busyTime = (unsigned int)(rg_system_timer() - startTime);
        rg_system_tick(busyTime);

        rg_task_msg_t audioMsg = {
            .type = (int)sound_enabled,
            .dataInt = (uint32_t)samplesPerFrame,
        };
        rg_task_send(audioTask, &audioMsg, -1);

        if (skipFrames == 0)
        {
            if (app->frameskip > 0) skipFrames = app->frameskip;
            else if (busyTime > app->frameTime + 1000) skipFrames = 1;
        }
        else skipFrames--;
    }
}
