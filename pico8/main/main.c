// pico8/main/main.c
//
// Entry point for the Pico-8 retro-go app. app_main() boots retro-go
// (mounting SD card, reading settings/LCD/joystick drivers), then enables
// the backend (init_video/audio/platform), and finally reads the .p8 cart
// from `app->romPath`, parses it, hands the result to cartParser + init_lua.

#include <rg_system.h>
#include <rg_display.h>
#include <rg_input.h>
#include <rg_audio.h>
#include <rg_storage.h>
#include <rg_gui.h>
#include <rg_settings.h>
#include <rg_utils.h>
#include <miniz.h>

#include "backend.h"
#include "data.h"
#include "p8_text_parser.h"
#include "p8_png.h"
#include "pico8_globals.h"

#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

// Forward decls from engine inside the pico8 component (unity-built in
// src/engine.c).
extern void engine_init(void);
extern void engine_prepare_cart_load(void);
extern int engine_take_cart_request(char *filename, size_t filename_size,
                                    char *breadcrumb, size_t breadcrumb_size,
                                    char *param, size_t param_size);
extern void engine_set_cart_param(const char *param);
extern bool engine_request_restart(const char *param);
extern bool engine_cart_request_pending(void);
extern bool init_lua(const uint8_t *bytecode, uint16_t code_len);
extern void cartParser(const GameCart *cart);
extern void flip(bool draw_frame);
extern int engine_frame_rate(void);
extern void engine_set_frame_stats(int busy_us, int tick_rate);
extern void engine_collect_garbage(void);
extern const char *engine_menuitem_label(int slot);
extern bool engine_menuitem_invoke(int slot, uint8_t buttons);

// Forward decl for the pico8_run_task — defined later in the file.
// Lifted out of app_main() so flip() can run on a dedicated FreeRTOS
// task with a stack generous enough for carts that nest many Lua→C
// frames per tick (see the rg_task_create() call in app_main). The
// task body is the same frame loop that previously ran inline on the
// FreeRTOS main task inside app_main().
static void pico8_run_task(void *arg);

static rg_app_t *app = NULL;
static atomic_bool low_memory_gc_pending = ATOMIC_VAR_INIT(false);

enum {
    P8_CART_REQUEST_NONE = 0,
    P8_CART_REQUEST_LOAD = 1,
    P8_CART_REQUEST_BREADCRUMB = 2,
    P8_CART_REQUEST_RUN = 3,
};

#define P8_BREADCRUMB_DEPTH 16
typedef struct {
    char path[RG_PATH_MAX + 1];
    char param[256];
} P8Breadcrumb;

static char current_cart_path[RG_PATH_MAX + 1];
static char current_cart_param[256];
static P8Breadcrumb *cart_breadcrumbs = NULL;
static int cart_breadcrumb_depth = 0;

// ----------------------------- Save states -------------------------------
// A useful PICO-8 state must include the live Lua object graph: carts commonly
// keep entities and timers in top-level locals captured by closures rather
// than in globals or machine RAM. Lua 5.2 has no heap snapshot facility, and
// raw allocator pointers are not portable across a cold boot. Do not create a
// partial RAM-only file and report success; explain the limitation through the
// shared menu until a graph/upvalue/coroutine serializer exists.
static bool load_state_cb(const char *filename)
{
    (void)filename;
    rg_gui_alert("Save states unavailable",
                 "Live PICO-8 Lua state cannot be restored safely.\n"
                 "Use the game's own save/checkpoint support.");
    return false;
}

static bool save_state_cb(const char *filename)
{
    (void)filename;
    rg_gui_alert("Save states unavailable",
                 "Live PICO-8 Lua state cannot be saved safely.\n"
                 "Use the game's own save/checkpoint support.");
    return false;
}

static bool reset_cb(bool hard)
{
    // PICO-8 has one cart restart operation. Both Retro-Go reset choices
    // therefore perform the same clean VM teardown/reload while preserving
    // cartdata and the active multicart parameter.
    (void)hard;
    if (!cartdata_flush()) return false;
    return engine_request_restart(current_cart_param);
}

static bool screenshot_cb(const char *filename, int width, int height)
{
    return gfx_screenshot(filename, width, height);
}

static void event_cb(int event, void *data)
{
    (void)data;
    if (event == RG_EVENT_REDRAW) {
        gfx_redraw();
    } else if (event == RG_EVENT_LOWMEMORY) {
        // Events can arrive from a Retro-Go service task. Lua itself is not
        // thread-safe, so defer the collection to the cart task instead of
        // entering the VM from this callback.
        atomic_store_explicit(&low_memory_gc_pending, true,
                              memory_order_release);
    } else if (event == RG_EVENT_SHUTDOWN || event == RG_EVENT_SLEEP) {
        cartdata_flush();
        audio_task_stop();
    }
}

static rg_gui_event_t menuitem_cb(rg_gui_option_t *option,
                                  rg_gui_event_t event)
{
    uint8_t buttons = 0;
    if (event == RG_DIALOG_PREV) buttons = 1;       // PICO-8 menu L
    else if (event == RG_DIALOG_NEXT) buttons = 2;  // PICO-8 menu R
    else if (event == RG_DIALOG_ENTER) buttons = 32; // PICO-8 menu X
    else return RG_DIALOG_VOID;

    bool keep_open = engine_menuitem_invoke((int)option->arg, buttons);
    const char *label = engine_menuitem_label((int)option->arg);
    if (label) option->label = label;
    else option->flags = RG_DIALOG_FLAG_HIDDEN;
    return keep_open ? RG_DIALOG_REDRAW : RG_DIALOG_CANCEL;
}

static void options_cb(rg_gui_option_t *dest)
{
    int out = 0;
    for (int slot = 0; slot < 5; ++slot) {
        const char *label = engine_menuitem_label(slot);
        if (!label) continue;
        dest[out++] = (rg_gui_option_t){slot, label, NULL,
                                        RG_DIALOG_FLAG_NORMAL, menuitem_cb};
    }
    dest[out] = (rg_gui_option_t)RG_DIALOG_END;
}
static void about_cb(rg_gui_option_t *dest)
{
    static char cart_name[96];
    static char tick_rate[16];
    const char *name = strrchr(current_cart_path, '/');
    name = name ? name + 1 : current_cart_path;
    snprintf(cart_name, sizeof(cart_name), "%.95s",
             (name && name[0]) ? name : "-");
    snprintf(tick_rate, sizeof(tick_rate), "%d Hz", engine_frame_rate());

    *dest++ = (rg_gui_option_t){0, "Engine", "PicoPico / z8lua",
                                RG_DIALOG_FLAG_NORMAL, NULL};
    *dest++ = (rg_gui_option_t){0, "Cartridge", cart_name,
                                RG_DIALOG_FLAG_NORMAL, NULL};
    *dest++ = (rg_gui_option_t){0, "Tick rate", tick_rate,
                                RG_DIALOG_FLAG_NORMAL, NULL};
    *dest++ = (rg_gui_option_t){0, "Formats", ".p8 / .p8.png / .zip",
                                RG_DIALOG_FLAG_NORMAL, NULL};
    *dest++ = (rg_gui_option_t){0, "Save states", "Unavailable",
                                RG_DIALOG_FLAG_NORMAL, NULL};
    *dest++ = (rg_gui_option_t)RG_DIALOG_END;
}

