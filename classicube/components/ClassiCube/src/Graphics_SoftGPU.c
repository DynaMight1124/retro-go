#include "Core.h"
#if CC_GFX_BACKEND == CC_GFX_BACKEND_SOFTGPU
#ifdef CC_BUILD_RETROGO
#include <sdkconfig.h>
#endif
#define CC_DYNAMIC_VBS_ARE_STATIC
#define OVERRIDE_BEGEND2D_FUNCTIONS
#include "_GraphicsBase.h"

#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define RENDERING_3D_WIDTH 320
#define RENDERING_3D_HEIGHT 240
#else
#define RENDERING_3D_WIDTH 160
#define RENDERING_3D_HEIGHT 120
#endif
#include "Errors.h"
#include "Window.h"
#include <rg_system.h>
#include <stdint.h>

static cc_bool faceCulling;
static int fb_width, fb_height; 
static struct Bitmap fb_bmp;
static float vp_hwidth, vp_hheight;
static int fb_maxX, fb_maxY;

static BitmapCol* colorBuffer;
static BitmapCol clearColor;
static cc_bool colWrite = true;
static int cb_stride;

typedef cc_uint16 DepthValue;
#define DEPTH_MAX_VALUE 65535
#define DEPTH_SCALE     65535.0f
/* ceil(0.001 * 65535), matching the existing floating-point depth tolerance. */
#define DEPTH_TOLERANCE 66

static DepthValue* depthBuffer;
static cc_bool depthTest  = true;
static cc_bool depthWrite = true;
static int db_stride;

static void* gfx_vertices;

static BitmapCol* realColorBuffer;
static DepthValue* realDepthBuffer;
static BitmapCol* lowColorBuffer;
static DepthValue* lowDepthBuffer;
static GfxResourceID white_square;

typedef uint32_t BitmapPair __attribute__((__may_alias__));

static void Gfx_RestoreState(void) {
	InitDefaultResources();

	// 1x1 dummy white texture
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
	Mem_Free(realDepthBuffer);
	realDepthBuffer = NULL;
#if RENDERING_3D_WIDTH < 320
	Mem_Free(lowColorBuffer);
	lowColorBuffer = NULL;
	Mem_Free(lowDepthBuffer);
	lowDepthBuffer = NULL;
#endif
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
	CCTexture* tex = texId;

	curTexture   = tex;
	curTexPixels = tex->pixels;
	curTexWidth  = tex->width;
	curTexHeight = tex->height;

	texWidthMask   = (1 << Math_ilog2(tex->width))  - 1;
	texHeightMask  = (1 << Math_ilog2(tex->height)) - 1;

	/* Technically the optimisation should only apply if width and height is 1 */
	/* But it's worth sacrificing this, so that rendering the world when */
	/*   no texture pack can use the more optimised rendering path */
	texSinglePixel = curTexWidth == 1;
}
		
void Gfx_DeleteTexture(GfxResourceID* texId) {
	GfxResourceID data = *texId;
	if (data) Mem_Free(data);
	*texId = NULL;
}
		
