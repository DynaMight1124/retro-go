#pragma once
#include <rg_surface.h>
typedef bool (*Screenshot_sink)(void *,const void *,size_t);
bool Screenshot_write(const rg_surface_t *,int,int,Screenshot_sink,void *);
bool Screenshot_save(const rg_surface_t *,const char *,int,int);