// ----------------------------- Cart loading ------------------------------

// PICO-8 accepts extensionless cartridge names. Offline releases may store
// the same cart as `part`, `part.p8`, or `part.p8.png`, so resolve those forms
// before tearing down the currently running cart. Exact names always win.
static bool resolve_existing_cart_path(const char *requested_path,
                                       char *resolved_path,
                                       size_t resolved_size)
{
    if (!requested_path || !requested_path[0] ||
        !resolved_path || resolved_size == 0)
        return false;

    size_t len = strlen(requested_path);
    if (len >= resolved_size) return false;
    memcpy(resolved_path, requested_path, len + 1);
    if (rg_storage_exists(resolved_path)) return true;

    // A conventional .p8 request may refer to a downloaded PNG cart.
    if (len >= 3 && strcmp(requested_path + len - 3, ".p8") == 0) {
        if (len + 4 >= resolved_size) return false;
        memcpy(resolved_path + len, ".png", 5);
        return rg_storage_exists(resolved_path);
    }

    // Already-complete PNG names have no further useful fallback.
    if (len >= 7 && strcmp(requested_path + len - 7, ".p8.png") == 0)
        return false;

    // Extensionless load("part") first means part.p8, then part.p8.png.
    if (len + 3 < resolved_size) {
        memcpy(resolved_path + len, ".p8", 4);
        if (rg_storage_exists(resolved_path)) return true;
    }

    if (len + 7 < resolved_size) {
        memcpy(resolved_path + len, ".p8.png", 8);
        if (rg_storage_exists(resolved_path)) return true;
    }
    return false;
}

typedef struct {
    const char *current_path;
    char candidate[RG_PATH_MAX + 1];
    int count;
} UniqueSiblingCart;

static bool cart_path_has_loadable_extension(const char *path)
{
    size_t len = path ? strlen(path) : 0;
    return (len >= 3 && strcmp(path + len - 3, ".p8") == 0) ||
           (len >= 7 && strcmp(path + len - 7, ".p8.png") == 0);
}

static int find_unique_sibling_cart_cb(const rg_scandir_t *file, void *arg)
{
    UniqueSiblingCart *search = (UniqueSiblingCart *)arg;
    if (!file->is_file || !cart_path_has_loadable_extension(file->path) ||
        strcmp(file->path, search->current_path) == 0)
        return RG_SCANDIR_CONTINUE;

    if (search->count == 0)
        snprintf(search->candidate, sizeof(search->candidate), "%s",
                 file->path);
    search->count++;
    return RG_SCANDIR_CONTINUE;
}

// Offline downloads do not always retain the BBS identifier used by
// load("#id"). If a package folder contains exactly one other cartridge,
// that sibling is unambiguous (Little Eidolon's #littleeidolons_data_1 is
// distributed as eido_data.p8.png). Remain conservative when two or more
// candidates exist rather than launching an arbitrary cart.
static bool resolve_unique_sibling_cart(char *resolved_path,
                                        size_t resolved_size)
{
    const char *slash = strrchr(current_cart_path, '/');
    if (!slash) return false;

    size_t dir_len = (size_t)(slash - current_cart_path);
    char directory[RG_PATH_MAX + 1];
    if (dir_len == 0 || dir_len >= sizeof(directory)) return false;
    memcpy(directory, current_cart_path, dir_len);
    directory[dir_len] = '\0';

    UniqueSiblingCart search = {.current_path = current_cart_path};
    if (!rg_storage_scandir(directory, find_unique_sibling_cart_cb, &search,
                            RG_SCANDIR_FILES) || search.count != 1)
        return false;

    size_t candidate_len = strlen(search.candidate);
    if (candidate_len >= resolved_size) return false;
    memcpy(resolved_path, search.candidate, candidate_len + 1);
    return true;
}

static void show_missing_cart_alert(const char *filename)
{
    char message[384];
    snprintf(message, sizeof(message),
             "Missing %s\n\nPlace it in the same folder as the main cart.",
             (filename && filename[0]) ? filename : "companion cart");

    // Keep the failed cart's music from playing behind the blocking dialog.
    // Restore the global mute state before rg_system_exit() hands control to
    // the launcher.
    rg_audio_set_mute(true);
    rg_gui_alert("Missing PICO-8 cart", message);
    rg_audio_set_mute(false);
}

// PICO-8 expands line-oriented `#include file.lua` directives before handing
// source to its Lua compiler. Standalone multicarts commonly keep a small .p8
// bootstrap beside shared .lua files (Poom 1.9 does this for both its title
// and gameplay carts). Expand only safe cart-relative Lua files and cap the
// result at the VM's 16-bit source length. Ordinary carts never allocate this
// buffer and there is no per-frame cost.
#define P8_LUA_SOURCE_MAX 65535u
#define P8_INCLUDE_DEPTH_MAX 8

typedef struct {
    uint8_t *data;
    size_t length;
} P8LuaSource;

// Text .p8/.lua files store PICO-8's upper character set as UTF-8. Convert
// those glyphs back to their single-byte P8SCII values while flattening an
// include. This is important for generated byte strings (Poom's title art)
// as well as keeping the logical 65535-character source limit distinct from
// the larger UTF-8 file size. The mapping follows the PICO-8 character set;
// the compact kana tables avoid a large string/regex implementation.
static uint32_t decode_utf8(const uint8_t *source, size_t length,
                            size_t *consumed)
{
    uint8_t c = source[0];
    *consumed = 1;
    if (c < 0x80) return c;
    if (c >= 0xc2 && c <= 0xdf && length >= 2 &&
        (source[1] & 0xc0) == 0x80) {
        *consumed = 2;
        return ((uint32_t)(c & 0x1f) << 6) | (source[1] & 0x3f);
    }
    if (c >= 0xe0 && c <= 0xef && length >= 3 &&
        (source[1] & 0xc0) == 0x80 && (source[2] & 0xc0) == 0x80) {
        uint32_t value = ((uint32_t)(c & 0x0f) << 12) |
                         ((uint32_t)(source[1] & 0x3f) << 6) |
                         (source[2] & 0x3f);
        if (value >= 0x800 && !(value >= 0xd800 && value <= 0xdfff)) {
            *consumed = 3;
            return value;
        }
    }
    if (c >= 0xf0 && c <= 0xf4 && length >= 4 &&
        (source[1] & 0xc0) == 0x80 && (source[2] & 0xc0) == 0x80 &&
        (source[3] & 0xc0) == 0x80) {
        uint32_t value = ((uint32_t)(c & 0x07) << 18) |
                         ((uint32_t)(source[1] & 0x3f) << 12) |
                         ((uint32_t)(source[2] & 0x3f) << 6) |
                         (source[3] & 0x3f);
        if (value >= 0x10000 && value <= 0x10ffff) {
            *consumed = 4;
            return value;
        }
    }
    return c;
}