GfxResourceID Gfx_AllocTexture(struct Bitmap* bmp, int rowWidth, cc_uint8 flags, cc_bool mipmaps) {
	CCTexture* tex = (CCTexture*)Mem_TryAlloc(2 + bmp->width * bmp->height, BITMAPCOLOR_SIZE);
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
*------------------------------------------------------State management---------------------------------------------------*
*#########################################################################################################################*/
void Gfx_SetFog(cc_bool enabled)	{ }
void Gfx_SetFogCol(PackedCol col)   { }
void Gfx_SetFogDensity(float value) { }
void Gfx_SetFogEnd(float value) 	{ }
void Gfx_SetFogMode(FogFunc func)   { }

void Gfx_SetFaceCulling(cc_bool enabled) {
	faceCulling = enabled;
}

static void SetAlphaTest(cc_bool enabled) {
	/* Uses value from Gfx_SetAlphaTest */
}

static void SetAlphaBlend(cc_bool enabled) {
	/* Uses value from Gfx_SetAlphaBlending */
}

void Gfx_SetAlphaArgBlend(cc_bool enabled) { }

static void FillColorRow(BitmapCol* dst, int count, BitmapCol color, BitmapPair pair) {
	BitmapPair* pairs;
	int i, pairCount;

	/* Bitmap pixels are at least 16-bit aligned. Consume one pixel if needed before 32-bit stores. */
	if (((uintptr_t)dst & 3) && count) {
		*dst++ = color;
		count--;
	}
	pairs = (BitmapPair*)dst;
	pairCount = count >> 1;
	for (i = 0; i < pairCount; i++) pairs[i] = pair;
	if (count & 1) dst[count - 1] = color;
}

static void ClearColorBuffer(void) {
	int y, size = fb_width * fb_height;
	BitmapPair pair = (uint32_t)clearColor | ((uint32_t)clearColor << 16);

	if (cb_stride == fb_width) {
		FillColorRow(colorBuffer, size, clearColor, pair);
	} else {
		/* Slower partial buffer clear */
		for (y = 0; y < fb_height; y++) {
			BitmapCol* row = colorBuffer + y * cb_stride;
			FillColorRow(row, fb_width, clearColor, pair);
		}
	}
}

static void ClearDepthBuffer(void) {
	int size = fb_width * fb_height;

	Mem_Set(depthBuffer, 0xFF, size * sizeof(DepthValue));
}

void Gfx_ClearBuffers(GfxBuffers buffers) {
	if (buffers & GFX_BUFFER_COLOR) ClearColorBuffer();
	if (buffers & GFX_BUFFER_DEPTH) ClearDepthBuffer();
}

void Gfx_ClearColor(PackedCol color) {
	int R = PackedCol_R(color);
	int G = PackedCol_G(color);
	int B = PackedCol_B(color);
	int A = PackedCol_A(color);
	(void)A;

	clearColor = BitmapCol_Make(R, G, B, A);
}

void Gfx_SetDepthTest(cc_bool enabled) {
	depthTest = enabled;
}

void Gfx_SetDepthWrite(cc_bool enabled) {
	depthWrite = enabled;
}

static void SetColorWrite(cc_bool r, cc_bool g, cc_bool b, cc_bool a) {
	// TODO
}

void Gfx_DepthOnlyRendering(cc_bool depthOnly) {
	colWrite = !depthOnly;
}


/*########################################################################################################################*
*-------------------------------------------------------Index buffers-----------------------------------------------------*
*#########################################################################################################################*/
GfxResourceID Gfx_CreateIb2(int count, Gfx_FillIBFunc fillFunc, void* obj) {
	return (void*)1;
}

void Gfx_BindIb(GfxResourceID ib) { }
void Gfx_DeleteIb(GfxResourceID* ib) { }


/*########################################################################################################################*
*-------------------------------------------------------Vertex buffers----------------------------------------------------*
*#########################################################################################################################*/
static GfxResourceID Gfx_AllocStaticVb(VertexFormat fmt, int count) {
	return Mem_TryAlloc(count, strideSizes[fmt]);
}

void Gfx_BindVb(GfxResourceID vb) { gfx_vertices = vb; }

void Gfx_DeleteVb(GfxResourceID* vb) {
	GfxResourceID data = *vb;
	if (data) Mem_Free(data);
	*vb = 0;
}

void* Gfx_LockVb(GfxResourceID vb, VertexFormat fmt, int count) { return vb; }

void Gfx_UnlockVb(GfxResourceID vb) { }


/*########################################################################################################################*
*---------------------------------------------------------Matrices--------------------------------------------------------*
*#########################################################################################################################*/
static float texOffsetX, texOffsetY;
static struct Matrix _view, _proj, _mvp;

void Gfx_LoadMatrix(MatrixType type, const struct Matrix* matrix) {
	if (type == MATRIX_VIEW) _view = *matrix;
	if (type == MATRIX_PROJ) _proj = *matrix;

	Matrix_Mul(&_mvp, &_view, &_proj);
}

void Gfx_LoadMVP(const struct Matrix* view, const struct Matrix* proj, struct Matrix* mvp) {
	_view = *view;
	_proj = *proj;

	Matrix_Mul(mvp, view, proj);
	_mvp  = *mvp;
}

void Gfx_EnableTextureOffset(float x, float y) {
	texOffsetX = x;
	texOffsetY = y;
}

void Gfx_DisableTextureOffset(void) {
	texOffsetX = 0;
	texOffsetY = 0;
}

void Gfx_CalcOrthoMatrix(struct Matrix* matrix, float width, float height, float zNear, float zFar) {
	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh */
	/*   The simplified calculation below uses: L = 0, R = width, T = 0, B = height */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
	*matrix = Matrix_Identity;

	matrix->row1.x =  2.0f / width;
	matrix->row2.y = -2.0f / height;
	matrix->row3.z =  1.0f / (zNear - zFar);

	matrix->row4.x = -1.0f;
	matrix->row4.y =  1.0f;
	matrix->row4.z = zNear / (zNear - zFar);
}

static float Cotangent(float x) { return Math_CosF(x) / Math_SinF(x); }
void Gfx_CalcPerspectiveMatrix(struct Matrix* matrix, float fov, float aspect, float zFar) {
	float zNear = 0.1f;

	/* Source https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovrh */
	/* NOTE: This calculation is shared with Direct3D 11 backend */
	float c = Cotangent(0.5f * fov);
	*matrix = Matrix_Identity;

	matrix->row1.x =  c / aspect;
	matrix->row2.y =  c;
	matrix->row3.z = zFar / (zNear - zFar);
	matrix->row3.w = -1.0f;
	matrix->row4.z = (zNear * zFar) / (zNear - zFar);
	matrix->row4.w =  0.0f;
}


/*########################################################################################################################*
*---------------------------------------------------------Rendering-------------------------------------------------------*
*#########################################################################################################################*/
typedef struct Vector3 { float x, y, z; } Vector3;
typedef struct Vector2 { float x, y; } Vector2;
typedef struct Vertex_ {
	float x, y, z, w;
	float u, v;
	PackedCol c;
} Vertex;

static void TransformVertex2D(int index, Vertex* vertex) {
	// TODO: avoid the multiply, just add down in DrawTriangles
	char* ptr = (char*)gfx_vertices + index * gfx_stride;
	Vector3* pos = (Vector3*)ptr;
	vertex->x = pos->x;
	vertex->y = pos->y;
	vertex->z = 0.0f;
	vertex->w = 1.0f;

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		struct VertexColoured* v = (struct VertexColoured*)ptr;
		vertex->u = 0.0f;
		vertex->v = 0.0f;
		vertex->c = v->Col;
	} else {
		struct VertexTextured* v = (struct VertexTextured*)ptr;
		vertex->u = v->U;
		vertex->v = v->V;
		vertex->c = v->Col;
	}
}

static int TransformVertex3D(int index, Vertex* vertex) {
	// TODO: avoid the multiply, just add down in DrawTriangles
	char* ptr = (char*)gfx_vertices + index * gfx_stride;
	Vector3* pos = (Vector3*)ptr;

	vertex->x = pos->x * _mvp.row1.x + pos->y * _mvp.row2.x + pos->z * _mvp.row3.x + _mvp.row4.x;
	vertex->y = pos->x * _mvp.row1.y + pos->y * _mvp.row2.y + pos->z * _mvp.row3.y + _mvp.row4.y;
	vertex->z = pos->x * _mvp.row1.z + pos->y * _mvp.row2.z + pos->z * _mvp.row3.z + _mvp.row4.z;
	vertex->w = pos->x * _mvp.row1.w + pos->y * _mvp.row2.w + pos->z * _mvp.row3.w + _mvp.row4.w;

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		struct VertexColoured* v = (struct VertexColoured*)ptr;
		vertex->u = 0.0f;
		vertex->v = 0.0f;
		vertex->c = v->Col;
	} else {
		struct VertexTextured* v = (struct VertexTextured*)ptr;
		vertex->u = (v->U + texOffsetX);
		vertex->v = (v->V + texOffsetY);
		vertex->c = v->Col;
	}
	return vertex->z >= 0.0f;
}

