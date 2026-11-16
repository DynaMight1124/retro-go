#include "car_model.h"
#include "assets.h"
#include "fx.h"
#include "j2me.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fx_math.h"

Mesh* CarModel_wheel = NULL;

void CarModel_normal(Mesh* m, int k, int a, int b, int c) {
    float ax = (float)(m->x[b] - m->x[a]);
    float ay = (float)(m->y[b] - m->y[a]);
    float az = (float)(m->z[b] - m->z[a]);
    float bx = (float)(m->x[c] - m->x[a]);
    float by = (float)(m->y[c] - m->y[a]);
    float bz = (float)(m->z[c] - m->z[a]);

    float cx = ay * bz - az * by;
    float cy = az * bx - ax * bz;
    float cz = ax * by - ay * bx;
    float l = (float)Fx_sqrtD((double)cx * cx + (double)cy * cy + (double)cz * cz);
    if (l < 1e-6f) l = 1.0f;
    m->nx[k] = (int8_t)Fx_rf(cx / l * 127.0f);
    m->ny[k] = (int8_t)Fx_rf(cy / l * 127.0f);
    m->nz[k] = (int8_t)Fx_rf(cz / l * 127.0f);
}

static Mesh* makeMesh(int* x, int* y, int* z, int* u, int* v, int nv,
                      int* ta, int* tb, int* tc, int* tex, int* fl, int* col, int nt,
                      int* texId, int texCount, bool glass, int count) {
    Mesh* m = Mesh_new(nv, count);
    if (!m) return NULL;
    for (int i = 0; i < nv; i++) {
        m->x[i] = x[i]; m->y[i] = y[i]; m->z[i] = z[i];
        m->u[i] = u[i]; m->v[i] = v[i];
    }
    int k = 0;
    for (int i = 0; i < nt; i++) {
        if (((fl[i] & 8) != 0) != glass) continue;
        m->ti[k * 3] = ta[i];
        m->ti[k * 3 + 1] = tb[i];
        m->ti[k * 3 + 2] = tc[i];
        m->tex[k] = (int16_t)(tex[i] >= 0 && tex[i] < texCount ? texId[tex[i]] : -1);
        m->col[k] = col[i];
        int f = 0;
        if ((fl[i] & 2) != 0) f |= MESH_F_EMISSIVE;
        if ((fl[i] & 4) != 0) f |= MESH_F_TINT;
        if ((fl[i] & 16) != 0) f |= MESH_F_TINT2;
        m->flag[k] = (int8_t)f;
        CarModel_normal(m, k, ta[i], tb[i], tc[i]);
        k++;
    }
    Mesh_bounds(m);
    return m;
}

static void readMesh(DataInputStream* d, int* texId, int texCount, Mesh* out[2]) {
    int nv = dis_readUnsignedShort(d);
    int* x = (int*)malloc((size_t)nv * sizeof(int));
    int* y = (int*)malloc((size_t)nv * sizeof(int));
    int* z = (int*)malloc((size_t)nv * sizeof(int));
    int* u = (int*)malloc((size_t)nv * sizeof(int));
    int* v = (int*)malloc((size_t)nv * sizeof(int));
    for (int i = 0; i < nv; i++) {
        x[i] = dis_readShort(d);
        y[i] = dis_readShort(d);
        z[i] = dis_readShort(d);
        u[i] = dis_readShort(d);
        v[i] = dis_readShort(d);
    }
    int nt = dis_readUnsignedShort(d);
    int* ta = (int*)malloc((size_t)nt * sizeof(int));
    int* tb = (int*)malloc((size_t)nt * sizeof(int));
    int* tc = (int*)malloc((size_t)nt * sizeof(int));
    int* tex = (int*)malloc((size_t)nt * sizeof(int));
    int* fl = (int*)malloc((size_t)nt * sizeof(int));
    int* col = (int*)malloc((size_t)nt * sizeof(int));
    int ng = 0;
    for (int i = 0; i < nt; i++) {
        ta[i] = dis_readUnsignedShort(d);
        tb[i] = dis_readUnsignedShort(d);
        tc[i] = dis_readUnsignedShort(d);
        tex[i] = dis_readByte(d);
        fl[i] = dis_readUnsignedByte(d);
        col[i] = (dis_readUnsignedByte(d) << 16) | (dis_readUnsignedByte(d) << 8) | dis_readUnsignedByte(d);
        if ((fl[i] & 8) != 0) ng++;
    }
    out[0] = makeMesh(x, y, z, u, v, nv, ta, tb, tc, tex, fl, col, nt, texId, texCount, false, nt - ng);
    out[1] = ng > 0 ? makeMesh(x, y, z, u, v, nv, ta, tb, tc, tex, fl, col, nt, texId, texCount, true, ng) : NULL;

    free(x); free(y); free(z); free(u); free(v);
    free(ta); free(tb); free(tc); free(tex); free(fl); free(col);
}