static int unicode_to_p8scii(uint32_t value)
{
    static const uint16_t hiragana[50] = {
        0x3042, 0x3044, 0x3046, 0x3048, 0x304a,
        0x304b, 0x304d, 0x304f, 0x3051, 0x3053,
        0x3055, 0x3057, 0x3059, 0x305b, 0x305d,
        0x305f, 0x3061, 0x3064, 0x3066, 0x3068,
        0x306a, 0x306b, 0x306c, 0x306d, 0x306e,
        0x306f, 0x3072, 0x3075, 0x3078, 0x307b,
        0x307e, 0x307f, 0x3080, 0x3081, 0x3082,
        0x3084, 0x3086, 0x3088, 0x3089, 0x308a,
        0x308b, 0x308c, 0x308d, 0x308f, 0x3092,
        0x3093, 0x3063, 0x3083, 0x3085, 0x3087,
    };
    static const uint16_t katakana[50] = {
        0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa,
        0x30ab, 0x30ad, 0x30af, 0x30b1, 0x30b3,
        0x30b5, 0x30b7, 0x30b9, 0x30bb, 0x30bd,
        0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8,
        0x30ca, 0x30cb, 0x30cc, 0x30cd, 0x30ce,
        0x30cf, 0x30d2, 0x30d5, 0x30d8, 0x30db,
        0x30de, 0x30df, 0x30e0, 0x30e1, 0x30e2,
        0x30e4, 0x30e6, 0x30e8, 0x30e9, 0x30ea,
        0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f2,
        0x30f3, 0x30c3, 0x30e3, 0x30e5, 0x30e7,
    };

    for (int i = 0; i < 50; ++i) {
        if (value == hiragana[i]) return 154 + i;
        if (value == katakana[i]) return 204 + i;
    }

    switch (value) {
        case 0x00b9: return 1;   case 0x00b2: return 2;
        case 0x00b3: return 3;   case 0x2074: return 4;
        case 0x2075: return 5;   case 0x2076: return 6;
        case 0x2077: return 7;   case 0x2078: return 8;
        case 0x1d47: return 11;  case 0x1d9c: return 12;
        case 0x1d49: return 14;  case 0x1da0: return 15;
        case 0x25ae: return 16;  case 0x25a0: return 17;
        case 0x25a1: return 18;  case 0x2059: return 19;
        case 0x2058: return 20;  case 0x2016: return 21;
        case 0x25c0: return 22;  case 0x25b6: return 23;
        case 0x300c: return 24;  case 0x300d: return 25;
        case 0x00a5: return 26;  case 0x2022: return 27;
        case 0x3001: return 28;  case 0x3002: return 29;
        case 0x309b: return 30;  case 0x309c: return 31;
        case 0x25cb: return 127; case 0x2588: return 128;
        case 0x2592: return 129; case 0x1f431: return 130;
        case 0x2b07: return 131; case 0x2591: return 132;
        case 0x273d: return 133; case 0x25cf: return 134;
        case 0x2665: return 135; case 0x2609: return 136;
        case 0xc6c3: return 137; case 0x2302: return 138;
        case 0x2b05: return 139; case 0x1f610: return 140;
        case 0x266a: return 141; case 0x1f17e: return 142;
        case 0x25c6: return 143; case 0x2026: return 144;
        case 0x27a1: return 145; case 0x2605: return 146;
        case 0x29d7: return 147; case 0x2b06: return 148;
        case 0x02c7: return 149; case 0x2227: return 150;
        case 0x274e: return 151; case 0x25a4: return 152;
        case 0x25a5: return 153; case 0x25dc: return 254;
        case 0x25dd: return 255; default: return -1;
    }
}

static bool append_p8scii_source(P8LuaSource *output,
                                 const uint8_t *data, size_t length)
{
    size_t pos = 0;
    while (pos < length) {
        size_t consumed = 1;
        uint32_t value = decode_utf8(data + pos, length - pos, &consumed);
        int p8 = value < 0x80 ? (int)value : unicode_to_p8scii(value);
        if (p8 >= 0) {
            if (output->length >= P8_LUA_SOURCE_MAX) return false;
            output->data[output->length++] = (uint8_t)p8;
            pos += consumed;
            // PICO-8 emoji glyphs may carry Unicode variation selector 16.
            if (pos + 3 <= length && data[pos] == 0xef &&
                data[pos + 1] == 0xb8 && data[pos + 2] == 0x8f)
                pos += 3;
        } else {
            if (consumed > P8_LUA_SOURCE_MAX - output->length) return false;
            memcpy(output->data + output->length, data + pos, consumed);
            output->length += consumed;
            pos += consumed;
        }
    }
    return true;
}

static bool lua_source_has_include(const uint8_t *source, size_t length)
{
    size_t pos = 0;
    while (pos < length) {
        size_t end = pos;
        while (end < length && source[end] != '\n') ++end;
        size_t text = pos;
        while (text < end && (source[text] == ' ' || source[text] == '\t'))
            ++text;
        if (end - text >= 8 && memcmp(source + text, "#include", 8) == 0 &&
            (text + 8 == end || source[text + 8] == ' ' ||
             source[text + 8] == '\t'))
            return true;
        pos = end < length ? end + 1 : end;
    }
    return false;
}

static bool append_lua_source(P8LuaSource *output,
                              const uint8_t *data, size_t length)
{
    if (!append_p8scii_source(output, data, length)) {
        RG_LOGE("pico8: expanded Lua source exceeds %u bytes",
                (unsigned)P8_LUA_SOURCE_MAX);
        return false;
    }
    return true;
}

static bool resolve_include_path(const char *including_path,
                                 const uint8_t *name, size_t name_length,
                                 char *path, size_t path_size)
{
    if (!including_path || !name || name_length == 0 ||
        name_length >= 256 || name[0] == '/' || name[0] == '\\')
        return false;

    char filename[256];
    memcpy(filename, name, name_length);
    filename[name_length] = '\0';
    if (strchr(filename, '\\') || strchr(filename, ':') ||
        strstr(filename, ".."))
        return false;

    // Including another .p8 tab has different section semantics. Keep this
    // initial implementation to the source-file form used by Poom.
    const char *extension = strrchr(filename, '.');
    if (!extension || strcmp(extension, ".lua") != 0) {
        RG_LOGE("pico8: unsupported include type: %s", filename);
        return false;
    }

    const char *slash = strrchr(including_path, '/');
    size_t dir_length = slash ? (size_t)(slash - including_path + 1) : 0;
    if (dir_length + name_length >= path_size) return false;
    memcpy(path, including_path, dir_length);
    memcpy(path + dir_length, filename, name_length + 1);
    return true;
}

