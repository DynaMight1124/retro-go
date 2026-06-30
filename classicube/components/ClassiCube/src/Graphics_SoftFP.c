#include "Core.h"
#if CC_GFX_BACKEND == CC_GFX_BACKEND_SOFTFP
#define CC_DYNAMIC_VBS_ARE_STATIC
#define OVERRIDE_BEGEND2D_FUNCTIONS
#include "_GraphicsBase.h"
#include "Errors.h"
#include "Window.h"
#include "Math.h"
#include <limits.h>
#include <stdint.h>
#include <rg_system.h>


// 16.16 fixed point
#define FP_SHIFT 16
#define FP_ONE (1 << FP_SHIFT)
#define FP_HALF (FP_ONE >> 1)
#define FP_MASK ((1 << FP_SHIFT) - 1)

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define FloatToFixed(x) ((int)((x) * FP_ONE))
#define FixedToFloat(x) ((float)(x) / FP_ONE)
#define FixedToInt(x) ((x) >> FP_SHIFT)
#define IntToFixed(x) ((x) << FP_SHIFT)

#define FixedMul(a, b) (((int64_t)(a) * (b)) >> FP_SHIFT)
#define FixedDiv(a, b) (((int64_t)(a) << FP_SHIFT) / (b))

static int FixedReciprocal(int x) {
    if (x == 0) return 0;
    return (int)(4294967296.0f / (float)x);
}

static cc_bool faceCulling;
static int fb_width, fb_height; 
static struct Bitmap fb_bmp;
static int vp_hwidth_fp, vp_hheight_fp;
static int fb_maxX, fb_maxY;

static BitmapCol* colorBuffer;
static BitmapCol clearColor;
static cc_bool colWrite = true;
static int cb_stride;

static int* depthBuffer;
static cc_bool depthTest  = true;
static cc_bool depthWrite = true;
static int db_stride;

static void* gfx_vertices;
static GfxResourceID white_square;

typedef struct FixedMatrix {
    int m[4][4];
} FixedMatrix;

typedef struct VertexFixed {
    int x, y, z, w;
    int u, v;
    BitmapCol c;
    cc_uint16 pad;
} VertexFixed;

static int tex_offseting;
static int texOffsetX_fp, texOffsetY_fp; // Fixed point texture offsets
static FixedMatrix _view_fp, _proj_fp, _mvp_fp;

static void Gfx_RestoreState(void) {
    InitDefaultResources();

    struct Bitmap bmp;
    BitmapCol pixels[1] = { BITMAPCOLOR_WHITE };
    Bitmap_Init(bmp, 1, 1, pixels);
    white_square = Gfx_CreateTexture(&bmp, 0, false);
}

static void Gfx_FreeState(void) {
    FreeDefaultResources();
    Gfx_DeleteTexture(&white_square);
}

void Gfx_Create(void) {
    Gfx.MaxTexWidth  = 4096;
    Gfx.MaxTexHeight = 4096;
    Gfx.Created      = true;
    Gfx.BackendType  = CC_GFX_BACKEND_SOFTGPU;
    Gfx.Limitations  = GFX_LIMIT_MINIMAL;
    
    Gfx_RestoreState();
}

static void DestroyBuffers(void) {
    Window_FreeFramebuffer(&fb_bmp);
    Mem_Free(depthBuffer);
    depthBuffer = NULL;
}

void Gfx_Free(void) { 
    Gfx_FreeState();
    DestroyBuffers();
}

typedef struct CCTexture {
    unsigned short width, height;
    BitmapCol pixels[];
} CCTexture;

static CCTexture* curTexture;
static BitmapCol* curTexPixels;
static int curTexWidth, curTexHeight;
static int texWidthMask, texHeightMask;
static int texSinglePixel;
        
void Gfx_BindTexture(GfxResourceID texId) {
    if (!texId) texId = white_square;
    CCTexture* tex = (CCTexture*)texId;

    curTexture   = tex;
    curTexPixels = tex->pixels;
    curTexWidth  = tex->width;
    curTexHeight = tex->height;

    texWidthMask   = (1 << Math_ilog2(tex->width))  - 1;
    texHeightMask  = (1 << Math_ilog2(tex->height)) - 1;

    texSinglePixel = curTexWidth == 1;
}
        
void Gfx_DeleteTexture(GfxResourceID* texId) {
    GfxResourceID data = *texId;
    if (data) Mem_Free(data);
    *texId = NULL;
}
        
GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
    CCTexture* tex = (CCTexture*)Mem_TryAlloc(sizeof(CCTexture) + bmp->width * bmp->height * BITMAPCOLOR_SIZE, 1);
	if (!tex) return NULL;

    tex->width  = bmp->width;
    tex->height = bmp->height;

    CopyPixels(tex->pixels, bmp->width * BITMAPCOLOR_SIZE,
               bmp->scan0,  rowWidth * BITMAPCOLOR_SIZE,
               bmp->width,  bmp->height);
    return tex;
}

void Gfx_UpdateTexture(GfxResourceID texId, int x, int y, struct Bitmap* part, int rowWidth, cc_bool mipmaps) {
    CCTexture* tex = (CCTexture*)texId;
    BitmapCol* dst = (tex->pixels + x) + y * tex->width;

    CopyPixels(dst,         tex->width * BITMAPCOLOR_SIZE,
               part->scan0, rowWidth   * BITMAPCOLOR_SIZE,
               part->width, part->height);
}

void Gfx_EnableMipmaps(void)  { }
void Gfx_DisableMipmaps(void) { }


/*########################################################################################################################*
*--------------------------------------------------------2D drawing-------------------------------------------------------*
*#########################################################################################################################*/
void Gfx_Begin2D(int width, int height) {
	gfx_rendering2D = true;
	Gfx_SetDepthTest(false);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaBlending(true);
}

void Gfx_End2D(void) {
	gfx_rendering2D = false;
	Gfx_SetDepthTest(true);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);
}

/*########################################################################################################################*
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled)    { }
void Gfx_SetFogCol(PackedCol col)   { }
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value)     { }
void Gfx_SetFogMode(FogFunc func)   { }

void Gfx_SetFaceCulling(cc_bool enabled) { faceCulling = enabled; }
static void SetAlphaTest(cc_bool enabled) { }
static void SetAlphaBlend(cc_bool enabled) { }
void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void ClearColorBuffer(void) {
    int x, y;
    for (y = 0; y < fb_height; y++) {
        BitmapCol* row = colorBuffer + y * cb_stride;
        for (x = 0; x < fb_width; x++) row[x] = clearColor;
    }
}

static void ClearDepthBuffer(void) {
    int i, size = fb_width * fb_height;
    int maxDepth = 0x7FFFFFFF; 
    for (i = 0; i < size; i++) depthBuffer[i] = maxDepth;
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
    if (buffers & GFX_BUFFER_COLOR) ClearColorBuffer();
    if (buffers & GFX_BUFFER_DEPTH) ClearDepthBuffer();
}

void Gfx_ClearColor(PackedCol color) {
    clearColor = BitmapCol_Make(PackedCol_R(color), PackedCol_G(color), PackedCol_B(color), PackedCol_A(color));
}

void Gfx_SetDepthTest(cc_bool enabled) { depthTest = enabled; }
void Gfx_SetDepthWrite(cc_bool enabled) { depthWrite = enabled; }
static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) { }
void Gfx_DepthOnlyRendering(cc_bool depthOnly) { colWrite = !depthOnly; }

/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) { return (void*)1; }
void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }

/*########################################################################################################################*
*-------------------------------------------------------Vertex buffers----------------------------------------------------*
*#########################################################################################################################*/
struct FPVertexColoured { int x, y, z; BitmapCol c; cc_uint16 pad; };
struct FPVertexTextured { int x, y, z; BitmapCol c; cc_uint16 pad; int u, v; };

static VertexFormat buf_fmt;
static int buf_count;

static void PreprocessTexturedVertices(void* vertices) {
	struct FPVertexTextured* dst = (struct FPVertexTextured*)vertices;
	struct VertexTextured* src   = (struct VertexTextured*)vertices;
	for (int i = 0; i < buf_count; i++) {
		float sx = src[i].x, sy = src[i].y, sz = src[i].z;
		float su = src[i].U, sv = src[i].V;
		PackedCol sc = src[i].Col;
		dst[i].x = FloatToFixed(sx); dst[i].y = FloatToFixed(sy); dst[i].z = FloatToFixed(sz);
		dst[i].u = FloatToFixed(su); dst[i].v = FloatToFixed(sv);
		dst[i].c = BitmapCol_Make(PackedCol_R(sc), PackedCol_G(sc), PackedCol_B(sc), PackedCol_A(sc));
	}
}

static void PreprocessColouredVertices(void* vertices) {
	struct FPVertexColoured* dst = (struct FPVertexColoured*)vertices;
	struct VertexColoured* src   = (struct VertexColoured*)vertices;
	for (int i = 0; i < buf_count; i++) {
		float sx = src[i].x, sy = src[i].y, sz = src[i].z;
		PackedCol sc = src[i].Col;
		dst[i].x = FloatToFixed(sx); dst[i].y = FloatToFixed(sy); dst[i].z = FloatToFixed(sz);
		dst[i].c = BitmapCol_Make(PackedCol_R(sc), PackedCol_G(sc), PackedCol_B(sc), PackedCol_A(sc));
	}
}