static void ViewportVertex3D(Vertex* vertex) {
	float invW = 1.0f / vertex->w;

	vertex->x = vp_hwidth  * (1 + vertex->x * invW);
	vertex->y = vp_hheight * (1 - vertex->y * invW);
	vertex->z = vertex->z * invW;
	vertex->w = invW;

	vertex->u *= invW;
	vertex->v *= invW;
}

// Ensure it's inlined, whereas Math_FloorF might not be
static CC_INLINE int FastFloor(float value) {
	int valueI = (int)value;
	return valueI > value ? valueI - 1 : valueI;
}

static void DrawSprite2D(Vertex* V0, Vertex* V1, Vertex* V2) {
	PackedCol vColor = V0->c;
	int minX = (int)V0->x;
	int minY = (int)V0->y;
	int maxX = (int)V1->x;
	int maxY = (int)V2->y;

	// Reject triangles completely outside
	if (maxX < 0 || minX > fb_maxX) return;
	if (maxY < 0 || minY > fb_maxY) return;

	int origX = minX;
	int origY = minY;

	// Perform scissoring
	minX = max(minX, 0); maxX = min(maxX, fb_maxX);
	minY = max(minY, 0); maxY = min(maxY, fb_maxY);

	if (gfx_format != VERTEX_FORMAT_TEXTURED) {
		int R = PackedCol_R(vColor);
		int G = PackedCol_G(vColor);
		int B = PackedCol_B(vColor);
		int A = PackedCol_A(vColor);

		if (gfx_alphaBlend && A == 0) return;

		int x, y;
		if (gfx_alphaBlend && A != 255) {
			for (y = minY; y <= maxY; y++) {
				int cb_row = y * cb_stride;
				for (x = minX; x <= maxX; x++) {
					int cb_index = cb_row + x;
					BitmapCol dst = colorBuffer[cb_index];
					int dstR = BitmapCol_R(dst);
					int dstG = BitmapCol_G(dst);
					int dstB = BitmapCol_B(dst);

					int finR = (R * A + dstR * (255 - A)) >> 8;
					int finG = (G * A + dstG * (255 - A)) >> 8;
					int finB = (B * A + dstB * (255 - A)) >> 8;
					colorBuffer[cb_index] = BitmapCol_Make(finR, finG, finB, 0xFF);
				}
			}
		} else {
			BitmapCol color = BitmapCol_Make(R, G, B, 0xFF);
			BitmapPair pair = (uint32_t)color | ((uint32_t)color << 16);
			int count = maxX - minX + 1;
			for (y = minY; y <= maxY; y++) {
				int cb_row = y * cb_stride;
				FillColorRow(colorBuffer + cb_row + minX, count, color, pair);
			}
		}
		return;
	}

	int begTX = (int)(V0->u * curTexWidth);
	int begTY = (int)(V0->v * curTexHeight);
	int delTX = (int)(V1->u * curTexWidth)  - begTX;
	int delTY = (int)(V2->v * curTexHeight) - begTY;

	int width = maxX - origX, height = maxY - origY;
	if (width == 0) width = 1;
	if (height == 0) height = 1;

	int fast =  delTX == width && delTY == height && 
				(begTX + delTX < curTexWidth ) && 
				(begTY + delTY < curTexHeight);

	int x, y;
#ifdef BITMAP_565
	/* Retro-Go textures use zero as transparent and all other RGB565 values as opaque.
	   Avoid unpacking and rebuilding the common un-tinted GUI/font pixels. */
	if (vColor == PACKEDCOL_WHITE) {
		for (y = minY; y <= maxY; y++) {
			int texY = fast ? (begTY + (y - origY)) : (((begTY + delTY * (y - origY) / height)) & texHeightMask);
			BitmapCol* dst = colorBuffer + y * cb_stride;
			for (x = minX; x <= maxX; x++) {
				int texX = fast ? (begTX + (x - origX)) : (((begTX + delTX * (x - origX) / width)) & texWidthMask);
				BitmapCol color = curTexPixels[texY * curTexWidth + texX];
				if (!gfx_alphaBlend || color) dst[x] = color;
			}
		}
		return;
	}
	if (gfx_alphaBlend && PackedCol_A(vColor) == 255) {
		int tintR = PackedCol_R(vColor);
		int tintG = PackedCol_G(vColor);
		int tintB = PackedCol_B(vColor);

		for (y = minY; y <= maxY; y++) {
			int texY = fast ? (begTY + (y - origY)) : (((begTY + delTY * (y - origY) / height)) & texHeightMask);
			BitmapCol* dst = colorBuffer + y * cb_stride;
			for (x = minX; x <= maxX; x++) {
				int texX = fast ? (begTX + (x - origX)) : (((begTX + delTX * (x - origX) / width)) & texWidthMask);
				BitmapCol color = curTexPixels[texY * curTexWidth + texX];
				int R, G, B;
				if (!color) continue;

				R = (BitmapCol_R(color) * tintR) >> 8;
				G = (BitmapCol_G(color) * tintG) >> 8;
				B = (BitmapCol_B(color) * tintB) >> 8;
				dst[x] = BitmapCol_Make(R, G, B, 0xFF);
			}
		}
		return;
	}
#endif
	for (y = minY; y <= maxY; y++) 
	{
		int texY = fast ? (begTY + (y - origY)) : (((begTY + delTY * (y - origY) / height)) & texHeightMask);
		for (x = minX; x <= maxX; x++) 
		{
			int texX = fast ? (begTX + (x - origX)) : (((begTX + delTX * (x - origX) / width)) & texWidthMask);
			int texIndex = texY * curTexWidth + texX;

			BitmapCol color = curTexPixels[texIndex];
			int R = BitmapCol_R(color);
			int G = BitmapCol_G(color);
			int B = BitmapCol_B(color);
			int A = BitmapCol_A(color);

			if (vColor != PACKEDCOL_WHITE) {
				R = (R * PackedCol_R(vColor)) >> 8;
				G = (G * PackedCol_G(vColor)) >> 8;
				B = (B * PackedCol_B(vColor)) >> 8;
				A = (A * PackedCol_A(vColor)) >> 8;
			}

			if (gfx_alphaBlend && A == 0) continue;
			int cb_index = y * cb_stride + x;

			if (gfx_alphaBlend && A != 255) {
				BitmapCol dst = colorBuffer[cb_index];
				int dstR = BitmapCol_R(dst);
				int dstG = BitmapCol_G(dst);
				int dstB = BitmapCol_B(dst);

				R = (R * A + dstR * (255 - A)) >> 8;
				G = (G * A + dstG * (255 - A)) >> 8;
				B = (B * A + dstB * (255 - A)) >> 8;
			}

			colorBuffer[cb_index] = BitmapCol_Make(R, G, B, 0xFF);
		}
	}
}