static bool expand_lua_source_recursive(const uint8_t *source, size_t length,
                                        const char *source_path,
                                        P8LuaSource *output, int depth)
{
    if (depth > P8_INCLUDE_DEPTH_MAX) {
        RG_LOGE("pico8: #include nesting exceeds %d levels",
                P8_INCLUDE_DEPTH_MAX);
        return false;
    }

    size_t pos = 0;
    while (pos < length) {
        size_t line_end = pos;
        while (line_end < length && source[line_end] != '\n') ++line_end;
        size_t content_end = line_end;
        if (content_end > pos && source[content_end - 1] == '\r')
            --content_end;

        size_t text = pos;
        while (text < content_end &&
               (source[text] == ' ' || source[text] == '\t'))
            ++text;

        bool is_include = content_end - text >= 8 &&
                          memcmp(source + text, "#include", 8) == 0 &&
                          (text + 8 == content_end || source[text + 8] == ' ' ||
                           source[text + 8] == '\t');
        if (!is_include) {
            size_t line_length = line_end - pos;
            if (line_end < length) ++line_length;
            if (!append_lua_source(output, source + pos, line_length))
                return false;
            pos = line_end < length ? line_end + 1 : line_end;
            continue;
        }

        size_t name_start = text + 8;
        while (name_start < content_end &&
               (source[name_start] == ' ' || source[name_start] == '\t'))
            ++name_start;
        size_t name_end = name_start;
        if (name_start < content_end && source[name_start] == '"') {
            ++name_start;
            name_end = name_start;
            while (name_end < content_end && source[name_end] != '"')
                ++name_end;
            if (name_end == content_end) {
                RG_LOGE("pico8: unterminated quoted #include in %s", source_path);
                return false;
            }
        } else {
            while (name_end < content_end && source[name_end] != ' ' &&
                   source[name_end] != '\t')
                ++name_end;
        }

        char include_path[RG_PATH_MAX + 1];
        if (!resolve_include_path(source_path, source + name_start,
                                  name_end - name_start, include_path,
                                  sizeof(include_path))) {
            RG_LOGE("pico8: invalid #include in %s", source_path);
            return false;
        }

        uint8_t *included_data = NULL;
        size_t included_length = 0;
        if (!rg_storage_read_file(include_path, (void **)&included_data,
                                  &included_length, 0)) {
            const char *missing = strrchr(include_path, '/');
            RG_LOGE("pico8: included Lua file not found: %s", include_path);
            show_missing_cart_alert(missing ? missing + 1 : include_path);
            return false;
        }
        RG_LOGI("pico8: expanding #include %s (%u bytes)", include_path,
                (unsigned)included_length);

        bool expanded = expand_lua_source_recursive(
            included_data, included_length, include_path, output, depth + 1);
        bool has_final_newline = included_length > 0 &&
                                 included_data[included_length - 1] == '\n';
        free(included_data);
        if (!expanded) return false;
        if (!has_final_newline && !append_lua_source(
                output, (const uint8_t *)"\n", 1))
            return false;

        pos = line_end < length ? line_end + 1 : line_end;
    }
    return true;
}

static bool expand_lua_includes(const uint8_t *source, size_t length,
                                const char *source_path,
                                bool normalize_p8scii,
                                uint8_t **expanded, uint16_t *expanded_length)
{
    *expanded = NULL;
    *expanded_length = (uint16_t)length;
    if (!normalize_p8scii && !lua_source_has_include(source, length))
        return true;

    uint8_t *buffer = (uint8_t *)rg_alloc(P8_LUA_SOURCE_MAX + 1, MEM_SLOW);
    if (!buffer) {
        RG_LOGE("pico8: unable to allocate #include expansion buffer");
        return false;
    }

    P8LuaSource output = {buffer, 0};
    if (!expand_lua_source_recursive(source, length, source_path, &output, 0)) {
        free(buffer);
        return false;
    }
    buffer[output.length] = '\0';
    *expanded = buffer;
    *expanded_length = (uint16_t)output.length;
    RG_LOGI("pico8: expanded Lua source from %u to %u bytes",
            (unsigned)length, (unsigned)output.length);
    return true;
}