static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) { return Mem_TryAlloc(count, strideSizes[fmt]); }
void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb; }
void Gfx_DeleteVb(GfxResourceID* vb) { GfxResourceID data = *vb; if (data) Mem_Free(data); *vb = 0; }
void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) { buf_fmt = fmt; buf_count = count; return vb; }
void Gfx_UnlockVb(GfxResourceID vb) { 
    if (buf_fmt == VERTEX_FORMAT_TEXTURED) PreprocessTexturedVertices(vb);
    else PreprocessColouredVertices(vb);
}

/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static void MatrixToFixed(FixedMatrix* dst, const struct Matrix* src) {
    dst->m[0][0] = FloatToFixed(src->row1.x); dst->m[0][1] = FloatToFixed(src->row1.y); dst->m[0][2] = FloatToFixed(src->row1.z); dst->m[0][3] = FloatToFixed(src->row1.w);
    dst->m[1][0] = FloatToFixed(src->row2.x); dst->m[1][1] = FloatToFixed(src->row2.y); dst->m[1][2] = FloatToFixed(src->row2.z); dst->m[1][3] = FloatToFixed(src->row2.w);
    dst->m[2][0] = FloatToFixed(src->row3.x); dst->m[2][1] = FloatToFixed(src->row3.y); dst->m[2][2] = FloatToFixed(src->row3.z); dst->m[2][3] = FloatToFixed(src->row3.w);
    dst->m[3][0] = FloatToFixed(src->row4.x); dst->m[3][1] = FloatToFixed(src->row4.y); dst->m[3][2] = FloatToFixed(src->row4.z); dst->m[3][3] = FloatToFixed(src->row4.w);
}

static void MatrixMulFixed(FixedMatrix* dst, const FixedMatrix* a, const FixedMatrix* b) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int64_t sum = 0;
            for (int k = 0; k < 4; k++) sum += (int64_t)a->m[i][k] * b->m[k][j];
            dst->m[i][j] = (int)(sum >> FP_SHIFT);
        }
    }
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
    MatrixToFixed(&_view_fp, view); MatrixToFixed(&_proj_fp, proj);
    MatrixMulFixed(&_mvp_fp, &_view_fp, &_proj_fp);
    Matrix_Mul(mvp, view, proj);
}

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
    if (type == MATRIX_VIEW) MatrixToFixed(&_view_fp, matrix);
    if (type == MATRIX_PROJ) MatrixToFixed(&_proj_fp, matrix);
    MatrixMulFixed(&_mvp_fp, &_view_fp, &_proj_fp);
}

void Gfx_EnableTextureOffset(float x, float y) { tex_offseting = true; texOffsetX_fp = FloatToFixed(x); texOffsetY_fp = FloatToFixed(y); }
void Gfx_DisableTextureOffset(void) { tex_offseting = false; }

static CC_NOINLINE void ShiftTextureCoords(int count) {
	for (int i = 0; i < count; i++) {
		struct FPVertexTextured* v = (struct FPVertexTextured*)gfx_vertices + i;
		v->u += texOffsetX_fp; v->v += texOffsetY_fp;
	}
}
static CC_NOINLINE void UnshiftTextureCoords(int count) {
	for (int i = 0; i < count; i++) {
		struct FPVertexTextured* v = (struct FPVertexTextured*)gfx_vertices + i;
		v->u -= texOffsetX_fp; v->v -= texOffsetY_fp;
	}
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
    *matrix = Matrix_Identity;
    matrix->row1.x = 2.0f/width; matrix->row2.y = -2.0f/height; matrix->row3.z = 1.0f/(zNear-zFar);
    matrix->row4.x = -1.0f; matrix->row4.y = 1.0f; matrix->row4.z = zNear/(zNear-zFar);
}

void Gfx_CalcPerspectiveMatrix(struct Matrix* m, float fov, float aspect, float zFar) {
    float zNear = 0.1f, c = Cotangent(0.5f * fov);
    *m = Matrix_Identity;
    m->row1.x = c/aspect; m->row2.y = c; m->row3.z = zFar/(zNear-zFar); m->row3.w = -1.0f; m->row4.z = (zNear*zFar)/(zNear-zFar); m->row4.w = 0.0f;
}

