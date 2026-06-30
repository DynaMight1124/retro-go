#ifdef __cplusplus
extern "C" {
#endif
#ifndef odx_WRAPPER
#define odx_WRAPPER

extern void odx_clear_video(); // for mame.cpp, to clear screen before running game

#define printf odx_printf
extern void odx_printf(char* fmt, ...);

//#define memcpy fast_memcpy
extern void* fast_memcpy(void *OUT, const void *IN, size_t N);
extern "C" {
void* memcpy32(void *OUT, const void *IN, size_t N);
}


//#define memset fast_memset
extern void* fast_memset(const void *DST, int C, size_t LENGTH);
extern "C" {
void* memset32(const void *DST, int C, size_t LENGTH);
}

#endif
#ifdef __cplusplus
}
#endif