#define edgeFunction(ax,ay, bx,by, cx,cy) (((bx) - (ax)) * ((cy) - (ay)) - ((by) - (ay)) * ((cx) - (ax)))

static void DrawTriangle2D(Vertex* V0, Vertex* V1, Vertex* V2) {
	int x0 = (int)V0->x, y0 = (int)V0->y;
	int x1 = (int)V1->x, y1 = (int)V1->y;
	int x2 = (int)V2->x, y2 = (int)V2->y;
	int minX = min(x0, min(x1, x2));
	int minY = min(y0, min(y1, y2));
	int maxX = max(x0, max(x1, x2));
	int maxY = max(y0, max(y1, y2));

	// Reject triangles completely outside
	if (maxX < 0 || minX > fb_maxX) return;
	if (maxY < 0 || minY > fb_maxY) return;

	// Perform scissoring
	minX = max(minX, 0); maxX = min(maxX, fb_maxX);
	minY = max(minY, 0); maxY = min(maxY, fb_maxY);

	int area = edgeFunction(x0,y0, x1,y1, x2,y2);
	if (area == 0) return;
	if (area < 0) {
		Vertex* t = V1; V1 = V2; V2 = t;
		x0 = (int)V0->x; y0 = (int)V0->y;
		x1 = (int)V1->x; y1 = (int)V1->y;
		x2 = (int)V2->x; y2 = (int)V2->y;
		area = -area;
	}

	float u0 = V0->u * curTexWidth,  u1 = V1->u * curTexWidth,  u2 = V2->u * curTexWidth;
	float v0 = V0->v * curTexHeight, v1 = V1->v * curTexHeight, v2 = V2->v * curTexHeight;
	PackedCol color = V0->c;

	float factor = 1.0f / area;
	int x, y;
	
	// https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
	// Essentially these are the deltas of edge functions between X/Y and X/Y + 1 (i.e. one X/Y step)
	int dx01  = y0 - y1, dy01 = x1 - x0;
	int dx12  = y1 - y2, dy12 = x2 - x1;
	int dx20  = y2 - y0, dy20 = x0 - x2;

	float bc0_start = edgeFunction(x1,y1, x2,y2, minX+0.5f,minY+0.5f);
	float bc1_start = edgeFunction(x2,y2, x0,y0, minX+0.5f,minY+0.5f);
	float bc2_start = edgeFunction(x0,y0, x1,y1, minX+0.5f,minY+0.5f);

	for (y = minY; y <= maxY; y++, bc0_start += dy12, bc1_start += dy20, bc2_start += dy01) 
	{
		float bc0 = bc0_start;
		float bc1 = bc1_start;
		float bc2 = bc2_start;

		for (x = minX; x <= maxX; x++, bc0 += dx12, bc1 += dx20, bc2 += dx01) 
		{
			if (bc0 < 0 || bc1 < 0 || bc2 < 0) continue;

			float ic0 = bc0 * factor;
			float ic1 = bc1 * factor;
			float ic2 = bc2 * factor;
			int cb_index = y * cb_stride + x;

			int R, G, B, A;
			if (gfx_format == VERTEX_FORMAT_TEXTURED) {
				float u = ic0 * u0 + ic1 * u1 + ic2 * u2;
				float v = ic0 * v0 + ic1 * v1 + ic2 * v2;
				int texX = ((int)u) & texWidthMask;
				int texY = ((int)v) & texHeightMask;
				int texIndex = texY * curTexWidth + texX;

				BitmapCol tColor = curTexPixels[texIndex];
				int a1 = PackedCol_A(color), a2 = BitmapCol_A(tColor);
				A = ( a1 * a2 ) >> 8;
				int r1 = PackedCol_R(color), r2 = BitmapCol_R(tColor);
				R = ( r1 * r2 ) >> 8;
				int g1 = PackedCol_G(color), g2 = BitmapCol_G(tColor);
				G = ( g1 * g2 ) >> 8;
				int b1 = PackedCol_B(color), b2 = BitmapCol_B(tColor);
				B = ( b1 * b2 ) >> 8;
			} else {
				R = PackedCol_R(color);
				G = PackedCol_G(color);
				B = PackedCol_B(color);
				A = PackedCol_A(color);
			}

			if (gfx_alphaTest && A < 0x80) continue;
			if (gfx_alphaBlend && A == 0)  continue;

			if (gfx_alphaBlend && A != 255) {
				BitmapCol dst = colorBuffer[cb_index];
				int dstR = BitmapCol_R(dst);
				int dstG = BitmapCol_G(dst);
				int dstB = BitmapCol_B(dst);

				R = (R * A + dstR * (255 - A)) >> 8;
				G = (G * A + dstG * (255 - A)) >> 8;
				B = (B * A + dstB * (255 - A)) >> 8;
			}

			colorBuffer[cb_index] = BitmapCol_Make(R, G, B, 0xFF);
		}
	}
}

