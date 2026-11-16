#ifndef GTAVC_TILE_LOADER_H
#define GTAVC_TILE_LOADER_H
#include "tile.h"
typedef struct { char path[600]; int i,j,k,ox,oz,tileMm; } TileLoadRequest;
typedef enum { TL_OK,TL_NO_CACHE,TL_IO_ERROR,TL_ALLOC_ERROR,TL_DECODE_ERROR,TL_CANCELLED } TileLoadStatus;
typedef struct {
    uint64_t open_us,alloc_us,read_us,decode_us;
    uint32_t read_calls,decode_calls,read_peak_us,decode_peak_us,bytes_read;
    int file_bytes;
} TileLoadStats;
typedef struct {
    Tile *tile; int i,j,k; TileLoadStatus status;
    TileLoadStats stats; uint64_t elapsed_us;
} TileLoadResult;
typedef bool (*TileLoadCancelled)(void *arg);
#define TILE_TEXTURE_BATCH 16
typedef struct {
    char path[600];
    int count, indices[TILE_TEXTURE_BATCH], ids[TILE_TEXTURE_BATCH];
} TileTextureRequest;
typedef struct {
    int count, indices[TILE_TEXTURE_BATCH];
    int *pixels[TILE_TEXTURE_BATCH];
    TileLoadStatus status;
    uint64_t elapsed_us, open_us, read_us;
    uint32_t bytes_read, read_peak_us;
} TileTextureResult;
/* Owns no Tile/Assets state. Result pixels transfer to caller or freeTextures. */
void TileLoader_readTextures(const TileTextureRequest*, TileTextureResult*, TileLoadCancelled, void*);
void TileLoader_freeTextures(TileTextureResult*);
bool TileLoader_submitTextures(const TileTextureRequest*);
bool TileLoader_pollTextures(TileTextureResult*);
/* Worker retains a private atlas between batches; cancel/drain closes it.
   The standalone readTextures backend still closes before returning. */
/* Backend owns temporary file/decoder/buffer; result tile transfers to caller. */
void TileLoader_decodeFile(const TileLoadRequest *request,TileLoadResult *result,TileLoadCancelled cancelled,void *arg);
bool TileLoader_start(void);
/* Conservative nonblocking check for queued/running/draining background I/O. */
bool TileLoader_ioBusy(void);
bool TileLoader_submit(const TileLoadRequest *request);
bool TileLoader_poll(TileLoadResult *result);
void TileLoader_cancel(void);
/* Cancel and wait for file closure/partial cleanup; use only at lifecycle pauses. */
void TileLoader_waitIdle(void);
#endif