/*########################################################################################################################*
*---------------------------------------------------------Rendering-------------------------------------------------------*
*#########################################################################################################################*/
static void TransformVertex2D(int index, VertexFixed* vertex) {
    if (gfx_format != VERTEX_FORMAT_TEXTURED) {
        struct FPVertexColoured* v = (struct FPVertexColoured*)gfx_vertices + index;
        vertex->x = v->x; vertex->y = v->y; vertex->c = v->c;
        vertex->u = 0; vertex->v = 0;
    } else {
        struct FPVertexTextured* v = (struct FPVertexTextured*)gfx_vertices + index;
        vertex->x = v->x; vertex->y = v->y; vertex->c = v->c;
        vertex->u = v->u; vertex->v = v->v;
    }
}

static int TransformVertex3D(int index, VertexFixed* vertex) {
    if (gfx_format != VERTEX_FORMAT_TEXTURED) {
        struct FPVertexColoured* v = (struct FPVertexColoured*)gfx_vertices + index;
        vertex->x = v->x; vertex->y = v->y; vertex->z = v->z; vertex->c = v->c;
        vertex->u = 0; vertex->v = 0;
    } else {
        struct FPVertexTextured* v = (struct FPVertexTextured*)gfx_vertices + index;
        vertex->x = v->x; vertex->y = v->y; vertex->z = v->z; vertex->c = v->c;
        vertex->u = v->u; vertex->v = v->v;
    }
    int px = vertex->x, py = vertex->y, pz = vertex->z;
    int64_t xt = (int64_t)px * _mvp_fp.m[0][0] + (int64_t)py * _mvp_fp.m[1][0] + (int64_t)pz * _mvp_fp.m[2][0] + ((int64_t)_mvp_fp.m[3][0] << FP_SHIFT);
    int64_t yt = (int64_t)px * _mvp_fp.m[0][1] + (int64_t)py * _mvp_fp.m[1][1] + (int64_t)pz * _mvp_fp.m[2][1] + ((int64_t)_mvp_fp.m[3][1] << FP_SHIFT);
    int64_t zt = (int64_t)px * _mvp_fp.m[0][2] + (int64_t)py * _mvp_fp.m[1][2] + (int64_t)pz * _mvp_fp.m[2][2] + ((int64_t)_mvp_fp.m[3][2] << FP_SHIFT);
    int64_t wt = (int64_t)px * _mvp_fp.m[0][3] + (int64_t)py * _mvp_fp.m[1][3] + (int64_t)pz * _mvp_fp.m[2][3] + ((int64_t)_mvp_fp.m[3][3] << FP_SHIFT);
    vertex->x = (int)(xt >> FP_SHIFT); vertex->y = (int)(yt >> FP_SHIFT); vertex->z = (int)(zt >> FP_SHIFT); vertex->w = (int)(wt >> FP_SHIFT);
    return 1;
}

static int vp_x_fp, vp_y_fp;
static cc_bool ViewportVertex3D(VertexFixed* vertex) {
    if (vertex->w == 0) return false;
    int invW = FixedReciprocal(vertex->w);
    int64_t x_ndc = ((int64_t)vertex->x * invW) >> FP_SHIFT;
    int64_t y_ndc = ((int64_t)vertex->y * invW) >> FP_SHIFT;
    int64_t z_ndc = ((int64_t)vertex->z * invW) >> FP_SHIFT;
    vertex->x = vp_x_fp + vp_hwidth_fp + ((x_ndc * vp_hwidth_fp) >> FP_SHIFT);
    vertex->y = vp_y_fp + vp_hheight_fp - ((y_ndc * vp_hheight_fp) >> FP_SHIFT);
    vertex->z = (int)(((int64_t)z_ndc + FP_ONE) * (0x7FFFFFFF / (2 * FP_ONE)));
    vertex->w = invW; vertex->u = FixedMul(vertex->u, invW); vertex->v = FixedMul(vertex->v, invW);
    return true;
}

static void DrawSprite2D(VertexFixed* V0, VertexFixed* V1, VertexFixed* V2) {
    int minX = max(FixedToInt(V0->x), 0), minY = max(FixedToInt(V0->y), 0);
    int maxX = min(FixedToInt(V1->x), fb_maxX), maxY = min(FixedToInt(V2->y), fb_maxY);
    if (maxX < minX || maxY < minY) return;
    int bTX = FixedMul(V0->u, IntToFixed(curTexWidth)) >> FP_SHIFT, bTY = FixedMul(V0->v, IntToFixed(curTexHeight)) >> FP_SHIFT;
    int dTX = (FixedMul(V1->u, IntToFixed(curTexWidth)) >> FP_SHIFT) - bTX, dTY = (FixedMul(V2->v, IntToFixed(curTexHeight)) >> FP_SHIFT) - bTY;
    int w = maxX-minX, h = maxY-minY; if (w<=0 || h<=0) return;
    for (int y=minY; y<=maxY; y++) {
        int tY = (bTY + (dTY * (y-minY) / h)) & texHeightMask;
        for (int x=minX; x<=maxX; x++) {
            BitmapCol t = curTexPixels[tY * curTexWidth + ((bTX + (dTX * (x-minX) / w)) & texWidthMask)];
            if (gfx_alphaBlend && BitmapCol_A(t) == 0) continue;
            colorBuffer[y * cb_stride + x] = t;
        }
    }
}