#define MultiplyColors(vColor, tColor) \
	a1 = PackedCol_A(vColor); \
	a2 = BitmapCol_A(tColor); \
	A  = ( a1 * a2 ) >> 8;    \
\
	r1 = PackedCol_R(vColor); \
	r2 = BitmapCol_R(tColor); \
	R  = ( r1 * r2 ) >> 8;    \
\
	g1 = PackedCol_G(vColor); \
	g2 = BitmapCol_G(tColor); \
	G  = ( g1 * g2 ) >> 8;    \
\
	b1 = PackedCol_B(vColor); \
	b2 = BitmapCol_B(tColor); \
	B  = ( b1 * b2 ) >> 8;    \

static void DrawTriangle3D(Vertex* V0, Vertex* V1, Vertex* V2) {
	int x0 = (int)V0->x, y0 = (int)V0->y;
	int x1 = (int)V1->x, y1 = (int)V1->y;
	int x2 = (int)V2->x, y2 = (int)V2->y;
	int minX = min(x0, min(x1, x2));
	int minY = min(y0, min(y1, y2));
	int maxX = max(x0, max(x1, x2));
	int maxY = max(y0, max(y1, y2));

	int area = edgeFunction(x0,y0, x1,y1, x2,y2);
	if (area == 0) return;
	if (area < 0) {
		if (faceCulling) return;
		Vertex* t = V1; V1 = V2; V2 = t;
		x1 = (int)V1->x; y1 = (int)V1->y;
		x2 = (int)V2->x; y2 = (int)V2->y;
		area = -area;
	}

	// Reject triangles completely outside
	if (maxX < 0 || minX > fb_maxX) return;
	if (maxY < 0 || minY > fb_maxY) return;

	// Perform scissoring
	minX = max(minX, 0); maxX = min(maxX, fb_maxX);
	minY = max(minY, 0); maxY = min(maxY, fb_maxY);

	// NOTE: W in frag variables below is actually 1/W 
	float factor = 1.0f / area;
	float w0 = V0->w, w1 = V1->w, w2 = V2->w;
	
	// TODO proper clipping
	if (w0 <= 0 || w1 <= 0 || w2 <= 0) {
		return;
	}

	float z0 = V0->z, z1 = V1->z, z2 = V2->z;
	PackedCol color = V0->c;

	float u0 = V0->u * curTexWidth,  u1 = V1->u * curTexWidth,  u2 = V2->u * curTexWidth;
	float v0 = V0->v * curTexHeight, v1 = V1->v * curTexHeight, v2 = V2->v * curTexHeight;
	
	// https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
	// Essentially these are the deltas of edge functions between X/Y and X/Y + 1 (i.e. one X/Y step)
	int dx01  = y0 - y1, dy01 = x1 - x0;
	int dx12  = y1 - y2, dy12 = x2 - x1;
	int dx20  = y2 - y0, dy20 = x0 - x2;

	float bc0_start = edgeFunction(x1,y1, x2,y2, minX+0.5f,minY+0.5f);
	float bc1_start = edgeFunction(x2,y2, x0,y0, minX+0.5f,minY+0.5f);
	float bc2_start = edgeFunction(x0,y0, x1,y1, minX+0.5f,minY+0.5f);
	float z_step = (dx12 * z0 + dx20 * z1 + dx01 * z2) * factor;

	int R = 0, G = 0, B = 0, A = 0, x, y;
	int a1, r1, g1, b1;
	int a2, r2, g2, b2;
	cc_bool texturing = gfx_format == VERTEX_FORMAT_TEXTURED;

	if (!texturing) {
		R = PackedCol_R(color);
		G = PackedCol_G(color);
		B = PackedCol_B(color);
		A = PackedCol_A(color);
	} else if (texSinglePixel) {
		/* Don't need to calculate complicated texturing in this case */
		float rawY0 = v0 / w0;
		float rawY1 = v1 / w1;

		float rawY = min(rawY0, rawY1);
		int texY   = (int)(rawY + 0.01f) & texHeightMask;
		MultiplyColors(color, curTexPixels[texY * curTexWidth]);
		texturing = false;
	}

	for (y = minY; y <= maxY; y++, bc0_start += dy12, bc1_start += dy20, bc2_start += dy01)
	{
		float bc0 = bc0_start;
		float bc1 = bc1_start;
		float bc2 = bc2_start;
		float z = (bc0_start * z0 + bc1_start * z1 + bc2_start * z2) * factor;
		int db_row = y * db_stride;
		int cb_row = y * cb_stride;

		for (x = minX; x <= maxX; x++, bc0 += dx12, bc1 += dx20, bc2 += dx01, z += z_step)
		{
			if (bc0 < 0 || bc1 < 0 || bc2 < 0) continue;

			int zValue;
			int db_index = db_row + x;

			if (depthTest && z < 0) continue;
			if (z <= 0.0f) {
				zValue = 0;
			} else if (z >= 1.0f) {
				zValue = DEPTH_MAX_VALUE;
			} else {
				zValue = (int)(z * DEPTH_SCALE);
			}
			if (depthTest && zValue > depthBuffer[db_index] + DEPTH_TOLERANCE) continue;
			if (!colWrite) {
				if (depthWrite) depthBuffer[db_index] = (DepthValue)zValue;
				continue;
			}

			if (texturing) {
				/* Only surviving textured pixels need perspective correction. */
				float ic0 = bc0 * factor;
				float ic1 = bc1 * factor;
				float ic2 = bc2 * factor;
				float w = 1.0f / (ic0 * w0 + ic1 * w1 + ic2 * w2);
				float u = (ic0 * u0 + ic1 * u1 + ic2 * u2) * w;
				float v = (ic0 * v0 + ic1 * v1 + ic2 * v2) * w;
				int texX = ((int)u) & texWidthMask;
				int texY = ((int)v) & texHeightMask;

				int texIndex = texY * curTexWidth + texX;
				BitmapCol tColor = curTexPixels[texIndex];

#ifdef BITMAP_565
				if (!gfx_alphaBlend) {
					int vertexA = PackedCol_A(color);
					BitmapCol shaded;
					if (gfx_alphaTest && (!tColor || ((vertexA * 255) >> 8) < 0x80)) continue;

					/* This is algebraically identical to expanding RGB565 to 8-bit,
					   multiplying by the vertex tint, then packing back to RGB565. */
					shaded  = (BitmapCol)((((tColor >> 11) & 0x1F) * PackedCol_R(color) >> 8) << 11);
					shaded |= (BitmapCol)((((tColor >>  5) & 0x3F) * PackedCol_G(color) >> 8) <<  5);
					shaded |= (BitmapCol)( ((tColor        & 0x1F) * PackedCol_B(color) >> 8));

					if (depthWrite) depthBuffer[db_index] = (DepthValue)zValue;
					colorBuffer[cb_row + x] = shaded;
					continue;
				}
#endif
				MultiplyColors(color, tColor);
			}

			if (gfx_alphaTest && A < 0x80) continue;
			if (depthWrite) depthBuffer[db_index] = (DepthValue)zValue;
			int cb_index = cb_row + x;
			
			if (!gfx_alphaBlend) {
				colorBuffer[cb_index] = BitmapCol_Make(R, G, B, 0xFF);
				continue;
			}

			BitmapCol dst = colorBuffer[cb_index];
			int dstR = BitmapCol_R(dst);
			int dstG = BitmapCol_G(dst);
			int dstB = BitmapCol_B(dst);

			int finR = (R * A + dstR * (255 - A)) >> 8;
			int finG = (G * A + dstG * (255 - A)) >> 8;
			int finB = (B * A + dstB * (255 - A)) >> 8;
			colorBuffer[cb_index] = BitmapCol_Make(finR, finG, finB, 0xFF);
		}
	}
}

