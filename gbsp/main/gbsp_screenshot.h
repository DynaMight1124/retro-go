#pragma once
#include <rg_surface.h>
#include <stdio.h>
#include <stdlib.h>

/* PNG with stored DEFLATE blocks: one converted scanline at a time. Avoid the
 * general encoder's 256 KiB hash table and full-image intermediate buffers. */
typedef struct { FILE *file; uint32_t crc; bool ok; } gp_png_writer_t;

static void gp_png_write(gp_png_writer_t *w, const void *data, size_t size)
{
    if (w->ok && size && fwrite(data, 1, size, w->file) != size) w->ok = false;
}

static void gp_png_u32(gp_png_writer_t *w, uint32_t value)
{
    const uint8_t bytes[4] = {value >> 24, value >> 16, value >> 8, value};
    gp_png_write(w, bytes, sizeof(bytes));
}

static void gp_png_data(gp_png_writer_t *w, const uint8_t *data, size_t size)
{
    gp_png_write(w, data, size);
    for (size_t i = 0; w->ok && i < size; i++) {
        w->crc ^= data[i];
        for (int bit = 0; bit < 8; bit++)
            w->crc = (w->crc >> 1) ^ (0xEDB88320u & (0u - (w->crc & 1u)));
    }
}

static void gp_png_begin(gp_png_writer_t *w, const char *type, uint32_t size)
{
    gp_png_u32(w, size);
    w->crc = 0xFFFFFFFFu;
    gp_png_data(w, (const uint8_t *)type, 4);
}

static void gp_png_end(gp_png_writer_t *w)
{
    gp_png_u32(w, w->crc ^ 0xFFFFFFFFu);
}

static bool gp_screenshot_save(const rg_surface_t *source, const char *filename, int width, int height)
{
    if (!source || !source->data || source->width <= 0 || source->height <= 0) return false;
    /* A zero dimension requests proportional scaling, just like rg_surface. */
    if (width <= 0 && height <= 0) width = source->width, height = source->height;
    else if (width <= 0) width = (int)((int64_t)source->width * height / source->height);
    else if (height <= 0) height = (int)((int64_t)source->height * width / source->width);
    /* Bound rg_surface_copy's stack-side x-map to the native GBA width. */
    if (width <= 0 || width > 240 || height <= 0) return false;
    const unsigned row_size = 1 + (unsigned)width * 3;
    const uint64_t idat_size = 6 + (uint64_t)height * (row_size + 5);
    if (idat_size > 0x7FFFFFFFu) return false;

    uint8_t *row = rg_alloc(row_size, MEM_SLOW | MEM_NOPANIC);
    if (!row) return false;
    FILE *file = fopen(filename, "wb");
    if (!file) { free(row); return false; }
    gp_png_writer_t writer = {.file=file, .ok=true};
    const uint8_t signature[8] = {137,80,78,71,13,10,26,10};
    const uint8_t ihdr[13] = {width >> 24, width >> 16, width >> 8, width,
        height >> 24, height >> 16, height >> 8, height, 8,2,0,0,0};
    gp_png_write(&writer, signature, sizeof(signature));
    gp_png_begin(&writer, "IHDR", sizeof(ihdr));
    gp_png_data(&writer, ihdr, sizeof(ihdr));
    gp_png_end(&writer);
    gp_png_begin(&writer, "IDAT", (uint32_t)idat_size);
    const uint8_t zlib_header[2] = {0x78,0x01};
    gp_png_data(&writer, zlib_header, sizeof(zlib_header));
    uint32_t s1 = 1, s2 = 0;
    rg_surface_t scanline = {.width=width, .height=1, .stride=width*3,
        .format=RG_PIXEL_888, .data=row+1};
    row[0] = 0; /* PNG filter None */
    for (int y = 0; writer.ok && y < height; y++) {
        const rg_rect_t rect = {0, (int)((int64_t)y * source->height / height), source->width, 1};
        if (!rg_surface_copy(source, &rect, &scanline, NULL, true)) { writer.ok=false; break; }
        const uint16_t inverse = (uint16_t)~row_size;
        const uint8_t block[5] = {y == height-1 ? 1 : 0,
            row_size, row_size >> 8, inverse, inverse >> 8};
        gp_png_data(&writer, block, sizeof(block));
        gp_png_data(&writer, row, row_size);
        for (unsigned i = 0; i < row_size; i++) {
            s1 = (s1 + row[i]) % 65521u;
            s2 = (s2 + s1) % 65521u;
        }
    }
    const uint32_t adler = (s2 << 16) | s1;
    const uint8_t checksum[4] = {adler >> 24, adler >> 16, adler >> 8, adler};
    gp_png_data(&writer, checksum, sizeof(checksum));
    gp_png_end(&writer);
    gp_png_begin(&writer, "IEND", 0);
    gp_png_end(&writer);
    if (fclose(file) != 0) writer.ok = false;
    free(row);
    if (!writer.ok) {
        remove(filename); /* do not leave a partial thumbnail for the launcher */
        RG_LOGE("Screenshot PNG write failed: %s", filename);
    }
    return writer.ok;
}
