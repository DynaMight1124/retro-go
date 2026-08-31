/* strl shim - strlcpy may already exist on ESP-IDF via newlib */
#ifndef COMPAT_STRL_H
#define COMPAT_STRL_H

#include <string.h>

#ifndef strlcpy
static inline size_t compat_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);
    if (size > 0) {
        size_t cp = (len < size - 1) ? len : size - 1;
        memcpy(dst, src, cp);
        dst[cp] = '\0';
    }
    return len;
}
#define strlcpy compat_strlcpy
#endif

#endif