#define V0_VIS (1 << 0)
#define V1_VIS (1 << 1)
#define V2_VIS (1 << 2)
#define V3_VIS (1 << 3)

// https://github.com/behindthepixels/EDXRaster/blob/master/EDXRaster/Core/Clipper.h
static void ClipLine(Vertex* v1, Vertex* v2, Vertex* V) {
	float t  = Math_AbsF(v1->z / (v2->z - v1->z));
	float invt = 1.0f - t;
	
	V->x = invt * v1->x + t * v2->x;
	V->y = invt * v1->y + t * v2->y;
	//V->z = invt * v1->z + t * v2->z;
	V->z = 0.0f; // clipped against near plane anyways (I.e Z/W = 0 --> Z = 0)
	V->w = invt * v1->w + t * v2->w;
	
	V->u = invt * v1->u + t * v2->u;
	V->v = invt * v1->v + t * v2->v;
	V->c = v1->c;
}

// https://casual-effects.com/research/McGuire2011Clipping/clip.glsl
static void DrawClipped(int mask, Vertex* v0, Vertex* v1, Vertex* v2, Vertex* v3) {
	Vertex tmp[2];
	Vertex* a = &tmp[0];
	Vertex* b = &tmp[1];

    switch (mask) {
	case V0_VIS:
	{
		//		   v0
		//		  / |
		//       /   |
		// .....A....B...
		//    /      |
		//  v3--v2---v1
		ClipLine(v3, v0, a);
		ClipLine(v0, v1, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v0, a, b);
	}
    break;
	case V1_VIS:
	{
		//		   v1
		//		  / |
		//       /   |
		// ....A.....B...
		//    /      |
		//  v0--v3---v2
		ClipLine(v0, v1, a);
		ClipLine(v1, v2, b);

		ViewportVertex3D(v1);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(a, b, v1);
	} break;
	case V2_VIS:
	{
		//		   v2
		//		  / |
		//       /   |
		// ....A.....B...
		//    /      |
		//  v1--v0---v3
		ClipLine(v1, v2, a);
		ClipLine(v2, v3, b);

		ViewportVertex3D(v2);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(a, b, v2);
	} break;
	case V3_VIS:
	{
		//		   v3
		//		  / |
		//       /   |
		// ....A.....B...
		//    /      |
		//  v2--v1---v0
		ClipLine(v2, v3, a);
		ClipLine(v3, v0, b);

		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);
		
		DrawTriangle3D(b, v3, a);
	}
	break;
	case V0_VIS | V1_VIS:
	{
		//    v0-----------v1
		//     \		   |
		//   ...B.........A...
		//		 \		  |
		//		  v3-----v2
		ClipLine(v1, v2, a);
		ClipLine(v3, v0, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(v1);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v1, v0,  a);
		DrawTriangle3D(a,  v0,  b);
	} break;
	// case V0_VIS | V2_VIS: degenerate case that should never happen
	case V0_VIS | V3_VIS:
	{
		//    v3-----------v0
		//     \		   |
		//   ...B.........A...
		//		 \		  |
		//		  v2-----v1
		ClipLine(v0, v1, a);
		ClipLine(v2, v3, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(a, v0,  b);
		DrawTriangle3D(b, v0, v3);
	} break;
	case V1_VIS | V2_VIS:
	{
		//    v1-----------v2
		//     \		   |
		//   ...B.........A...
		//		 \		  |
		//		  v0-----v3
		ClipLine(v2, v3, a);
		ClipLine(v0, v1, b);

		ViewportVertex3D(v1);
		ViewportVertex3D(v2);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v1,  b, v2);
		DrawTriangle3D(v2,  b,  a);
	} break;
	// case V1_VIS | V3_VIS: degenerate case that should never happen
	case V2_VIS | V3_VIS:
	{
		//    v2-----------v3
		//     \		   |
		//   ...B.........A...
		//		 \		  |
		//		  v1-----v0
		ClipLine(v3, v0, a);
		ClipLine(v1, v2, b);

		ViewportVertex3D(v2);
		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D( b,  a, v2);
		DrawTriangle3D(v2,  a, v3);
	} break;
	case V0_VIS | V1_VIS | V2_VIS:
	{
		//		  --v1--
		//    v0--      --v2
		//      \		 /
		//   ....B.....A...
		//		  \   /
		//		    v3
		// v1,v2,v0  v2,v0,A  v0,A,B
		ClipLine(v2, v3, a);
		ClipLine(v3, v0, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(v1);
		ViewportVertex3D(v2);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v1, v0, v2);
		DrawTriangle3D(v2, v0,  a);
		DrawTriangle3D(v0,  b,  a);
	} break;
	case V0_VIS | V1_VIS | V3_VIS:
	{
		//		  --v0--
		//    v3--      --v1
		//      \		 /
		//   ....B.....A...
		//		  \   /
		//		    v2
		// v0,v1,v3  v1,v3,A  v3,A,B
		ClipLine(v1, v2, a);
		ClipLine(v2, v3, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(v1);
		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v0, v3, v1);
		DrawTriangle3D(v1, v3,  a);
		DrawTriangle3D(v3,  b,  a);
	} break;
	case V0_VIS | V2_VIS | V3_VIS:
	{
		//		  --v3--
		//    v2--      --v0
		//      \		 /
		//   ....B.....A...
		//		  \   /
		//		    v1
		// v3,v0,v2  v0,v2,A  v2,A,B
		ClipLine(v0, v1, a);
		ClipLine(v1, v2, b);

		ViewportVertex3D(v0);
		ViewportVertex3D(v2);
		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v3, v2, v0);
		DrawTriangle3D(v0, v2,  a);
		DrawTriangle3D(v2,  b,  a);
	} break;
	case V1_VIS | V2_VIS | V3_VIS:
	{
		//		  --v2--
		//    v1--      --v3
		//      \		 /
		//   ....B.....A...
		//		  \   /
		//		    v0
		// v2,v3,v1  v3,v1,A  v1,A,B
		ClipLine(v3, v0, a);
		ClipLine(v0, v1, b);

		ViewportVertex3D(v1);
		ViewportVertex3D(v2);
		ViewportVertex3D(v3);
		ViewportVertex3D(a);
		ViewportVertex3D(b);

		DrawTriangle3D(v2, v1, v3);
		DrawTriangle3D(v3, v1,  a);
		DrawTriangle3D(v1,  b,  a);
	} break;
	}
}