#define edgeFunctionFixed(ax,ay, bx,by, cx,cy) \
    ((int64_t)((bx) - (ax)) * ((cy) - (ay)) - (int64_t)((by) - (ay)) * ((cx) - (ax)))

static int diag_3d_pixels_written;
static int diag_3d_tris_called;
static int diag_3d_tris_area_zero;
static int diag_3d_tris_clipped;
static int diag_3d_pixels_inside;
static int diag_3d_depth_fail;
static int diag_3d_alpha_fail;

static void DrawTriangle3D(VertexFixed* V0, VertexFixed* V1, VertexFixed* V2) {
    diag_3d_tris_called++;
    int minX = FixedToInt(min(V0->x, min(V1->x, V2->x))), minY = FixedToInt(min(V0->y, min(V1->y, V2->y)));
    int maxX = FixedToInt(max(V0->x, max(V1->x, V2->x))), maxY = FixedToInt(max(V0->y, max(V1->y, V2->y)));
    if (maxX < 0 || minX > fb_maxX || maxY < 0 || minY > fb_maxY) return;
    minX = max(minX, 0); maxX = min(maxX, fb_maxX); minY = max(minY, 0); maxY = min(maxY, fb_maxY);
    int64_t area = edgeFunctionFixed(V0->x, V0->y, V1->x, V1->y, V2->x, V2->y);
    if (area == 0) return;
    if (area < 0) {
        if (faceCulling) return;
        VertexFixed* t = V1; V1 = V2; V2 = t;
        area = -area;
    }

    int32_t area_div = (int32_t)(area >> 18);
    if (area_div == 0) { diag_3d_tris_area_zero++; return; }

    int32_t inv_area = (int32_t)((1LL << 32) / area_div);

    int w0=V0->w, w1=V1->w, w2=V2->w, z0=V0->z, z1=V1->z, z2=V2->z;
    BitmapCol color = V0->c;
    int u0=FixedMul(V0->u, IntToFixed(curTexWidth)), v0=FixedMul(V0->v, IntToFixed(curTexHeight));
    int u1=FixedMul(V1->u, IntToFixed(curTexWidth)), v1=FixedMul(V1->v, IntToFixed(curTexHeight));
    int u2=FixedMul(V2->u, IntToFixed(curTexWidth)), v2=FixedMul(V2->v, IntToFixed(curTexHeight));

    int64_t dx12=(int64_t)(V1->y-V2->y)<<16, dy12=(int64_t)(V2->x-V1->x)<<16;
    int64_t dx20=(int64_t)(V2->y-V0->y)<<16, dy20=(int64_t)(V0->x-V2->x)<<16;
    int64_t dx01=(int64_t)(V0->y-V1->y)<<16, dy01=(int64_t)(V1->x-V0->x)<<16;

    int64_t bc0_s = edgeFunctionFixed(V1->x, V1->y, V2->x, V2->y, IntToFixed(minX)+FP_HALF, IntToFixed(minY)+FP_HALF);
    int64_t bc1_s = edgeFunctionFixed(V2->x, V2->y, V0->x, V0->y, IntToFixed(minX)+FP_HALF, IntToFixed(minY)+FP_HALF);
    int64_t bc2_s = edgeFunctionFixed(V0->x, V0->y, V1->x, V1->y, IntToFixed(minX)+FP_HALF, IntToFixed(minY)+FP_HALF);

    int R=BitmapCol_R(color), G=BitmapCol_G(color), B=BitmapCol_B(color), A=BitmapCol_A(color);
    cc_bool texturing = gfx_format == VERTEX_FORMAT_TEXTURED;

    for (int y = minY; y <= maxY; y++, bc0_s += dy12, bc1_s += dy20, bc2_s += dy01) {
        int64_t bc0=bc0_s, bc1=bc1_s, bc2=bc2_s;
        for (int x = minX; x <= maxX; x++, bc0 += dx12, bc1 += dx20, bc2 += dx01) {
            if ((bc0 | bc1 | bc2) < 0) continue;
            diag_3d_pixels_inside++;
            int ic0 = (int)(((int64_t)(bc0 >> 18) * inv_area) >> 16);
            int ic1 = (int)(((int64_t)(bc1 >> 18) * inv_area) >> 16);
            int ic2 = (int)(((int64_t)(bc2 >> 18) * inv_area) >> 16);

            int w_i = FixedMul(ic0, w0) + FixedMul(ic1, w1) + FixedMul(ic2, w2); if (w_i == 0) continue;
            int w = FixedReciprocal(w_i), z = FixedMul(ic0, z0) + FixedMul(ic1, z1) + FixedMul(ic2, z2);
            if (depthTest && (z < 0 || z > depthBuffer[y * db_stride + x])) { diag_3d_depth_fail++; continue; }
            if (depthWrite) depthBuffer[y * db_stride + x] = z;
            if (!colWrite) continue;

            int Rl=R, Gl=G, Bl=B, Al=A;
            if (texturing) {
                int u = FixedMul(FixedMul(ic0, u0) + FixedMul(ic1, u1) + FixedMul(ic2, u2), w);
                int v = FixedMul(FixedMul(ic0, v0) + FixedMul(ic1, v1) + FixedMul(ic2, v2), w);
                BitmapCol t = curTexPixels[(FixedToInt(v) & texHeightMask) * curTexWidth + (FixedToInt(u) & texWidthMask)];
                Al=(A*BitmapCol_A(t))>>8; if (gfx_alphaTest && Al<0x80) { diag_3d_alpha_fail++; continue; }
                Rl=(R*BitmapCol_R(t))>>8; Gl=(G*BitmapCol_G(t))>>8; Bl=(B*BitmapCol_B(t))>>8;
            }
            if (gfx_alphaBlend && Al != 255) {
                BitmapCol d = colorBuffer[y * cb_stride + x];
                Rl=(Rl*Al+BitmapCol_R(d)*(255-Al))>>8; Gl=(Gl*Al+BitmapCol_G(d)*(255-Al))>>8; Bl=(Bl*Al+BitmapCol_B(d)*(255-Al))>>8;
            }
            colorBuffer[y * cb_stride + x] = BitmapCol_Make(Rl, Gl, Bl, 0xFF);
            diag_3d_pixels_written++;
        }
    }
}

