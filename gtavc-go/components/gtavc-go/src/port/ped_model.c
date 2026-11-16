#include "ped_model.h"
#include "assets.h"
#include "fx.h"
#include "j2me.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fx_math.h"

static const int PAR[9] = { -1, 0, 1, 0, 3, -1, 5, -1, 7 };

void PedModel_hand(PedModel* m, int o[3]) {
    int* R = m->rp[PED_MODEL_FORE_A];
    o[0] = m->px[PED_MODEL_FORE_A] + ((R[0] * m->hx + R[1] * m->hy + R[2] * m->hz) >> 14);
    o[1] = m->py[PED_MODEL_FORE_A] + ((R[3] * m->hx + R[4] * m->hy + R[5] * m->hz) >> 14);
    o[2] = m->pz[PED_MODEL_FORE_A] + ((R[6] * m->hx + R[7] * m->hy + R[8] * m->hz) >> 14);
}

PedModel* PedModel_load(const char* res) {
    int len = 0;
    uint8_t* data = Assets_readAll(res, &len);
    if (!data) return NULL;

    DataInputStream d;
    dis_init(&d, data, len);

    PedModel* m = (PedModel*)calloc(1, sizeof(PedModel));
    if (!m) { dis_close(&d); free(data); return NULL; }
    m->n = dis_readUnsignedByte(&d);
    m->rootH = dis_readShort(&d);
    m->jx = (int*)malloc((size_t)m->n * sizeof(int));
    m->jy = (int*)malloc((size_t)m->n * sizeof(int));
    m->jz = (int*)malloc((size_t)m->n * sizeof(int));
    m->part = (Mesh**)calloc((size_t)m->n, sizeof(Mesh*));
    if (!m->jx || !m->jy || !m->jz || !m->part) {
        dis_close(&d); free(data); PedModel_free(m); return NULL;
    }

    for (int p = 0; p < m->n; p++) {
        m->jx[p] = -dis_readShort(&d);
        m->jy[p] = dis_readShort(&d);
        m->jz[p] = dis_readShort(&d);

        int nv = dis_readShort(&d);
        int nt = 0;
        Mesh* me = (Mesh*)calloc(1, sizeof(Mesh));
        if (!me) { dis_close(&d); free(data); PedModel_free(m); return NULL; }
        me->nv = nv;
        me->x = (int*)malloc((size_t)nv * sizeof(int));
        me->y = (int*)malloc((size_t)nv * sizeof(int));
        me->z = (int*)malloc((size_t)nv * sizeof(int));
        me->u = (int*)malloc((size_t)nv * sizeof(int));
        me->v = (int*)malloc((size_t)nv * sizeof(int));
        if (!me->x || !me->y || !me->z || !me->u || !me->v) {
            Mesh_free(me); dis_close(&d); free(data); PedModel_free(m); return NULL;
        }
        for (int i = 0; i < nv; i++) {
            me->x[i] = -dis_readShort(&d);
            me->y[i] = dis_readShort(&d);
            me->z[i] = dis_readShort(&d);
            me->u[i] = dis_readShort(&d);
            me->v[i] = dis_readShort(&d);
        }
        nt = dis_readShort(&d);
        me->nt = nt;
        me->ti = (int*)malloc((size_t)nt * 3 * sizeof(int));
        me->tex = (int16_t*)calloc((size_t)nt, sizeof(int16_t));
        me->col = (int*)calloc((size_t)nt, sizeof(int));
        me->flag = (int8_t*)calloc((size_t)nt, sizeof(int8_t));
        me->nx = (int8_t*)malloc((size_t)nt * sizeof(int8_t));
        me->ny = (int8_t*)malloc((size_t)nt * sizeof(int8_t));
        me->nz = (int8_t*)malloc((size_t)nt * sizeof(int8_t));
        if (!me->ti || !me->tex || !me->col || !me->flag || !me->nx || !me->ny || !me->nz) {
            Mesh_free(me); dis_close(&d); free(data); PedModel_free(m); return NULL;
        }

        for (int i = 0; i < nt; i++) {
            int i0 = dis_readShort(&d);
            int i1 = dis_readShort(&d);
            int i2 = dis_readShort(&d);
            me->ti[i * 3]     = i0;
            me->ti[i * 3 + 1] = i2;
            me->ti[i * 3 + 2] = i1;
            me->tex[i] = 0;

            int ax = me->x[i1] - me->x[i0], ay = me->y[i1] - me->y[i0], az = me->z[i1] - me->z[i0];
            int bx = me->x[i2] - me->x[i0], by = me->y[i2] - me->y[i0], bz = me->z[i2] - me->z[i0];
            float cx = (float)ay * (float)bz - (float)az * (float)by;
            float cy = (float)az * (float)bx - (float)ax * (float)bz;
            float cz = (float)ax * (float)by - (float)ay * (float)bx;
            float l = (float)Fx_sqrtD((double)cx * cx + (double)cy * cy + (double)cz * cz);
            if (l < 1e-6f) l = 1.0f;
            me->nx[i] = (int8_t)Fx_rf(-cx / l * 127.0f);
            me->ny[i] = (int8_t)Fx_rf(-cy / l * 127.0f);
            me->nz[i] = (int8_t)Fx_rf(-cz / l * 127.0f);
        }
        Mesh_bounds(me);
        m->part[p] = me;
    }
    dis_close(&d);
    free(data);

    Mesh* f = m->part[PED_MODEL_FORE_A];
    int64_t best = -1;
    for (int i = 0; i < f->nv; i++) {
        int64_t dd = (int64_t)f->x[i] * f->x[i] + (int64_t)f->y[i] * f->y[i] + (int64_t)f->z[i] * f->z[i];
        if (dd > best) {
            best = dd;
            m->hx = f->x[i];
            m->hy = f->y[i];
            m->hz = f->z[i];
        }
    }
    return m;
}