void DrawQuads(int startVertex, int verticesCount, DrawHints hints) {
	Vertex vertices[4];
	int i, j = startVertex;

	if (gfx_rendering2D && (hints & (DRAW_HINT_SPRITE|DRAW_HINT_RECT))) {
		// 4 vertices = 1 quad = 2 triangles
		for (i = 0; i < verticesCount / 4; i++, j += 4)
		{
			TransformVertex2D(j + 0, &vertices[0]);
			TransformVertex2D(j + 1, &vertices[1]);
			TransformVertex2D(j + 2, &vertices[2]);

			DrawSprite2D(&vertices[0], &vertices[1], &vertices[2]);
		}
	} else if (gfx_rendering2D) {
		// 4 vertices = 1 quad = 2 triangles
		for (i = 0; i < verticesCount / 4; i++, j += 4)
		{
			TransformVertex2D(j + 0, &vertices[0]);
			TransformVertex2D(j + 1, &vertices[1]);
			TransformVertex2D(j + 2, &vertices[2]);
			TransformVertex2D(j + 3, &vertices[3]);

			DrawTriangle2D(&vertices[0], &vertices[2], &vertices[1]);
			DrawTriangle2D(&vertices[2], &vertices[0], &vertices[3]);
		}
	} else {
		// 4 vertices = 1 quad = 2 triangles
		for (i = 0; i < verticesCount / 4; i++, j += 4)
		{
			int clip = TransformVertex3D(j + 0, &vertices[0]) << 0
					|  TransformVertex3D(j + 1, &vertices[1]) << 1
					|  TransformVertex3D(j + 2, &vertices[2]) << 2
					|  TransformVertex3D(j + 3, &vertices[3]) << 3;

			if (clip == 0) {
				// Quad entirely clipped
			} else if (clip == 0x0F) {
				// Quad entirely visible
				ViewportVertex3D(&vertices[0]);
				ViewportVertex3D(&vertices[1]);
				ViewportVertex3D(&vertices[2]);
				ViewportVertex3D(&vertices[3]);

				DrawTriangle3D(&vertices[0], &vertices[2], &vertices[1]);
				DrawTriangle3D(&vertices[2], &vertices[0], &vertices[3]);
			} else {
				// Quad partially visible
				DrawClipped(clip, &vertices[0], &vertices[1], &vertices[2], &vertices[3]);
			}
		}
	}
}

void Gfx_SetVertexFormat(VertexFormat fmt) {
	gfx_format = fmt;
	gfx_stride = strideSizes[fmt];
}

void Gfx_DrawVb_Lines(int verticesCount) { } /* TODO */

void Gfx_DrawVb_IndexedTris_Range(int verticesCount, int startVertex, DrawHints hints) {
	DrawQuads(startVertex, verticesCount, hints);
}

void Gfx_DrawVb_IndexedTris(int verticesCount) {
	DrawQuads(0, verticesCount, DRAW_HINT_NONE);
}

void Gfx_DrawIndexedTris_T2fC4b(int verticesCount, int startVertex) {
	DrawQuads(startVertex, verticesCount, DRAW_HINT_NONE);
}


/*########################################################################################################################*
*---------------------------------------------------------Other/Misc------------------------------------------------------*
*#########################################################################################################################*/
static BitmapCol* CB_GetRow(struct Bitmap* bmp, int y, void* ctx) {
	return colorBuffer + cb_stride * y;
}

cc_result Gfx_TakeScreenshot(struct Stream* output) {
	struct Bitmap bmp;
	Bitmap_Init(bmp, fb_width, fb_height, NULL);
	return Png_Encode(&bmp, output, CB_GetRow, false, NULL);
}