static void DrawTriangle2D(VertexFixed* V0, VertexFixed* V1, VertexFixed* V2) {
    int minX = FixedToInt(min(V0->x, min(V1->x, V2->x))), minY = FixedToInt(min(V0->y, min(V1->y, V2->y)));
    int maxX = FixedToInt(max(V0->x, max(V1->x, V2->x))), maxY = FixedToInt(max(V0->y, max(V1->y, V2->y)));
    minX = max(minX, 0); maxX = min(maxX, fb_maxX); minY = max(minY, 0); maxY = min(maxY, fb_maxY);
    int64_t area = edgeFunctionFixed(V0->x, V0->y, V1->x, V1->y, V2->x, V2->y);
    if (area == 0) return;
    if (area < 0) {
        VertexFixed* t = V1; V1 = V2; V2 = t;
        area = -area;
    }
    int32_t area_div = (int32_t)(area >> 18);
    if (area_div == 0) return;

    int32_t inv_area = (int32_t)((1LL << 32) / area_div);

    BitmapCol color = V0->c;
    int u0=FixedMul(V0->u, IntToFixed(curTexWidth)), v0=FixedMul(V0->v, IntToFixed(curTexHeight));
    int u1=FixedMul(V1->u, IntToFixed(curTexWidth)), v1=FixedMul(V1->v, IntToFixed(curTexHeight));
    int u2=FixedMul(V2->u, IntToFixed(curTexWidth)), v2=FixedMul(V2->v, IntToFixed(curTexHeight));

    int64_t dx12=(int64_t)(V1->y-V2->y)<<16, dy12=(int64_t)(V2->x-V1->x)<<16;
    int64_t dx20=(int64_t)(V2->y-V0->y)<<16, dy20=(int64_t)(V0->x-V2->x)<<16;
    int64_t dx01=(int64_t)(V0->y-V1->y)<<16, dy01=(int64_t)(V1->x-V0->x)<<16;

    int64_t bc0_s = edgeFunctionFixed(V1->x, V1->y, V2->x, V2->y, IntToFixed(minX)+FP_HALF, IntToFixed(minY)+FP_HALF);
    int64_t bc1_s = edgeFunctionFixed(V2->x, V2->y, V0->x, V0->y, IntToFixed(minX)+FP_HALF, IntToFixed(minY)+FP_HALF);
    int64_t bc2_s = edgeFunctionFixed(V0->x, V0->y, V1->x, V1->y, IntToFixed(minX)+FP_HALF, IntToFixed(minY)+FP_HALF);

    for (int y = minY; y <= maxY; y++, bc0_s += dy12, bc1_s += dy20, bc2_s += dy01) {
        int64_t bc0=bc0_s, bc1=bc1_s, bc2=bc2_s;
        for (int x = minX; x <= maxX; x++, bc0 += dx12, bc1 += dx20, bc2 += dx01) {
            if ((bc0 | bc1 | bc2) < 0) continue;
            
            int ic0 = (int)(((int64_t)(bc0 >> 18) * inv_area) >> 16);
            int ic1 = (int)(((int64_t)(bc1 >> 18) * inv_area) >> 16);
            int ic2 = (int)(((int64_t)(bc2 >> 18) * inv_area) >> 16);

            int R=BitmapCol_R(color), G=BitmapCol_G(color), B=BitmapCol_B(color), A=BitmapCol_A(color);
            if (gfx_format == VERTEX_FORMAT_TEXTURED) {
                int u = FixedMul(ic0, u0) + FixedMul(ic1, u1) + FixedMul(ic2, u2);
                int v = FixedMul(ic0, v0) + FixedMul(ic1, v1) + FixedMul(ic2, v2);
                BitmapCol t = curTexPixels[(FixedToInt(v) & texHeightMask) * curTexWidth + (FixedToInt(u) & texWidthMask)];
                A=(A*BitmapCol_A(t))>>8; if (A<0x80 && gfx_alphaTest) continue;
                R=(R*BitmapCol_R(t))>>8; G=(G*BitmapCol_G(t))>>8; B=(B*BitmapCol_B(t))>>8;
            }
            if (gfx_alphaBlend && A != 255) {
                BitmapCol d = colorBuffer[y * cb_stride + x];
                R=(R*A+BitmapCol_R(d)*(255-A))>>8; G=(G*A+BitmapCol_G(d)*(255-A))>>8; B=(B*A+BitmapCol_B(d)*(255-A))>>8;
            }
            colorBuffer[y * cb_stride + x] = BitmapCol_Make(R, G, B, 0xFF);
        }
    }
}