IRAM_ATTR void PedModel_draw(PedModel* m, R3D* r, int fx, int fy, int fz, int yaw, int tilt, int lift, const int* ang, int tex) {
    Fx_rotY(m->rYaw, yaw);
    if (tilt != 0) {
        Fx_rotX(m->a, tilt);
        Fx_mul(m->rYaw, m->a, m->rbase);
    } else {
        memcpy(m->rbase, m->rYaw, 9 * sizeof(int));
    }

    Fx_rotX(m->a, ang[0]);
    Fx_rotY(m->b, ang[1]);
    Fx_mul(m->a, m->b, m->tmp);
    Fx_rotZ(m->b, ang[2]);
    Fx_mul(m->tmp, m->b, m->rt);
    Fx_mul(m->rbase, m->rt, m->rtor);

    int p0x = fx, p0y = fy + m->rootH + lift, p0z = fz;

    for (int i = 0; i < m->n; i++) {
        int par = PAR[i];
        int* R = m->rp[i];

        Fx_rotX(m->a, ang[i * 3]);
        Fx_rotY(m->b, ang[i * 3 + 1]);
        Fx_mul(m->a, m->b, m->tmp);
        Fx_rotZ(m->b, ang[i * 3 + 2]);
        Fx_mul(m->tmp, m->b, m->a);

        if (i == PED_MODEL_TORSO) {
            memcpy(R, m->rtor, 9 * sizeof(int));
            m->px[i] = p0x; m->py[i] = p0y; m->pz[i] = p0z;
        } else {
            int* pf;
            int bx, by, bz, ox0, oy0, oz0;
            if (par < 0) {
                pf = m->rbase; bx = p0x; by = p0y; bz = p0z;
                ox0 = m->jx[i]; oy0 = m->jy[i]; oz0 = m->jz[i];
            } else if (par == 0) {
                pf = m->rtor; bx = p0x; by = p0y; bz = p0z;
                ox0 = m->jx[i]; oy0 = m->jy[i]; oz0 = m->jz[i];
            } else {
                pf = m->rp[par]; bx = m->px[par]; by = m->py[par]; bz = m->pz[par];
                ox0 = m->jx[i] - m->jx[par]; oy0 = m->jy[i] - m->jy[par]; oz0 = m->jz[i] - m->jz[par];
            }
            m->px[i] = bx + ((pf[0] * ox0 + pf[1] * oy0 + pf[2] * oz0) >> 14);
            m->py[i] = by + ((pf[3] * ox0 + pf[4] * oy0 + pf[5] * oz0) >> 14);
            m->pz[i] = bz + ((pf[6] * ox0 + pf[7] * oy0 + pf[8] * oz0) >> 14);
            Fx_mul(pf, m->a, R);
        }

        int dx = m->px[i] - r->camX, dy = m->py[i] - r->camY, dz = m->pz[i] - r->camZ;
        if (dx > 30000 || dx < -30000 || dz > 30000 || dz < -30000) continue;
        const int* mv = r->mv;
        int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
        int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
        int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
        Fx_mul(mv, R, m->mm);

        int lmx = (R[0] * r->lx + R[3] * r->ly + R[6] * r->lz) >> 14;
        int lmy = (R[1] * r->lx + R[4] * r->ly + R[7] * r->lz) >> 14;
        int lmz = (R[2] * r->lx + R[5] * r->ly + R[8] * r->lz) >> 14;

        r->forceTex = tex;
        R3D_drawMesh(r, m->part[i], m->mm, tx, ty, tz, lmx, lmy, lmz, 0);
        r->forceTex = -1;
    }
}

