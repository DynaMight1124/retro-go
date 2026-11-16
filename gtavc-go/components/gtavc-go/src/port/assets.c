#include "assets.h"
#include "r3d.h"
#include "j2me.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

int Assets_SLOTS = 256;
char* Assets_slotName[ASSETS_MAX_SLOTS];
int Assets_slotRef[ASSETS_MAX_SLOTS];
int Assets_slotUse[ASSETS_MAX_SLOTS];
bool Assets_legacy = true;
int Assets_useClock = 0;
struct R3D* Assets_r = NULL;

void Assets_init(struct R3D* r) {
    Assets_r = r;
    int len = 0;
    uint8_t* legacyData = platform_read_resource("/legacy.txt", &len);
    if (legacyData) {
        Assets_legacy = true;
        free(legacyData);
    } else {
        /* On Retro-Go / ESP32 handhelds, default to legacy mode */
        Assets_legacy = true;
    }
    Assets_SLOTS = 256;
    for (int i = 0; i < ASSETS_MAX_SLOTS; i++) {
        if (Assets_slotName[i]) {
            free(Assets_slotName[i]);
            Assets_slotName[i] = NULL;
        }
        Assets_slotRef[i] = 0;
        Assets_slotUse[i] = 0;
    }
    Assets_useClock = 0;
}


/* One packed atlas for the 32x32 map textures.  Opening ~70 files per tile
   was still visible on the SD card; a single seek/read of 4 KB per texture
   is much cheaper.  The raw per-file cache remains as fallback. */
static FILE* atlasFile = NULL;
static int atlasCount = 0;
static int atlasTried = 0;

static int pathTextureId(const char* path) {
    if (!path || strncmp(path, "/x/", 3) != 0) return -1;
    const char* s = path + 3;
    int id = 0;
    if (*s < '0' || *s > '9') return -1;
    while (*s >= '0' && *s <= '9') { id = id * 10 + (*s - '0'); s++; }
    if (*s != '.') return -1;
    return id;
}

static int* decodeAtlasTexture(const char* path, int dim[2]) {
    int id = pathTextureId(path);
    if (id < 0) return NULL;
    if (!atlasTried) {
        atlasTried = 1;
        char p[600];
        const char* d = platform_assets_dir();
        snprintf(p, sizeof(p), "%s/x/xatlas.bin", d ? d : "");
        atlasFile = fopen(p, "rb");
        if (!atlasFile) {
            atlasFile = fopen("assets/x/xatlas.bin", "rb");
        }
        if (atlasFile) {
            uint8_t h[8];
            if (fread(h, 1, 8, atlasFile) == 8 && memcmp(h, "XAT1", 4) == 0) {
                atlasCount = (int)((uint32_t)h[4] | ((uint32_t)h[5] << 8) |
                                   ((uint32_t)h[6] << 16) | ((uint32_t)h[7] << 24));
                if (atlasCount <= 0 || atlasCount > 8192) {
                    fclose(atlasFile);
                    atlasFile = NULL;
                    atlasCount = 0;
                }
            } else {
                fclose(atlasFile);
                atlasFile = NULL;
            }
        }
    }
    if (!atlasFile || id >= atlasCount) return NULL;
    if (fseek(atlasFile, 8L + (long)id * 32L * 32L * 4L, SEEK_SET) != 0) return NULL;
    int* p = (int*)malloc(32 * 32 * sizeof(int));
    if (!p) return NULL;
    if (fread(p, 1, 32 * 32 * sizeof(int), atlasFile) != 32 * 32 * sizeof(int)) {
        free(p);
        return NULL;
    }
    dim[0] = 32;
    dim[1] = 32;
    return p;
}

/* Pre-decoded /x/<id>.raw: u32 width, u32 height, then ARGB words.  This
   bypasses stb PNG decoding during tile streaming. */
static int* decodeRawTexture(const char* path, int dim[2]) {
    if (!path || strncmp(path, "/x/", 3) != 0) return NULL;
    size_t n = strlen(path);
    if (n < 4 || strcmp(path + n - 4, ".png") != 0) return NULL;
    char raw[320];
    if (n > sizeof(raw) - 8) return NULL;
    memcpy(raw, path, n - 4);
    memcpy(raw + n - 4, ".raw", 5);
    int len = 0;
    uint8_t* b = platform_read_resource(raw, &len);
    if (!b || len < 8) { free(b); return NULL; }
    uint32_t w = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    uint32_t h = (uint32_t)b[4] | ((uint32_t)b[5] << 8) | ((uint32_t)b[6] << 16) | ((uint32_t)b[7] << 24);
    if (w == 0 || h == 0 || w > 1024 || h > 1024 ||
        (size_t)len < 8 + (size_t)w * h * 4) {
        free(b);
        return NULL;
    }
    int* p = (int*)malloc((size_t)w * h * sizeof(int));
    if (!p) { free(b); return NULL; }
    memcpy(p, b + 8, (size_t)w * h * sizeof(int));
    free(b);
    dim[0] = (int)w;
    dim[1] = (int)h;
    return p;
}

int* Assets_decode(const char* path, int dim[2]) {
    int* atl = decodeAtlasTexture(path, dim);
    if (atl) return atl;
    int* raw = decodeRawTexture(path, dim);
    if (raw) return raw;

    Image* im = Image_createImageRes(path);
    if (!im) {
        dim[0] = 0; dim[1] = 0;
        return NULL;
    }
    int w = im->width;
    int h = im->height;
    int* p = (int*)malloc((size_t)w * (size_t)h * sizeof(int));
    if (!p) {
        Image_free(im);
        dim[0] = 0; dim[1] = 0;
        return NULL;
    }
    Image_getRGB(im, p, 0, w, 0, 0, w, h);
    int total = w * h;
    for (int i = 0; i < total; i++) {
        uint32_t c = (uint32_t)p[i];
        p[i] = ((c >> 24) < 128) ? 0 : (int)(c | 0xFF000000u);
    }
    dim[0] = w;
    dim[1] = h;
    Image_free(im);
    return p;
}