static void ProcessClippedTriangleAndDraw(const VertexFixed* inVerts, int polyCount) {
    VertexFixed projected[16];
    int i, count = 0;

    for (i = 0; i < polyCount; i++) {
        projected[count] = inVerts[i];
        if (ViewportVertex3D(&projected[count])) count++;
    }

    for (i = 1; i + 1 < count; i++) {
        DrawTriangle3D(&projected[0], &projected[i], &projected[i + 1]);
    }
}

enum { PLANE_LEFT=0, PLANE_RIGHT, PLANE_BOTTOM, PLANE_TOP, PLANE_NEAR, PLANE_FAR };
static int PlaneDistFixed(const VertexFixed* v, int plane) {
    if (plane == PLANE_LEFT)   return v->x + v->w;
    if (plane == PLANE_RIGHT)  return v->w - v->x;
    if (plane == PLANE_BOTTOM) return v->y + v->w;
    if (plane == PLANE_TOP)    return v->w - v->y;
    if (plane == PLANE_NEAR)   return v->z;
    if (plane == PLANE_FAR)    return v->w - v->z;
    return 0;
}

static void LerpClipFixed(VertexFixed* out, const VertexFixed* a, const VertexFixed* b, int t) {
    int invt = FP_ONE - t;
    out->x = (int)(((int64_t)invt * a->x + (int64_t)t * b->x) >> FP_SHIFT);
    out->y = (int)(((int64_t)invt * a->y + (int64_t)t * b->y) >> FP_SHIFT);
    out->z = (int)(((int64_t)invt * a->z + (int64_t)t * b->z) >> FP_SHIFT);
    out->w = (int)(((int64_t)invt * a->w + (int64_t)t * b->w) >> FP_SHIFT);
    out->u = (int)(((int64_t)invt * a->u + (int64_t)t * b->u) >> FP_SHIFT);
    out->v = (int)(((int64_t)invt * a->v + (int64_t)t * b->v) >> FP_SHIFT);
    out->c = (t < FP_HALF) ? a->c : b->c;
}

static int ClipPolygonPlaneFixed(const VertexFixed* in, int inCount, VertexFixed* out, int plane) {
    int outCount = 0;
    for (int i = 0; i < inCount; i++) {
        const VertexFixed *cur = &in[i], *next = &in[(i + 1) % inCount];
        int dCur = PlaneDistFixed(cur, plane), dNext = PlaneDistFixed(next, plane);
        if (dCur >= 0 && dNext >= 0) {
            out[outCount++] = *next;
        } else if (dCur >= 0) {
            int t = (int)(((int64_t)dCur << FP_SHIFT) / (dCur - dNext));
            LerpClipFixed(&out[outCount++], cur, next, t);
        } else if (dNext >= 0) {
            int t = (int)(((int64_t)dCur << FP_SHIFT) / (dCur - dNext));
            LerpClipFixed(&out[outCount++], cur, next, t);
            out[outCount++] = *next;
        }
    }
    return outCount;
}

