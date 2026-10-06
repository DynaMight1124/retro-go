/* Frozen pre-scanline rasterizer: host regression oracle only. */
static void reference_raster_triangle(const RGVertex vertices[3])
{
    if (!sSurface || !sColorTarget)
        return;
    float x[3], y[3];
    for (unsigned i = 0; i < 3; ++i) {
        const float w = vertices[i].w;
        x[i] = (vertices[i].x / w + 1.0f) *
               (MK64_SCREEN_WIDTH * 0.5f);
        y[i] = (1.0f - vertices[i].y / w) *
               (MK64_SCREEN_HEIGHT * 0.5f);
        if (!isfinite(x[i]) || !isfinite(y[i]) ||
            x[i] < -10000.0f || x[i] > 10000.0f ||
            y[i] < -10000.0f || y[i] > 10000.0f) {
            ++sBadGeometry;
            return;
        }
    }
    if (sTraceGeometry && sTriangles == 1)
        RG_LOGI("MK64 first triangle screen: (%.1f,%.1f) (%.1f,%.1f) "
                "(%.1f,%.1f)", (double)x[0], (double)y[0],
                (double)x[1], (double)y[1], (double)x[2], (double)y[2]);
    float area = triangle_edge(x[0], y[0], x[1], y[1], x[2], y[2]);
    if (area > -0.01f && area < 0.01f)
        return;
    unsigned cull = sGeometryMode & G_CULL_BOTH;
    /* triangle_edge has the opposite sign to the N64 winding cross. */
    if (cull == G_CULL_BOTH ||
        (cull == G_CULL_BACK && area <= 0.0f) ||
        (cull == G_CULL_FRONT && area >= 0.0f)) {
        ++sCulledTriangles;
        return;
    }
    float min_x = fminf(x[0], fminf(x[1], x[2]));
    float max_x = fmaxf(x[0], fmaxf(x[1], x[2]));
    float min_y = fminf(y[0], fminf(y[1], y[2]));
    float max_y = fmaxf(y[0], fmaxf(y[1], y[2]));
    if (min_x >= MK64_SCREEN_WIDTH || max_x < 0 ||
        min_y >= MK64_SCREEN_HEIGHT || max_y < 0)
        return;
    int left = (int)fmaxf(0.0f, floorf(min_x));
    int right = (int)fminf(MK64_SCREEN_WIDTH - 1, ceilf(max_x));
    int top = (int)fmaxf(0.0f, floorf(min_y));
    int bottom = (int)fminf(MK64_SCREEN_HEIGHT - 1, ceilf(max_y));
    unsigned red = (vertices[0].red + vertices[1].red + vertices[2].red) / 3;
    unsigned green = (vertices[0].green + vertices[1].green + vertices[2].green) / 3;
    unsigned blue = (vertices[0].blue + vertices[1].blue + vertices[2].blue) / 3;
    unsigned texture_stride = sLoadedTile
        ? (sLoadedStride << sLoadedSize) >> sTextureSize : sTextureWidth;
    unsigned texture_pixels = sTextureHeight
        ? (sTextureHeight - 1) * texture_stride + sTextureWidth : 0;
    if (sLoadedTile) texture_pixels += sLoadedNibbleOffset;
    unsigned texture_bytes = (texture_pixels * (4u << sTextureSize) + 7) / 8;
    uint16_t first_texel;
    unsigned first_alpha;
    unsigned combiner_mode = triangle_combiner_fast_mode();
    /* Direct shade/primitive/environment equations cannot depend on a stale
     * enabled texture. Keep sampling whenever RGB or alpha needs TEXEL0/1,
     * and conservatively for every general or modulate equation. */
    unsigned rgb_source = sTriangleCombiner[1].rgb[3];
    unsigned alpha_source = sTriangleCombiner[1].alpha[3];
    bool needs_texture = combiner_mode != COMBINE_DIRECT ||
                         rgb_source == 1 || rgb_source == 2 ||
                         alpha_source == 1 || alpha_source == 2;
    bool textured = needs_texture && sTextureEnabled && sTextureImage &&
                    sTextureWidth && sTextureHeight &&
                    sTextureWidth <= 512 && sTextureHeight <= 512 &&
                    readable_data(sLoadedTile ? sLoadedImage : (const uint8_t *)sTextureImage, texture_bytes) &&
                    sample_triangle_texture(0, &first_texel, &first_alpha);
    bool gradient = vertices[0].red != vertices[1].red || vertices[0].red != vertices[2].red ||
                    vertices[0].green != vertices[1].green || vertices[0].green != vertices[2].green ||
                      vertices[0].blue != vertices[1].blue || vertices[0].blue != vertices[2].blue;
    if (sTraceAssets && sSpriteTraces < 40 &&
        (sTextureFormat == G_IM_FMT_CI || sTextureFormat == G_IM_FMT_I ||
         sTextureFormat == G_IM_FMT_IA)) {
        ++sSpriteTraces;
        RG_LOGI("MK64 sprite %u: image %p %ux%u fmt %u size %u enable %u sample %u "
                "palette %u bank %u scale %.5f,%.5f UV %.2f,%.2f %.2f,%.2f %.2f,%.2f "
                "prim %08x env %08x cycle %u",
                sSpriteTraces, sTextureImage, sTextureWidth, sTextureHeight,
                sTextureFormat, sTextureSize, sTextureEnabled, textured,
                sPaletteEntries, sTexturePaletteBank, sTextureScaleS, sTextureScaleT,
                vertices[0].u, vertices[0].v, vertices[1].u, vertices[1].v,
                vertices[2].u, vertices[2].v, (unsigned)sPrimitiveColor,
                (unsigned)sEnvironmentColor, sCycleType);
        RG_LOGI("MK64 sprite mux: RGB %u,%u,%u,%u / %u,%u,%u,%u alpha %u,%u,%u,%u / %u,%u,%u,%u",
                sTriangleCombiner[0].rgb[0], sTriangleCombiner[0].rgb[1],
                sTriangleCombiner[0].rgb[2], sTriangleCombiner[0].rgb[3],
                sTriangleCombiner[1].rgb[0], sTriangleCombiner[1].rgb[1],
                sTriangleCombiner[1].rgb[2], sTriangleCombiner[1].rgb[3],
                sTriangleCombiner[0].alpha[0], sTriangleCombiner[0].alpha[1],
                sTriangleCombiner[0].alpha[2], sTriangleCombiner[0].alpha[3],
                sTriangleCombiner[1].alpha[0], sTriangleCombiner[1].alpha[1],
                sTriangleCombiner[1].alpha[2], sTriangleCombiner[1].alpha[3]);
        if (sTextureImage && readable_data(sTextureImage, 4)) {
            const uint8_t *bytes = (const uint8_t *)sTextureImage;
            RG_LOGI("MK64 sprite data: %02x %02x %02x %02x palette[0,1] %04x,%04x",
                    bytes[0], bytes[1], bytes[2], bytes[3],
                    sTexturePalette[0], sTexturePalette[1]);
        }
    }
    bool depth_test = sDepth && (sGeometryMode & G_ZBUFFER) &&
                      (sOtherModeL & Z_CMP);
    bool depth_write = depth_test && (sOtherModeL & Z_UPD);
    bool translucent = (sOtherModeL & FORCE_BL) && !depth_write;
    if (textured) ++sTexturedTriangles;
    if (depth_test) ++sDepthTriangles;
    if (translucent) ++sBlendedTriangles;
    float inverse_w[3], z[3], u[3], v[3];
    for (unsigned i = 0; i < 3; ++i) {
        inverse_w[i] = 1.0f / vertices[i].w;
        z[i] = vertices[i].z * inverse_w[i];
        u[i] = vertices[i].u * inverse_w[i];
        v[i] = vertices[i].v * inverse_w[i];
    }
    float inverse_area = 1.0f / area;
    uint16_t *pixels = sSurface->data;
    unsigned written = 0;
    int64_t pixel_started = sProfileFrame ? rg_system_timer() : 0;
    if (sProfileFrame)
        sBoxPixels += (unsigned)(right - left + 1) * (unsigned)(bottom - top + 1);
    for (int py = top; py <= bottom; ++py)
        for (int px = left; px <= right; ++px) {
            float sample_x = px + 0.5f, sample_y = py + 0.5f;
            float e0 = triangle_edge(x[0], y[0], x[1], y[1], sample_x, sample_y);
            float e1 = triangle_edge(x[1], y[1], x[2], y[2], sample_x, sample_y);
            float e2 = triangle_edge(x[2], y[2], x[0], y[0], sample_x, sample_y);
            if ((area > 0 && e0 >= 0 && e1 >= 0 && e2 >= 0) ||
                (area < 0 && e0 <= 0 && e1 <= 0 && e2 <= 0)) {
                float b0 = e1 * inverse_area, b1 = e2 * inverse_area, b2 = e0 * inverse_area;
                unsigned pixel_index = py * MK64_SCREEN_WIDTH + px;
                /* Reject occluded fragments before texture fetch and combiner.
                 * Delay the write until final alpha is known: sprite holes
                 * must not hide geometry drawn later. */
                uint16_t depth = 0;
                if (depth_test) {
                    float ndc_z = b0 * z[0] + b1 * z[1] + b2 * z[2];
                    if (ndc_z < -1.0f) ndc_z = -1.0f;
                    if (ndc_z > 1.0f) ndc_z = 1.0f;
                    depth = (uint16_t)((ndc_z + 1.0f) * 32767.0f);
                    if (depth >= sDepth[pixel_index]) {
                        if (sProfileFrame) ++sEarlyDepthRejects;
                        continue;
                    }
                }
                unsigned rr = red, gg = green, bb = blue;
                unsigned shade_alpha = (unsigned)(b0 * vertices[0].alpha + b1 * vertices[1].alpha + b2 * vertices[2].alpha);
                uint16_t tex_color = 0xffff;
                unsigned alpha = 255;
                if (gradient) {
                    rr = (unsigned)(b0 * vertices[0].red + b1 * vertices[1].red + b2 * vertices[2].red);
                    gg = (unsigned)(b0 * vertices[0].green + b1 * vertices[1].green + b2 * vertices[2].green);
                    bb = (unsigned)(b0 * vertices[0].blue + b1 * vertices[1].blue + b2 * vertices[2].blue);
                }
                if (textured) {
                    float reciprocal = b0 * inverse_w[0] +
                                       b1 * inverse_w[1] + b2 * inverse_w[2];
                    if (reciprocal <= 0.0f) continue;
                    int tx = (int)floorf((b0 * u[0] + b1 * u[1] +
                                          b2 * u[2]) / reciprocal);
                    int ty = (int)floorf((b0 * v[0] + b1 * v[1] +
                                          b2 * v[2]) / reciprocal);
                    tx = triangle_texture_coordinate(tx, sTextureWidth, sTextureModeS);
                    ty = triangle_texture_coordinate(ty, sTextureHeight, sTextureModeT);
                    if (!sample_triangle_texture(ty * sTextureWidth + tx, &tex_color, &alpha)) {
                        if (sProfileFrame) ++sTextureSampleFailures;
                        continue;
                    }
                }
                uint16_t shade = combine_triangle_pixel_fast(combiner_mode, tex_color, alpha, rr, gg, bb, shade_alpha, &alpha);
                if (sProfileFrame && combiner_mode != COMBINE_GENERAL) ++sFastPixels;
                if (!alpha) {
                    if (sProfileFrame) ++sAlphaRejects;
                    continue;
                }
                if (depth_write) sDepth[pixel_index] = depth;
                pixels[pixel_index] = alpha < 255 ? blend_565(shade, pixels[pixel_index], alpha) : shade;
                ++written;
            }
        }
    if (sProfileFrame) {
        sPixelUs += (uint32_t)(rg_system_timer() - pixel_started);
        sWrittenPixels += written;
    }
    if (written)
        ++sVisibleTriangles;
}
