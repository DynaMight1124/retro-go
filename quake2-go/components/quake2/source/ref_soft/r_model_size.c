// Sizing only: keep the existing loaders, resident data and hunk allocator.
#include "r_local.h"
#include "r_model_size.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

static qboolean ModelRange(int offset, int count, size_t stride, int length)
{
    return offset >= 0 && count >= 0 && offset <= length &&
        (uint64_t)(unsigned)count * stride <= (unsigned)(length - offset);
}

static qboolean ModelAdd(int *total, uint64_t bytes)
{
    if (bytes > INT_MAX - 31) return false;
    unsigned aligned = ((unsigned)bytes + 31u) & ~31u;
    if (aligned > (unsigned)(INT_MAX - *total)) return false;
    *total += (int)aligned;
    return true;
}

int Mod_ReservationSize(const void *data, int filelen)
{
    int ident, total = 0;
    if (!data || filelen < (int)sizeof(ident)) return 0;
    memcpy(&ident, data, sizeof(ident));
    ident = LittleLong(ident);
    if (ident == IDALIASHEADER) {
        dmdl_t h;
        if (filelen < (int)sizeof(h)) return 0;
        int fields[sizeof(h)/sizeof(int)];
        memcpy(fields, data, sizeof(fields));
        for (unsigned i = 0; i < sizeof(fields)/sizeof(fields[0]); ++i)
            fields[i] = LittleLong(fields[i]);
        memcpy(&h, fields, sizeof(h));
        if (h.version != ALIAS_VERSION || h.ofs_end < (int)sizeof(h) || h.ofs_end > filelen ||
            h.num_xyz <= 0 || h.num_xyz > MAX_VERTS || h.num_st <= 0 ||
            h.num_tris <= 0 || h.num_frames <= 0 || h.num_skins > MAX_MD2SKINS ||
            h.skinheight > MAX_LBM_HEIGHT || h.framesize < (int)offsetof(daliasframe_t, verts) +
            h.num_xyz * (int)sizeof(dtrivertx_t) ||
            !ModelRange(h.ofs_skins, h.num_skins, MAX_SKINNAME, h.ofs_end) ||
            !ModelRange(h.ofs_st, h.num_st, sizeof(dstvert_t), h.ofs_end) ||
            !ModelRange(h.ofs_tris, h.num_tris, sizeof(dtriangle_t), h.ofs_end) ||
            !ModelRange(h.ofs_frames, h.num_frames, (size_t)h.framesize, h.ofs_end) ||
            !ModelRange(h.ofs_glcmds, h.num_glcmds, sizeof(int), h.ofs_end)) return 0;
        return ModelAdd(&total, (unsigned)h.ofs_end) ? total : 0;
    }
    if (ident == IDSPRITEHEADER) {
        int header[3];
        if (filelen < (int)sizeof(header)) return 0;
        memcpy(header, data, sizeof(header));
        int frames = LittleLong(header[2]);
        if (LittleLong(header[1]) != SPRITE_VERSION || frames <= 0 || frames > MAX_MD2SKINS ||
            !ModelRange((int)offsetof(dsprite_t, frames), frames, sizeof(dsprframe_t), filelen)) return 0;
        return ModelAdd(&total, (unsigned)filelen) ? total : 0;
    }
    if (ident == IDBSPHEADER) {
        dheader_t h;
        if (filelen < (int)sizeof(h)) return 0;
        memcpy(&h, data, sizeof(h));
        if (LittleLong(h.version) != BSPVERSION) return 0;
        for (unsigned i = 0; i < HEADER_LUMPS; ++i) {
            h.lumps[i].fileofs = LittleLong(h.lumps[i].fileofs);
            h.lumps[i].filelen = LittleLong(h.lumps[i].filelen);
            if (!ModelRange(h.lumps[i].fileofs, h.lumps[i].filelen, 1, filelen)) return 0;
        }
        // One entry per existing Hunk_Alloc, including every skybox allowance.
        static const struct { int lump; size_t disk, resident; unsigned extra; } parts[] = {
            {LUMP_VERTEXES, sizeof(dvertex_t), sizeof(mvertex_t), 8},
            {LUMP_EDGES, sizeof(dedge_t), sizeof(medge_t), 13},
            {LUMP_SURFEDGES, sizeof(int), sizeof(int), 24},
            {LUMP_LIGHTING, 3, 1, 0},
            {LUMP_PLANES, sizeof(dplane_t), sizeof(mplane_t), 6},
            {LUMP_TEXINFO, sizeof(texinfo_t), sizeof(mtexinfo_t), 6},
            {LUMP_FACES, sizeof(dface_t), sizeof(msurface_t), 6},
            {LUMP_LEAFFACES, sizeof(short), sizeof(msurface_t *), 0},
            {LUMP_VISIBILITY, 1, 1, 0},
            {LUMP_LEAFS, sizeof(dleaf_t), sizeof(mleaf_t), 0},
            {LUMP_NODES, sizeof(dnode_t), sizeof(mnode_t), 0},
            {LUMP_MODELS, sizeof(dmodel_t), sizeof(dmodel_t), 0}
        };
        for (unsigned i = 0; i < sizeof(parts)/sizeof(parts[0]); ++i) {
            unsigned length = (unsigned)h.lumps[parts[i].lump].filelen;
            if (length % parts[i].disk) return 0;
            uint64_t count = length / parts[i].disk + parts[i].extra;
            if (!ModelAdd(&total, count * parts[i].resident)) return 0;
        }
        return total;
    }
    return 0;
}
