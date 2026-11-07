#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
typedef struct {
    uint64_t seek_us,read_us,max_offset;
    uint64_t requested_bytes,total_bytes;
    unsigned calls,failures,slow,max_us,max_seek_us,max_read_us;
    size_t max_count;
} T386FileProfile;
const T386FileProfile *t386_file_profile_get(void);
void t386_file_profile_reset(void);
bool t386_file_read(FILE *f,uint64_t offset,void *buf,size_t count);
bool t386_file_write(FILE *f,uint64_t offset,const void *buf,size_t count);
bool t386_file_write_current(FILE *f,const void *buf,size_t count);
bool t386_file_flush(FILE *f);
bool t386_file_close(FILE **f);
bool t386_file_load_rom(void *memory,size_t capacity,const char *path,uint32_t address,bool backward,int *loaded);