IRAM_ATTR void PedModel_drawPose(PedModel* m, R3D* r, int fx, int fy, int fz, int yaw, const int* pose, int tex) {
    Fx_rotY(m->rYaw, yaw);
    int offR = pose[90], offU = pose[91], offF = pose[92];
    int p0x = fx + ((m->rYaw[0] * offR + m->rYaw[2] * offF) >> 14);
    int p0y = fy + m->rootH + offU;
    int p0z = fz + ((m->rYaw[6] * offR + m->rYaw[8] * offF) >> 14);

    for (int i = 0; i < 10; i++) {
        for (int k = 0; k < 9; k++) m->tmp[k] = pose[i * 9 + k];
        Fx_mul(m->rYaw, m->tmp, m->rp[i == 9 ? 0 : i]);
        if (i == 9) memcpy(m->rb, m->rp[0], 9 * sizeof(int));
        if (i == 9) Fx_rotY(m->rYaw, yaw);
    }
    for (int k = 0; k < 9; k++) m->tmp[k] = pose[k];
    Fx_mul(m->rYaw, m->tmp, m->rp[0]);

    for (int i = 0; i < m->n; i++) {
        int par = PAR[i];
        int* R = m->rp[i];
        if (i == PED_MODEL_TORSO) {
            m->px[i] = p0x; m->py[i] = p0y; m->pz[i] = p0z;
        } else {
            int* pf;
            int bx, by, bz, ox0, oy0, oz0;
            if (par < 0) {
                pf = m->rb; bx = p0x; by = p0y; bz = p0z;
                ox0 = m->jx[i]; oy0 = m->jy[i]; oz0 = m->jz[i];
            } else if (par == 0) {
                pf = m->rp[0]; bx = p0x; by = p0y; bz = p0z;
                ox0 = m->jx[i]; oy0 = m->jy[i]; oz0 = m->jz[i];
            } else {
                pf = m->rp[par]; bx = m->px[par]; by = m->py[par]; bz = m->pz[par];
                ox0 = m->jx[i] - m->jx[par]; oy0 = m->jy[i] - m->jy[par]; oz0 = m->jz[i] - m->jz[par];
            }
            m->px[i] = bx + ((pf[0] * ox0 + pf[1] * oy0 + pf[2] * oz0) >> 14);
            m->py[i] = by + ((pf[3] * ox0 + pf[4] * oy0 + pf[5] * oz0) >> 14);
            m->pz[i] = bz + ((pf[6] * ox0 + pf[7] * oy0 + pf[8] * oz0) >> 14);
        }

        int dx = m->px[i] - r->camX, dy = m->py[i] - r->camY, dz = m->pz[i] - r->camZ;
        if (dx > 30000 || dx < -30000 || dz > 30000 || dz < -30000) continue;
        const int* mv = r->mv;
        int tx = (mv[0] * dx + mv[1] * dy + mv[2] * dz) >> 14;
        int ty = (mv[3] * dx + mv[4] * dy + mv[5] * dz) >> 14;
        int tz = (mv[6] * dx + mv[7] * dy + mv[8] * dz) >> 14;
        Fx_mul(mv, R, m->mm);

        int lmx = (R[0] * r->lx + R[3] * r->ly + R[6] * r->lz) >> 14;
        int lmy = (R[1] * r->lx + R[4] * r->ly + R[7] * r->lz) >> 14;
        int lmz = (R[2] * r->lx + R[5] * r->ly + R[8] * r->lz) >> 14;

        r->forceTex = tex;
        R3D_drawMesh(r, m->part[i], m->mm, tx, ty, tz, lmx, lmy, lmz, 0);
        r->forceTex = -1;
    }
}

void PedModel_free(PedModel* m) {
    if (!m) return;
    if (m->part) {
        for (int i = 0; i < m->n; i++) {
            if (m->part[i]) Mesh_free(m->part[i]);
        }
        free(m->part);
    }
    free(m->jx);
    free(m->jy);
    free(m->jz);
    free(m);
}