static int quadsDrawnThisFrame;
void DrawQuadsFixed(int startVertex, int verticesCount, DrawHints hints) {
    quadsDrawnThisFrame += verticesCount / 4;
    VertexFixed v[4];
    for (int i = 0; i < verticesCount / 4; i++) {
        int j = startVertex + i * 4;
        if (gfx_rendering2D) {
            TransformVertex2D(j+0, &v[0]); TransformVertex2D(j+1, &v[1]); TransformVertex2D(j+2, &v[2]);
            if (hints & (DRAW_HINT_SPRITE|DRAW_HINT_RECT)) DrawSprite2D(&v[0], &v[1], &v[2]);
            else { TransformVertex2D(j+3, &v[3]); DrawTriangle2D(&v[0], &v[2], &v[1]); DrawTriangle2D(&v[2], &v[0], &v[3]); }
        } else {
            if (tex_offseting) ShiftTextureCoords(verticesCount);
            TransformVertex3D(j+0, &v[0]); TransformVertex3D(j+1, &v[1]); TransformVertex3D(j+2, &v[2]);
            TransformVertex3D(j+3, &v[3]);
            VertexFixed b1[16], b2[16], *src = b1, *dst = b2; int count = 4; for(int k=0;k<4;k++) src[k]=v[k];
            for (int p = 0; p < 6; p++) { count = ClipPolygonPlaneFixed(src, count, dst, p); VertexFixed* t = src; src = dst; dst = t; if (!count) break; }
            if (count > 0) ProcessClippedTriangleAndDraw(src, count);
            if (tex_offseting) UnshiftTextureCoords(verticesCount);
        }
    }
}

void Gfx_SetVertexFormat(VertexFormat fmt) { gfx_format = fmt; gfx_stride = strideSizes[fmt]; }
void Gfx_DrawVb_Lines(int verticesCount) { } 
void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) { DrawQuadsFixed(startVertex, verticesCount, hints); }
void Gfx_DrawVb_IndexedTris(int verticesCount) { DrawQuadsFixed(0, verticesCount, DRAW_HINT_NONE); }
void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) { DrawQuadsFixed(startVertex, verticesCount, DRAW_HINT_NONE); }
static BitmapCol* CB_GetRow(struct Bitmap* bmp, int y, void* ctx) { return colorBuffer + cb_stride * y; }
cc_result Gfx_TakeScreenshot(struct Stream* output) { struct Bitmap b; Bitmap_Init(b, fb_width, fb_height, NULL); return Png_Encode(&b, output, CB_GetRow, false, NULL); }
cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }
void Gfx_BeginFrame(void) { quadsDrawnThisFrame = 0; }
static int totalFrames;
void Gfx_EndFrame(void) { 
    totalFrames++;
    Window_DrawFramebuffer((Rect2D){0,0,fb_width,fb_height}, &fb_bmp); 
    colorBuffer = fb_bmp.scan0;
}
void Gfx_SetVSync(cc_bool vsync) { gfx_vsync = vsync; }
void Gfx_OnWindowResize(void) {
    if (depthBuffer) DestroyBuffers();
    fb_width = Game.Width; fb_height = Game.Height; Window_AllocFramebuffer(&fb_bmp, fb_width, fb_height);
    colorBuffer = fb_bmp.scan0; cb_stride = fb_bmp.width; depthBuffer = Mem_Alloc(fb_width * fb_height, 4, "depth"); db_stride = fb_width;
    Gfx_SetViewport(0, 0, fb_width, fb_height); Gfx_SetScissor(0, 0, fb_width, fb_height);
}
void Gfx_SetViewport(int x, int y, int w, int h) { vp_hwidth_fp = FloatToFixed(w/2.0f); vp_hheight_fp = FloatToFixed(h/2.0f); vp_x_fp = IntToFixed(x); vp_y_fp = IntToFixed(y); }
void Gfx_SetScissor(int x, int y, int w, int h) { fb_maxX = x + w - 1; fb_maxY = y + h - 1; }
void Gfx_GetApiInfo(cc_string* info) { int ps = sizeof(void*)*8; String_Format1(info, "-- Software Fixed-Point (%i bit) --\n", &ps); PrintMaxTextureInfo(info); }
cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