int Assets_find(const char* path) {
    if (!path) return -1;
    for (int i = 0; i < Assets_SLOTS; i++) {
        if (Assets_slotName[i] && strcmp(path, Assets_slotName[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static int freeSlot(void) {
    int best = -1, bu = INT_MAX;
    for (int i = 0; i < Assets_SLOTS; i++) {
        if (Assets_slotName[i] == NULL) return i;
        if (Assets_slotRef[i] == 0 && Assets_slotUse[i] < bu) {
            bu = Assets_slotUse[i];
            best = i;
        }
    }
    if (best >= 0) {
        if (Assets_r && Assets_r->T[best]) {
            free(Assets_r->T[best]);
            Assets_r->T[best] = NULL;
        }
        free(Assets_slotName[best]);
        Assets_slotName[best] = NULL;
    }
    return best;
}

int Assets_acquire(const char* path, int* pix, int w, int h) {
    if (!path) return -1;
    int s = Assets_find(path);
    if (s < 0) {
        int d[2] = {0, 0};
        if (pix == NULL) {
            pix = Assets_decode(path, d);
            if (!pix) return -1;
            w = d[0];
            h = d[1];
        }
        s = freeSlot();
        if (s < 0) {
            free(pix);
            return -1;
        }
        Assets_slotName[s] = strdup(path);
        if (Assets_r) {
            R3D_setTexture(Assets_r, s, pix, w, h);
            Assets_r->uvs[s] = 0;
        }
    }
    Assets_slotRef[s]++;
    Assets_slotUse[s] = ++Assets_useClock;
    return s;
}

int Assets_acquirePath(const char* path) {
    return Assets_acquire(path, NULL, 0, 0);
}

void Assets_release(int s) {
    if (s >= 0 && s < Assets_SLOTS && Assets_slotRef[s] > 0) {
        Assets_slotRef[s]--;
        if (Assets_slotRef[s] == 0) {
            if (Assets_r && Assets_r->T[s]) {
                free(Assets_r->T[s]);
                Assets_r->T[s] = NULL;
            }
            if (Assets_slotName[s]) {
                free(Assets_slotName[s]);
                Assets_slotName[s] = NULL;
            }
            Assets_slotUse[s] = 0;
        }
    }
}

int* Assets_imgPix(const char* res, int dim[2]) {
    Image* im = Image_createImageRes(res);
    if (!im) {
        dim[0] = 0; dim[1] = 0;
        return NULL;
    }
    int w = im->width;
    int h = im->height;
    int* p = (int*)malloc((size_t)w * (size_t)h * sizeof(int));
    if (p) {
        Image_getRGB(im, p, 0, w, 0, 0, w, h);
    }
    dim[0] = w;
    dim[1] = h;
    Image_free(im);
    return p;
}

int* Assets_intensity(const char* res, int dim[2]) {
    int* p = Assets_imgPix(res, dim);
    if (!p) return NULL;
    int total = dim[0] * dim[1];
    for (int i = 0; i < total; i++) {
        p[i] = (p[i] >> 16) & 255;
    }
    return p;
}

uint8_t* Assets_readAll(const char* res, int* outLen) {
    return platform_read_resource((String)res, outLen);
}

char** Assets_readLines(const char* res, int* outCount) {
    if (outCount) *outCount = 0;
    int len = 0;
    uint8_t* b = Assets_readAll(res, &len);
    if (!b) return NULL;

    char* tmp[400];
    int n = 0, p = 0;
    while (p < len && n < 400) {
        int q = p;
        while (q < len && b[q] != '\n') q++;
        int lineLen = q - p;
        if (lineLen > 0 && b[q - 1] == '\r') lineLen--;
        if (lineLen > 0) {
            char* line = (char*)malloc((size_t)lineLen + 1);
            memcpy(line, b + p, (size_t)lineLen);
            line[lineLen] = '\0';
            /* trim */
            int st = 0, en = lineLen;
            while (st < en && ((uint8_t)line[st] <= ' ')) st++;
            while (en > st && ((uint8_t)line[en - 1] <= ' ')) en--;
            line[en] = '\0';
            if (en > st) {
                memmove(line, line + st, (size_t)(en - st) + 1);
                tmp[n++] = line;
            } else {
                free(line);
            }
        }
        p = q + 1;
    }
    free(b);

    if (n == 0) return NULL;
    char** out = (char**)malloc((size_t)n * sizeof(char*));
    for (int i = 0; i < n; i++) out[i] = tmp[i];
    if (outCount) *outCount = n;
    return out;
}

void Assets_freeLines(char** lines, int count) {
    if (!lines) return;
    for (int i = 0; i < count; i++) free(lines[i]);
    free(lines);
}

void Assets_cleanup(void) {
    if (atlasFile) { fclose(atlasFile); atlasFile = NULL; }
    atlasCount = 0;
    atlasTried = 0;
    for (int i = 0; i < ASSETS_MAX_SLOTS; i++) {
        if (Assets_slotName[i]) {
            free(Assets_slotName[i]);
            Assets_slotName[i] = NULL;
        }
        if (Assets_r && Assets_r->T[i]) {
            free(Assets_r->T[i]);
            Assets_r->T[i] = NULL;
        }
        Assets_slotRef[i] = 0;
        Assets_slotUse[i] = 0;
    }
}