cc_bool Gfx_WarnIfNecessary(void) { return false; }
cc_bool Gfx_GetUIOptions(struct MenuOptionsScreen* s) { return false; }

void Gfx_BeginFrame(void) {
	realColorBuffer = fb_bmp.scan0;
#if RENDERING_3D_WIDTH >= 320
	colorBuffer = realColorBuffer;
#endif
}

void Gfx_EndFrame(void) {
	Rect2D r = { 0, 0, fb_width, fb_height };
	Window_DrawFramebuffer(r, &fb_bmp);
}

void Gfx_SetVSync(cc_bool vsync) {
	gfx_vsync = vsync;
}

void Gfx_OnWindowResize(void) {
	if (depthBuffer) DestroyBuffers();

	fb_width   = Game.Width;
	fb_height  = Game.Height;

	Window_AllocFramebuffer(&fb_bmp, Game.Width, Game.Height);
	realColorBuffer = fb_bmp.scan0;

#if RENDERING_3D_WIDTH < 320
	lowColorBuffer = (BitmapCol*)rg_alloc(160 * 120 * sizeof(BitmapCol), MEM_FAST);
	lowDepthBuffer = (DepthValue*)rg_alloc(160 * 120 * sizeof(DepthValue), MEM_FAST);

	// Start in 3D mode (low res)
	colorBuffer = lowColorBuffer;
	depthBuffer = lowDepthBuffer;
	fb_width = 160;
	fb_height = 120;
	cb_stride = 160;
	db_stride = 160;
	fb_maxX = 159;
	fb_maxY = 119;

	Gfx_SetViewport(0, 0, 160, 120);
	Gfx_SetScissor (0, 0, 160, 120);
#else
	realDepthBuffer = (DepthValue*)rg_alloc(Game.Width * Game.Height * sizeof(DepthValue), MEM_FAST);
	colorBuffer = realColorBuffer;
	depthBuffer = realDepthBuffer;
	cb_stride = fb_bmp.width;
	db_stride = fb_width;
	fb_maxX = Game.Width - 1;
	fb_maxY = Game.Height - 1;

	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
	Gfx_SetScissor (0, 0, Game.Width, Game.Height);
#endif
}

void Gfx_SetViewport(int x, int y, int w, int h) {
	vp_hwidth  = w / 2.0f;
	vp_hheight = h / 2.0f;
}

void Gfx_SetScissor (int x, int y, int w, int h) {
	/* TODO minX/Y */
	fb_maxX = x + w - 1;
	fb_maxY = y + h - 1;
}

void Gfx_GetApiInfo(cc_string* info) {
	int pointerSize = sizeof(void*) * 8;
	String_Format1(info, "-- Using software (%i bit) --\n", &pointerSize);
	PrintMaxTextureInfo(info);
}

static void Upscale3D(void) {
#if RENDERING_3D_WIDTH < 320
	int x, y;
	BitmapPair expandedLine[160];
	if (!lowColorBuffer || !realColorBuffer) return;

	int dest_width = Game.Width;
	for (y = 0; y < 120; y++) {
		BitmapCol* srcLine = lowColorBuffer + y * 160;
		BitmapPair* dstLine1 = (BitmapPair*)(realColorBuffer + (y * 2) * dest_width);
		BitmapPair* dstLine2 = (BitmapPair*)(realColorBuffer + (y * 2 + 1) * dest_width);
		for (x = 0; x < 160; x++) {
			BitmapPair pixel = srcLine[x];
			expandedLine[x] = pixel | (pixel << 16);
		}
		Mem_Copy(dstLine1, expandedLine, sizeof(expandedLine));
		Mem_Copy(dstLine2, expandedLine, sizeof(expandedLine));
	}
#endif
}

void Gfx_Begin2D(int width, int height) {
	struct Matrix ortho;
	gfx_rendering2D = true;

#if RENDERING_3D_WIDTH < 320
	// 1. Upscale the completed 3D frame to high-res
	Upscale3D();

	// 2. Switch pointers to high-res colorBuffer for 2D UI drawing
	colorBuffer = realColorBuffer;
	// The 2D pass has depth testing and writes disabled, so it needs no full-resolution depth buffer.
	depthBuffer = NULL;
	fb_width = Game.Width;
	fb_height = Game.Height;
	cb_stride = Game.Width;
	db_stride = Game.Width;
	fb_maxX = Game.Width - 1;
	fb_maxY = Game.Height - 1;

	// 3. Set the 2D viewport
	Gfx_SetViewport(0, 0, Game.Width, Game.Height);
	Gfx_SetScissor (0, 0, Game.Width, Game.Height);
#endif

	Gfx_CalcOrthoMatrix(&ortho, (float)width, (float)height, -100.0f, 1000.0f);
	Gfx_LoadMatrix(MATRIX_PROJ, &ortho);
	Gfx_LoadMatrix(MATRIX_VIEW, &Matrix_Identity);

	Gfx_SetDepthTest(false);
	Gfx_SetDepthWrite(false);
	Gfx_SetAlphaBlending(true);
}

void Gfx_End2D(void) {
	gfx_rendering2D = false;
	Gfx_SetDepthTest(true);
	Gfx_SetDepthWrite(true);
	Gfx_SetAlphaBlending(false);

#if RENDERING_3D_WIDTH < 320
	// Prepare pointers back to low-res 160x120 for the next frame's 3D rendering
	colorBuffer = lowColorBuffer;
	depthBuffer = lowDepthBuffer;
	fb_width = 160;
	fb_height = 120;
	cb_stride = 160;
	db_stride = 160;
	fb_maxX = 159;
	fb_maxY = 119;

	Gfx_SetViewport(0, 0, 160, 120);
	Gfx_SetScissor (0, 0, 160, 120);
#endif
}

cc_bool Gfx_TryRestoreContext(void) { return true; }
#endif
