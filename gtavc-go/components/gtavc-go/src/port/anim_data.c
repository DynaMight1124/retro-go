#include "anim_data.h"
#include "assets.h"
#include "j2me.h"
#include <rg_system.h>

int AnimData_n = 0;
char** AnimData_name = NULL;
int* AnimData_dur = NULL;
int* AnimData_nf = NULL;
int* AnimData_travel = NULL;
bool* AnimData_loop = NULL;
int16_t** AnimData_fr = NULL;

bool AnimData_load(void) {
    int len = 0;
    uint8_t* data = Assets_readAll("/anim.bin", &len);
    if (!data) return false;

    DataInputStream d;
    dis_init(&d, data, len);

    AnimData_n = dis_readUnsignedShort(&d);
    if (AnimData_n <= 0 || AnimData_n > 4096) { dis_close(&d); free(data); AnimData_n = 0; return false; }
    AnimData_name = (char**)rg_alloc((size_t)AnimData_n * sizeof(char*), MEM_FAST);
    AnimData_dur = (int*)rg_alloc((size_t)AnimData_n * sizeof(int), MEM_FAST);
    AnimData_nf = (int*)rg_alloc((size_t)AnimData_n * sizeof(int), MEM_FAST);
    AnimData_travel = (int*)rg_alloc((size_t)AnimData_n * sizeof(int), MEM_FAST);
    AnimData_loop = (bool*)rg_alloc((size_t)AnimData_n * sizeof(bool), MEM_FAST);
    AnimData_fr = (int16_t**)rg_alloc((size_t)AnimData_n * sizeof(int16_t*), MEM_FAST);
    if (!AnimData_name || !AnimData_dur || !AnimData_nf || !AnimData_travel || !AnimData_loop || !AnimData_fr) {
        AnimData_free();
        dis_close(&d);
        free(data);
        return false;
    }

    for (int i = 0; i < AnimData_n; i++) {
        int l = dis_readUnsignedByte(&d);
        char* b = (char*)malloc((size_t)l + 1);
        if (!b) {
            AnimData_free();
            dis_close(&d);
            free(data);
            return false;
        }
        dis_readFully(&d, (uint8_t*)b, l);
        b[l] = '\0';
        AnimData_name[i] = b;
        AnimData_dur[i] = dis_readUnsignedShort(&d);
        AnimData_nf[i] = dis_readUnsignedShort(&d);
        AnimData_loop[i] = dis_readUnsignedByte(&d) != 0;
        AnimData_travel[i] = dis_readShort(&d);

        if (AnimData_nf[i] <= 0 || AnimData_nf[i] > 20000) {
            AnimData_free();
            dis_close(&d);
            free(data);
            return false;
        }
        int totalShorts = AnimData_nf[i] * 93;
        int16_t* f = (int16_t*)malloc((size_t)totalShorts * sizeof(int16_t));
        if (!f) {
            AnimData_free();
            dis_close(&d);
            free(data);
            return false;
        }
        for (int k = 0; k < totalShorts; k++) {
            f[k] = (int16_t)dis_readShort(&d);
        }
        AnimData_fr[i] = f;
    }

    dis_close(&d);
    free(data);
    return true;
}

int AnimData_find(const char* s) {
    if (!s) return 0;
    for (int i = 0; i < AnimData_n; i++) {
        if (strcmp(AnimData_name[i], s) == 0) return i;
    }
    return 0;
}

int AnimData_len(int c) {
    if (c < 0 || c >= AnimData_n) return 0;
    return (AnimData_nf[c] - 1) * 50;
}

int AnimData_speed(int c) {
    if (c < 0 || c >= AnimData_n) return 1;
    int l = AnimData_len(c);
    if (l == 0) return 1;
    return AnimData_travel[c] == 0 ? 1 : AnimData_travel[c] * 16 * 1000 / l;
}

static void anim_identity(int* out) {
    for (int i = 0; i < 10; i++) {
        int b = i * 9;
        out[b + 0] = 16384; out[b + 1] = 0; out[b + 2] = 0;
        out[b + 3] = 0; out[b + 4] = 16384; out[b + 5] = 0;
        out[b + 6] = 0; out[b + 7] = 0; out[b + 8] = 16384;
    }
    out[90] = out[91] = out[92] = 0;
}

void AnimData_sample(int c, int tMs, int* out) {
    if (c < 0 || c >= AnimData_n || !AnimData_fr[c] || AnimData_nf[c] < 1) {
        /* Missing/corrupt animation file: keep a neutral pose so pedestrians
           are still visible instead of collapsing into degenerate triangles. */
        anim_identity(out);
        return;
    }
    int16_t* f = AnimData_fr[c];
    int last = AnimData_nf[c] - 1;
    if (last <= 0) {
        for (int k = 0; k < 93; k++) out[k] = f[k];
        return;
    }
    int pos = tMs * 20; // frame * 1000
    int i = pos / 1000;
    int w = ((pos % 1000) << 8) / 1000;
    int j;
    if (AnimData_loop[c]) {
        i %= last;
        j = i + 1 >= last ? 0 : i + 1;
    } else {
        if (i >= last) {
            i = last;
            j = last;
            w = 0;
        } else {
            j = i + 1;
        }
    }
    int a = i * 93;
    int b = j * 93;
    for (int k = 0; k < 93; k++) {
        out[k] = f[a + k] + (((f[b + k] - f[a + k]) * w) >> 8);
    }
}

void AnimData_free(void) {
    if (!AnimData_name) return;
    for (int i = 0; i < AnimData_n; i++) {
        free(AnimData_name[i]);
        free(AnimData_fr[i]);
    }
    free(AnimData_name); AnimData_name = NULL;
    free(AnimData_dur); AnimData_dur = NULL;
    free(AnimData_nf); AnimData_nf = NULL;
    free(AnimData_travel); AnimData_travel = NULL;
    free(AnimData_loop); AnimData_loop = NULL;
    free(AnimData_fr); AnimData_fr = NULL;
    AnimData_n = 0;
}
