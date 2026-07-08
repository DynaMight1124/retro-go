#ifndef __FS_UTILS_H__
#define __FS_UTILS_H__
#include <stdio.h>

FILE *fopen_home(const char *filename, const char *mode);
int file_exists_home(const char *filename);

#endif /* __FS_UTILS_H__ */ 