static int* readTex(DataInputStream* d, int* outCount) {
    int nt = dis_readUnsignedByte(d);
    if (outCount) *outCount = nt;
    int* texId = (int*)malloc((size_t)nt * sizeof(int));
    for (int i = 0; i < nt; i++) {
        int l = dis_readUnsignedByte(d);
        char b[256];
        dis_readFully(d, (uint8_t*)b, l);
        b[l] = '\0';
        char path[300];
        snprintf(path, sizeof(path), "/t_%s.png", b);
        texId[i] = Assets_acquirePath(path);
    }
    return texId;
}

static Mesh* loadWheel(void) {
    int len = 0;
    uint8_t* data = Assets_readAll("/wheel.bin", &len);
    if (!data) return NULL;
    DataInputStream d;
    dis_init(&d, data, len);
    int nt = 0;
    int* t = readTex(&d, &nt);
    Mesh* meshes[2];
    readMesh(&d, t, nt, meshes);
    dis_close(&d);
    free(data);
    free(t);
    if (meshes[1]) Mesh_free(meshes[1]);
    return meshes[0];
}

CarModel* CarModel_load(const char* name) {
    char path[256];
    snprintf(path, sizeof(path), "/car_%s.bin", name);
    int len = 0;
    uint8_t* data = Assets_readAll(path, &len);
    if (!data) return NULL;

    DataInputStream d;
    dis_init(&d, data, len);
    uint8_t mg[4];
    dis_readFully(&d, mg, 4);

    CarModel* c = (CarModel*)calloc(1, sizeof(CarModel));
    c->name = strdup(name);
    c->tex = readTex(&d, &c->texCount);
    c->dimX = dis_readShort(&d);
    c->dimY = dis_readShort(&d);
    c->dimZ = dis_readShort(&d);
    c->seatX = dis_readShort(&d);
    c->seatY = dis_readShort(&d);
    c->seatZ = dis_readShort(&d);
    c->nw = dis_readUnsignedByte(&d);
    for (int i = 0; i < c->nw; i++) {
        int x = dis_readShort(&d);
        int y = dis_readShort(&d);
        int z = dis_readShort(&d);
        int r = dis_readShort(&d);
        if (i < 4) {
            c->wx[i] = x; c->wy[i] = y; c->wz[i] = z; c->wr[i] = r;
        }
    }
    if (c->nw > 4) c->nw = 4;
    c->bike = dis_readUnsignedByte(&d) != 0;
    c->np = dis_readUnsignedByte(&d);
    c->partId = (int*)malloc((size_t)c->np * sizeof(int));
    c->ok = (Mesh***)malloc((size_t)c->np * sizeof(Mesh**));
    c->dam = (Mesh***)malloc((size_t)c->np * sizeof(Mesh**));

    for (int p = 0; p < c->np; p++) {
        c->partId[p] = dis_readUnsignedByte(&d);
        dis_readShort(&d); dis_readShort(&d); dis_readShort(&d);
        int nv = dis_readUnsignedByte(&d);
        Mesh** a = (Mesh**)malloc(2 * sizeof(Mesh*));
        readMesh(&d, c->tex, c->texCount, a);
        c->ok[p] = a;
        if (nv > 1) {
            Mesh** b = (Mesh**)malloc(2 * sizeof(Mesh*));
            readMesh(&d, c->tex, c->texCount, b);
            c->dam[p] = b;
        } else {
            c->dam[p] = a;
        }
    }
    Mesh* f[2];
    readMesh(&d, c->tex, c->texCount, f);
    c->farMesh = f[0];
    c->farGlass = f[1];
    dis_close(&d);
    free(data);

    if (CarModel_wheel == NULL) {
        CarModel_wheel = loadWheel();
    }
    return c;
}

void CarModel_unload(CarModel* c) {
    if (!c) return;
    if (c->tex) {
        for (int i = 0; i < c->texCount; i++) {
            Assets_release(c->tex[i]);
        }
        free(c->tex);
        c->tex = NULL;
    }
}

void CarModel_free(CarModel* c) {
    if (!c) return;
    CarModel_unload(c);
    if (c->ok) {
        for (int p = 0; p < c->np; p++) {
            bool same = (c->dam[p] == c->ok[p]);
            if (c->ok[p]) {
                if (c->ok[p][0]) Mesh_free(c->ok[p][0]);
                if (c->ok[p][1]) Mesh_free(c->ok[p][1]);
                free(c->ok[p]);
            }
            if (!same && c->dam[p]) {
                if (c->dam[p][0]) Mesh_free(c->dam[p][0]);
                if (c->dam[p][1]) Mesh_free(c->dam[p][1]);
                free(c->dam[p]);
            }
        }
        free(c->ok);
        free(c->dam);
    }
    if (c->farMesh) Mesh_free(c->farMesh);
    if (c->farGlass) Mesh_free(c->farGlass);
    free(c->partId);
    free(c->name);
    free(c);
}