// ZIP support is deliberately limited to one cartridge entry. Other archive
// members are ignored, but multiple carts are ambiguous because multicart
// packages have no standard entry filename. Parse the central directory here
// because Retro-Go's current unzip helper ignores its filter argument.
static uint16_t zip_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t zip_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool zip_name_is_cart(const uint8_t *name, size_t length)
{
    if (!name || !length || name[length - 1] == '/') return false;
    char tail[8];
    size_t count = length < sizeof(tail) - 1 ? length : sizeof(tail) - 1;
    for (size_t i = 0; i < count; ++i) {
        uint8_t c = name[length - count + i];
        tail[i] = (char)((c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c);
    }
    tail[count] = '\0';
    return (count >= 3 && strcmp(tail + count - 3, ".p8") == 0) ||
           (count >= 7 && strcmp(tail + count - 7, ".p8.png") == 0);
}

static bool load_single_cart_zip(const char *path, uint8_t **data_out,
                                 size_t *size_out, char *name_out,
                                 size_t name_out_size)
{
    uint8_t *zip = NULL;
    size_t zip_size = 0;
    if (!rg_storage_read_file(path, (void **)&zip, &zip_size, 0)) return false;

    size_t search_start = zip_size > 65557 ? zip_size - 65557 : 0;
    size_t eocd = SIZE_MAX;
    if (zip_size >= 22) {
        for (size_t pos = zip_size - 22;; --pos) {
            if (zip_u32(zip + pos) == 0x06054b50u) {
                eocd = pos;
                break;
            }
            if (pos == search_start) break;
        }
    }
    if (eocd == SIZE_MAX || eocd + 22 > zip_size) {
        RG_LOGE("pico8: ZIP central directory not found: %s", path);
        free(zip);
        return false;
    }

    uint16_t entries = zip_u16(zip + eocd + 10);
    uint32_t central_size = zip_u32(zip + eocd + 12);
    uint32_t central_offset = zip_u32(zip + eocd + 16);
    if ((uint64_t)central_offset + central_size > zip_size) {
        RG_LOGE("pico8: invalid ZIP central directory: %s", path);
        free(zip);
        return false;
    }

    const uint8_t *chosen = NULL;
    const uint8_t *chosen_name = NULL;
    uint16_t chosen_name_len = 0;
    int cart_count = 0;
    size_t pos = central_offset;
    for (uint16_t i = 0; i < entries; ++i) {
        if (pos + 46 > zip_size || zip_u32(zip + pos) != 0x02014b50u) {
            RG_LOGE("pico8: invalid ZIP entry table: %s", path);
            free(zip);
            return false;
        }
        uint16_t name_len = zip_u16(zip + pos + 28);
        uint16_t extra_len = zip_u16(zip + pos + 30);
        uint16_t comment_len = zip_u16(zip + pos + 32);
        size_t next = pos + 46u + name_len + extra_len + comment_len;
        if (next > zip_size) {
            free(zip);
            return false;
        }
        if (zip_name_is_cart(zip + pos + 46, name_len)) {
            cart_count++;
            chosen = zip + pos;
            chosen_name = zip + pos + 46;
            chosen_name_len = name_len;
        }
        pos = next;
    }

    if (cart_count != 1 || !chosen) {
        char message[192];
        if (cart_count == 0) {
            snprintf(message, sizeof(message),
                     "No .p8 or .p8.png cartridge was found.");
        } else {
            snprintf(message, sizeof(message),
                     "This ZIP contains %d cartridges.\n\n"
                     "Extract multicart games\n"
                     "to their own folder.",
                     cart_count);
        }
        rg_gui_alert("Unsupported PICO-8 ZIP", message);
        RG_LOGE("pico8: ZIP requires exactly one cart, found %d: %s",
                cart_count, path);
        free(zip);
        return false;
    }

    uint16_t flags = zip_u16(chosen + 8);
    uint16_t method = zip_u16(chosen + 10);
    uint32_t compressed_size = zip_u32(chosen + 20);
    uint32_t output_size = zip_u32(chosen + 24);
    uint32_t local_offset = zip_u32(chosen + 42);
    if ((flags & 1) || (method != 0 && method != 8) || output_size == 0 ||
        output_size > 2u * 1024u * 1024u || local_offset + 30u > zip_size ||
        zip_u32(zip + local_offset) != 0x04034b50u) {
        RG_LOGE("pico8: unsupported ZIP cartridge entry: %s", path);
        free(zip);
        return false;
    }
    uint16_t local_name_len = zip_u16(zip + local_offset + 26);
    uint16_t local_extra_len = zip_u16(zip + local_offset + 28);
    size_t stream = (size_t)local_offset + 30u + local_name_len + local_extra_len;
    if (stream + compressed_size > zip_size) {
        free(zip);
        return false;
    }

    uint8_t *output = (uint8_t *)malloc(output_size);
    if (!output) {
        free(zip);
        return false;
    }
    bool ok = false;
    if (method == 0) {
        if (compressed_size == output_size) {
            memcpy(output, zip + stream, output_size);
            ok = true;
        }
    } else {
        size_t produced = tinfl_decompress_mem_to_mem(
            output, output_size, zip + stream, compressed_size,
            TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
        ok = produced == output_size;
    }

    if (ok && name_out && name_out_size) {
        const uint8_t *base = chosen_name;
        size_t base_len = chosen_name_len;
        for (size_t i = 0; i < chosen_name_len; ++i) {
            if (chosen_name[i] == '/' || chosen_name[i] == '\\') {
                base = chosen_name + i + 1;
                base_len = chosen_name_len - i - 1;
            }
        }
        size_t copy = base_len < name_out_size - 1 ? base_len : name_out_size - 1;
        memcpy(name_out, base, copy);
        name_out[copy] = '\0';
    }
    free(zip);
    if (!ok) {
        RG_LOGE("pico8: ZIP cartridge decompression failed: %s", path);
        free(output);
        return false;
    }
    *data_out = output;
    *size_out = output_size;
    RG_LOGI("pico8: loaded ZIP cartridge %s (%u bytes)",
            name_out && name_out[0] ? name_out : "(unnamed)",
            (unsigned)output_size);
    return true;
}

// Read `app->romPath` into a buffer, parse it as a .p8 text cart, then
// cartParser + init_lua the result. Frees the buffer and parsed blobs on the
// way out so rerunning with a new ROM is well-behaved.
static bool load_cart_from_path(const char *requested_path, const char *param,
                                char *loaded_path, size_t loaded_path_size)
{
    if (!requested_path || !requested_path[0]) return false;

    uint8_t *rom_data = NULL;
    size_t   rom_size = 0;
    char archive_name[256] = {0};
    char resolved_path[RG_PATH_MAX + 1];
    if (!resolve_existing_cart_path(requested_path, resolved_path,
                                    sizeof(resolved_path))) {
        RG_LOGE("pico8: cart file not found: %s", requested_path);
        return false;
    }
    if (strcmp(requested_path, resolved_path) != 0) {
        RG_LOGI("pico8: resolved cart %s as %s",
                requested_path, resolved_path);
    }
    bool zipped = rg_extension_match(resolved_path, "zip");
    bool read_ok = zipped
        ? load_single_cart_zip(resolved_path, &rom_data, &rom_size,
                               archive_name, sizeof(archive_name))
        : rg_storage_read_file(resolved_path, (void **)&rom_data,
                               &rom_size, 0);
    if (!read_ok) {
        RG_LOGE("pico8: unable to read cart file: %s", resolved_path);
        return false;
    }
    RG_LOGI("pico8: loaded %u bytes from %s",
            (unsigned)rom_size, resolved_path);
    if (loaded_path && loaded_path_size)
        snprintf(loaded_path, loaded_path_size, "%s", resolved_path);

    LoadedCart cart = {0};
    const char *basename = archive_name[0] ? archive_name
                                           : strrchr(resolved_path, '/');
    if (!archive_name[0]) basename = basename ? basename + 1 : resolved_path;

    // Route PNG carts through p8_png before falling back to the text
    // parser. p8_png_extract_text also returns the exact packed ROM image;
    // preserving it is essential for carts that hide compressed data inside
    // gfx memory and access it later with peek() (notably Celeste 2).
    const char *parse_ptr = (const char *)rom_data;
    size_t      parse_len = rom_size;
    char       *png_text  = NULL;
    uint8_t    *png_rom   = NULL;
    size_t      png_rom_len = 0;
    bool        png_routed = false;
    if (p8_png_is_png(rom_data, rom_size)) {
        png_routed = true;
        if (p8_png_extract_text(rom_data, rom_size, &png_text, &parse_len,
                                &png_rom, &png_rom_len)) {
            parse_ptr = png_text;
        } else {
            // Decode failed. Force the
            // ``p8_parse_text failed`` branch below by clearing parse
            // pointer so the loader logs a clean failure for PNG input
            // too. We still want the user's filename to be visible in
            // the log, so we keep this instead of an early return.
            parse_ptr = NULL;
            parse_len = 0;
        }
    }

    bool parsed = (parse_ptr != NULL)
                  && p8_parse_text(parse_ptr, parse_len, basename, &cart);
    if (parsed && png_rom) {
        if (!p8_cart_take_rom(&cart, png_rom, png_rom_len)) {
            free(png_rom);
            png_rom = NULL;
            parsed = false;
        } else {
            png_rom = NULL; // ownership transferred to LoadedCart
        }
    }
    bool running = false;
    if (!parsed) {
        if (png_routed) {
            RG_LOGE("pico8: .p8.png decode failed: %s", resolved_path);
        } else {
            RG_LOGE("pico8: p8_parse_text failed for %s", resolved_path);
        }
    } else {
        init_platform();                          // sets bootup_time
        cartParser(&cart.cart);                   // copies gfx/gff/sfx/map into engine
        engine_set_cart_param(param);
        uint8_t *expanded_code = NULL;
        uint16_t expanded_length = cart.cart.code_len;
        bool includes_ok = expand_lua_includes(
            cart.cart.code, cart.cart.code_len, resolved_path,
            !png_routed,
            &expanded_code, &expanded_length);
        const uint8_t *lua_code = expanded_code ? expanded_code : cart.cart.code;
        if (!includes_ok) {
            RG_LOGE("pico8: #include expansion failed for %s", basename);
        } else if (!init_lua(lua_code, expanded_length)) {
            RG_LOGE("pico8: init_lua failed for %s", basename);
        } else {
            RG_LOGI("pico8: Lua VM running cart \"%s\"", basename);
            running = true;
        }
        free(expanded_code);
    }
    free(png_rom);
    free(png_text);
    p8_cart_free(&cart);
    free(rom_data);
    return running;
}

static bool resolve_relative_cart_path(const char *filename,
                                       char *path, size_t path_size)
{
    if (!filename || !filename[0]) {
        RG_LOGE("pico8: refusing non-local cart path: %s",
                filename ? filename : "(null)");
        return false;
    }

    // BBS multicarts use load("#cart_id") for another published cart. This
    // offline port deliberately does not fetch from the network; resolve a
    // simple BBS id to a same-directory cart instead. Both cart_id.p8 and
    // cart_id.p8.png are accepted by the normal loader fallback.
    char bbs_name[260];
    const char *local_name = filename;
    if (filename[0] == '#') {
        const char *id = filename + 1;
        if (!id[0] || strchr(id, '/') || strchr(id, '\\') || strchr(id, ':') ||
            strstr(id, "..") || snprintf(bbs_name, sizeof(bbs_name),
                                          "%s.p8", id) >= sizeof(bbs_name)) {
            RG_LOGE("pico8: refusing invalid BBS cart id: %s", filename);
            return false;
        }
        local_name = bbs_name;
        RG_LOGI("pico8: resolving BBS cart %s as local companion %s",
                filename, local_name);
    }

    if (local_name[0] == '/' || strchr(local_name, '\\') ||
        strchr(local_name, ':') || strstr(local_name, "..")) {
        RG_LOGE("pico8: refusing non-local cart path: %s", local_name);
        return false;
    }

    const char *slash = strrchr(current_cart_path, '/');
    size_t dir_len = slash ? (size_t)(slash - current_cart_path + 1) : 0;
    size_t name_len = strlen(local_name);
    if (dir_len + name_len >= path_size) {
        RG_LOGE("pico8: cart-relative path is too long: %s", local_name);
        return false;
    }

    memcpy(path, current_cart_path, dir_len);
    memcpy(path + dir_len, local_name, name_len);
    path[dir_len + name_len] = '\0';
    return true;
}

bool pico8_read_cart_rom(const char *filename, uint8_t *dest,
                         size_t dest_size, size_t *rom_length)
{
    if (rom_length) *rom_length = 0;
    if (!filename || !filename[0] || !dest || dest_size < 0x4300)
        return false;

    char requested_path[RG_PATH_MAX + 1];
    if (!resolve_relative_cart_path(filename, requested_path,
                                    sizeof(requested_path)))
        return false;

    char resolved_path[RG_PATH_MAX + 1];
    if (!resolve_existing_cart_path(requested_path, resolved_path,
                                    sizeof(resolved_path))) {
        RG_LOGE("pico8: reload cart file not found: %s", requested_path);
        show_missing_cart_alert(filename);
        return false;
    }

    uint8_t *file_data = NULL;
    size_t file_size = 0;
    if (!rg_storage_read_file(resolved_path, (void **)&file_data,
                              &file_size, 0)) {
        RG_LOGE("pico8: unable to read reload cart file: %s", resolved_path);
        show_missing_cart_alert(filename);
        return false;
    }

    bool loaded = false;
    if (p8_png_is_png(file_data, file_size)) {
        char *text = NULL;
        size_t text_length = 0;
        uint8_t *png_rom = NULL;
        size_t png_rom_length = 0;
        if (p8_png_extract_text(file_data, file_size, &text, &text_length,
                                &png_rom, &png_rom_length) &&
            png_rom && png_rom_length >= 0x4300) {
            memcpy(dest, png_rom, 0x4300);
            if (rom_length) *rom_length = 0x4300;
            loaded = true;
        }
        free(text);
        free(png_rom);
    } else {
        LoadedCart cart = {0};
        const char *basename = strrchr(resolved_path, '/');
        basename = basename ? basename + 1 : resolved_path;
        if (p8_parse_text_data((const char *)file_data, file_size, basename,
                               &cart) && cart.cart.rom &&
            cart.cart.rom_len >= 0x4300) {
            memcpy(dest, cart.cart.rom, 0x4300);
            if (rom_length) *rom_length = 0x4300;
            loaded = true;
        }
        p8_cart_free(&cart);
    }
    free(file_data);

    if (loaded) {
        RG_LOGI("pico8: reload data bank loaded from %s", resolved_path);
    } else {
        RG_LOGE("pico8: unable to decode reload data bank: %s",
                resolved_path);
    }
    return loaded;
}

static bool process_cart_requests(bool *cart_changed)
{
    if (cart_changed) *cart_changed = false;

    // A loader cart can immediately request a breadcrumb return from its
    // top-level chunk. Bound the chain so malformed carts cannot loop forever
    // without ever reaching the frame loop.
    for (int transitions = 0; transitions < 32; ++transitions) {
        char filename[256] = {0};
        char breadcrumb[256] = {0};
        char param[256] = {0};
        int request = engine_take_cart_request(
            filename, sizeof(filename), breadcrumb, sizeof(breadcrumb),
            param, sizeof(param));
        if (request == P8_CART_REQUEST_NONE) return true;

        char target_path[RG_PATH_MAX + 1];
        char target_param[256];
        if (request == P8_CART_REQUEST_LOAD) {
            char requested_target_path[RG_PATH_MAX + 1];
            if (!resolve_relative_cart_path(filename, requested_target_path,
                                            sizeof(requested_target_path)))
                return false;

            bool resolved = resolve_existing_cart_path(
                requested_target_path, target_path, sizeof(target_path));
            if (!resolved && filename[0] == '#') {
                resolved = resolve_unique_sibling_cart(target_path,
                                                       sizeof(target_path));
                if (resolved) {
                    RG_LOGI("pico8: BBS companion %s not named locally; "
                            "using sole sibling %s", filename, target_path);
                }
            }
            if (!resolved) {
                RG_LOGE("pico8: missing companion cart requested by load(): %s",
                        requested_target_path);
                const char *missing_name = strrchr(requested_target_path, '/');
                show_missing_cart_alert(missing_name ? missing_name + 1
                                                     : requested_target_path);
                return false;
            }
            if (strcmp(requested_target_path, target_path) != 0) {
                RG_LOGI("pico8: resolved companion cart %s as %s",
                        requested_target_path, target_path);
            }

            // Snekburd's data bootstrap supplies a return filename through
            // PARAM_STR and calls extcmd breadcrumb even though its visible
            // breadcrumb label is nil. Keep a return entry in either case.
            if (breadcrumb[0] || param[0]) {
                if (!cart_breadcrumbs) {
                    cart_breadcrumbs = (P8Breadcrumb *)rg_alloc(
                        sizeof(P8Breadcrumb) * P8_BREADCRUMB_DEPTH, MEM_SLOW);
                    if (!cart_breadcrumbs) {
                        RG_LOGE("pico8: unable to allocate breadcrumb history");
                        return false;
                    }
                    memset(cart_breadcrumbs, 0,
                           sizeof(P8Breadcrumb) * P8_BREADCRUMB_DEPTH);
                }
                if (cart_breadcrumb_depth >= P8_BREADCRUMB_DEPTH) {
                    RG_LOGE("pico8: breadcrumb stack overflow");
                    return false;
                }
                P8Breadcrumb *entry =
                    &cart_breadcrumbs[cart_breadcrumb_depth++];
                snprintf(entry->path, sizeof(entry->path), "%s",
                         current_cart_path);
                snprintf(entry->param, sizeof(entry->param), "%s",
                         current_cart_param);
            }
            snprintf(target_param, sizeof(target_param), "%s", param);
        } else if (request == P8_CART_REQUEST_BREADCRUMB) {
            if (cart_breadcrumb_depth <= 0) {
                RG_LOGE("pico8: breadcrumb requested with no return cart");
                return false;
            }
            P8Breadcrumb *entry =
                &cart_breadcrumbs[--cart_breadcrumb_depth];
            snprintf(target_path, sizeof(target_path), "%s", entry->path);
            snprintf(target_param, sizeof(target_param), "%s", entry->param);
        } else if (request == P8_CART_REQUEST_RUN) {
            // run() restarts the current cart from a clean machine state and
            // does not alter multicart breadcrumb history.
            snprintf(target_path, sizeof(target_path), "%s",
                     current_cart_path);
            snprintf(target_param, sizeof(target_param), "%s", param);
        } else {
            RG_LOGE("pico8: unknown cart request %d", request);
            return false;
        }

        if (!cartdata_flush())
            return false;
        if (request == P8_CART_REQUEST_RUN) {
            // Close the outgoing VM before engine_init() replaces L. Calling
            // engine_init() alone would overwrite the pointer in init_lua()
            // and leak one complete Lua state on every in-cart restart.
            engine_prepare_cart_load();
            engine_init();
        } else {
            engine_prepare_cart_load();
        }

        char loaded_path[RG_PATH_MAX + 1];
        if (!load_cart_from_path(target_path, target_param,
                                 loaded_path, sizeof(loaded_path)))
            return false;

        snprintf(current_cart_path, sizeof(current_cart_path), "%s",
                 loaded_path);
        snprintf(current_cart_param, sizeof(current_cart_param), "%s",
                 target_param);
        if (cart_changed) *cart_changed = true;
    }

    RG_LOGE("pico8: cart transition loop exceeded 32 loads");
    return false;
}

// ----------------------------- Boot --------------------------------------

void app_main(void)
{
    const rg_config_t config = {
        .sampleRate      = 22050,   // matches SAMPLE_RATE in data.h
        .frameRate       = 30,
        .storageRequired = true,
        .romRequired     = true,
        .isLauncher      = false,
        .mallocAlwaysInternal = 0,
        .handlers = {
            .loadState  = load_state_cb,
            .saveState  = save_state_cb,
            .reset      = reset_cb,
            .screenshot = screenshot_cb,
            .event      = event_cb,
            .options    = options_cb,
            .about      = about_cb,
        },
    };

    app = rg_system_init(&config);
    if (!app) {
        RG_PANIC("rg_system_init returned NULL");
    }

    RG_LOGI("Pico-8 retro-go starting; rom=%s", app->romPath ? app->romPath : "(none)");

    if (!init_platform()) RG_PANIC("init_platform failed");
    if (!init_video())    RG_PANIC("init_video failed");
    if (!init_audio())    RG_PANIC("init_audio failed");

    // Spawn the cart frame loop on a dedicated retro-go FreeRTOS task
    // with its own stack. The system's main FreeRTOS task (which
    // app_main runs on) has CONFIG_ESP_MAIN_TASK_STACK_SIZE = 8 KB in
    // our build, and valdi.p8 measured ~3700 B accrued per _update
    // tick before the FreeRTOS stack-canary tripped with:
    //
    //   ***ERROR*** A stack overflow in task main has been detected.
    //
    // Giving the emitter 32 KB of stack removes the hard wall without
    // touching any sdkconfig — the entire fix must live inside
    // pico8/, and per-target sdkconfig.defaults overwrite our settings
    // during the build, so a runtime knob is the only viable path.
    //
    // Priority RG_TASK_PRIORITY_2 matches what retro-go uses for
    // typical application tasks; affinity = 0 pins the task to the
    // same core as the GUI/input helpers. The audio task lives on
    // core 1 and reads engine state via the sfx/synth channels[]
    // array, which is unchanged here — no shared-state coupling intro-
    // duced by relocating the emitter. (The audio-task-vs-engine cross-
    // core race on channels[] / synth state is pre-existing: the
    // inline version of this loop had the same race against
    // init_audio's audio_task; the relocation doesn't alter it. The
    // audio task may run briefly before pico8_run_task reaches
    // `engine_init()` and resets channels[] — same window as before.)
    rg_task_t *pico8_task = rg_task_create("pico8_run", &pico8_run_task, NULL,
                                           /*stackSize*/ 32 * 1024,
                                           /*queueDepth*/ 4,
                                           /*priority*/  RG_TASK_PRIORITY_2,
                                           /*affinity*/  0);
    if (!pico8_task) {
        RG_PANIC("pico8_run task create failed");
    }
    (void)pico8_task;

    // app_main has nothing else to do on the system main task. Pin it
    // to a delay-only loop (instead of `return`) so the task stays
    // alive with a known body — FreeRTOS will still hold the TCB but
    // its C-stack HWM stays near zero. Yield generously so the system
    // monitor (rg_sysmon, priority 5) and the pico8_run task are
    // scheduled without starvation.
    while (true) {
        rg_task_delay(1000);
    }
}

// Cart frame loop, lifted out of app_main() so it can run on its own
// FreeRTOS task with a larger stack (see the rg_task_create call in
// app_main). Behaviour is identical to the original inline `while
// (true)` block:
//   * engine_init() resets _init_done and reinitialises the engine
//     singletons (DrawState, fontsheet, hud_sprites, channels[],
//     map_data, cartdata, audiobuf).
//   * load_cart_from_rompath() parses the .p8 text into the engine's
//     blob-type slots and runs init_lua for the bundled bytecode.
//   * The per-tick loop reads the gamepad, surfaces MENU/OPTION to
//     the GUI before flip() can swallow them as a SELECT+START press,
//     and runs flip() + rg_system_tick() — rg_system_tick is a thread-
//     safe counter increment, safe to call from any task.
//
// The while-loop is intentionally non-terminating. rg_task_create's
// FreeRTOS trampoline (rg_system.c::task_wrapper) self-deletes the
// task only on function return; because we never return, the trampoline
// never fires and the task stays alive but inactive. A Lua-side
// unrecoverable error meanwhile routes through cart_panic_handler -> 
// rg_system_panic (__attribute__((noreturn))) which reboots the device.
static void pico8_run_task(void *arg) {
    (void)arg;

    engine_init();
    current_cart_path[0] = '\0';
    current_cart_param[0] = '\0';
    cart_breadcrumb_depth = 0;
    if (!load_cart_from_path(app->romPath, "", current_cart_path,
                             sizeof(current_cart_path))) {
        RG_LOGE("pico8: cartridge could not start; returning to launcher");
        rg_system_exit();
        return;
    }

    bool startup_cart_changed = false;
    if (!process_cart_requests(&startup_cart_changed)) {
        RG_LOGE("pico8: startup cart transition failed; returning to launcher");
        rg_system_exit();
        return;
    }

    int frame_rate = engine_frame_rate();
    rg_system_set_tick_rate(frame_rate);
    // PICO-8 carts are commonly 30 Hz. Start with full rendering and let
    // Retro-Go auto-frameskip raise this only when measured load requires it;
    // inheriting the platform default of one would cap light carts at 15 FPS.
    app->frameskip = 0;
    RG_LOGI("pico8: cartridge tick rate %d Hz", frame_rate);
    audio_task_set_paused(false);

    int64_t next_frame_us = rg_system_timer();
    int skip_frames = 0;
    uint32_t previous_system_keys = 0;

    // Frame loop: engine.cpp::flip() drives input, Lua callbacks, and video.
    // Pacing stays here so rg_system_tick records emulation/display work but
    // does not count deliberate sleep time as CPU-busy time.
    while (true) {
        // MENU/OPTION: rg_gui_game_menu() / rg_gui_options_menu(). Mirror
        // the gwenesis/snes9x wiring pattern. The check lives OUTSIDE
        // flip() because flip()'s handle_input() reads the same gamepad
        // mask and would otherwise swallow the press as a normal cart
        // button — a cart that listens for SELECT+START would steal
        // those combos first.
        uint32_t joystick = rg_input_read_gamepad();
        uint32_t system_keys = joystick & (RG_KEY_MENU | RG_KEY_OPTION);
        uint32_t pressed_system_keys = system_keys & ~previous_system_keys;
        previous_system_keys = system_keys;
        if (pressed_system_keys) {
            cartdata_flush();
            audio_task_set_paused(true);
            rg_audio_set_mute(true);
            if (pressed_system_keys & RG_KEY_MENU)
                rg_gui_game_menu();
            else
                rg_gui_options_menu();

            // Cartridge callbacks may update dset() values while either
            // menu is open. Persist those writes when control returns instead
            // of waiting for a later shutdown event.
            cartdata_flush();

            while (rg_input_read_gamepad() & (RG_KEY_MENU | RG_KEY_OPTION)) {
                rg_task_delay(10);
            }
            handle_input_reset();
            previous_system_keys = 0;
            next_frame_us = rg_system_timer();
            skip_frames = 0;

            // Reset and cartridge-defined pause-menu callbacks can request a
            // run()/load() while the Retro-Go dialog is active. Apply it now,
            // before one stale tick of the outgoing VM can execute.
            bool menu_cart_changed = false;
            if (!process_cart_requests(&menu_cart_changed)) {
                RG_LOGE("pico8: menu cart transition failed; returning to launcher");
                rg_system_exit();
                return;
            }
            if (menu_cart_changed) {
                frame_rate = engine_frame_rate();
                rg_system_set_tick_rate(frame_rate);
                RG_LOGI("pico8: menu switched cart; tick rate %d Hz",
                        frame_rate);
                next_frame_us = rg_system_timer();
                skip_frames = 0;
                previous_system_keys = 0;
                rg_audio_set_mute(false);
                audio_task_set_paused(false);
                continue;
            }
            rg_audio_set_mute(false);
            audio_task_set_paused(false);
        }

        if (atomic_exchange_explicit(&low_memory_gc_pending, false,
                                     memory_order_acq_rel)) {
            engine_collect_garbage();
        }

        int64_t t0 = rg_system_timer();
        bool draw_frame = (skip_frames == 0);
        flip(draw_frame);
        int elapsed_us = (int)(rg_system_timer() - t0);
        engine_set_frame_stats(elapsed_us, frame_rate);
        rg_system_tick(elapsed_us);

        bool cart_changed = false;
        bool runtime_audio_pause = engine_cart_request_pending();
        if (runtime_audio_pause) audio_task_set_paused(true);
        if (!process_cart_requests(&cart_changed)) {
            RG_LOGE("pico8: runtime cart transition failed; returning to launcher");
            rg_system_exit();
            return;
        }
        if (runtime_audio_pause) audio_task_set_paused(false);
        if (cart_changed) {
            frame_rate = engine_frame_rate();
            rg_system_set_tick_rate(frame_rate);
            RG_LOGI("pico8: switched cart; tick rate %d Hz", frame_rate);
            next_frame_us = rg_system_timer();
            skip_frames = 0;
            previous_system_keys = 0;
            continue;
        }

        // `_set_fps(30/60)` can change the requested host cadence from Lua
        // during an ordinary update. Apply it before scheduling the next tick
        // and discard cadence-specific remainder/backlog from the old rate.
        int requested_frame_rate = engine_frame_rate();
        if (requested_frame_rate != frame_rate) {
            frame_rate = requested_frame_rate;
            rg_system_set_tick_rate(frame_rate);
            RG_LOGI("pico8: runtime tick rate changed to %d Hz", frame_rate);
            next_frame_us = rg_system_timer();
            skip_frames = 0;
        }

        // Match the reference cores: render one frame, then run the
        // requested number of complete simulation ticks without drawing.
        if (skip_frames > 0) {
            skip_frames--;
        } else if (app->frameskip > 0) {
            skip_frames = app->frameskip;
        } else if (elapsed_us > app->frameTime + 1500) {
            skip_frames = 1;
        }

        // Retro-Go owns speed policy. frameTime changes when the user toggles
        // speedup, so pacing and overrun detection must not use a fixed
        // cartridge-rate interval here.
        int frame_time_us = app->frameTime;
        if (frame_time_us <= 0) frame_time_us = 1000000 / frame_rate;
        next_frame_us += frame_time_us;

        // Explicit cart flip animations can block for many host frames.
        // Discard extreme backlog rather than running seconds of stale
        // simulation afterward. Ordinary render overruns still catch up.
        int64_t current_us = rg_system_timer();
        int64_t max_backlog_us = (int64_t)frame_time_us * 6;
        if (next_frame_us < current_us - max_backlog_us)
            next_frame_us = current_us;

        while (true) {
            int64_t remaining = next_frame_us - rg_system_timer();
            if (remaining <= 0) break;
            if (remaining >= 2000) rg_task_delay((uint32_t)(remaining / 1000));
            else rg_task_yield();
        }
    }
}
