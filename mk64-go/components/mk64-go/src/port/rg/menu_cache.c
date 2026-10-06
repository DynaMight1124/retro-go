/* One immutable ROM background, independent of the menu's recycled heap. */
#include <string.h>
#include <rg_utils.h>
#include "menu_cache.h"
static uint8_t *sBackground;
static uintptr_t sSource;
static unsigned sAlpha;
static bool sValid;
bool mk64_menu_cache_restore(uintptr_t source, unsigned width, unsigned height, unsigned alpha, void *output)
{
    if (!sValid || width != 320 || height != 240 || source != sSource || alpha != sAlpha)
        return false;
    memcpy(output, sBackground, 320 * 240 * 2);
    return true;
}
void mk64_menu_cache_store(uintptr_t source, unsigned width, unsigned height, unsigned alpha, const void *pixels)
{
    if (width != 320 || height != 240) return;
    if (!sBackground) sBackground = rg_alloc(320 * 240 * 2, MEM_SLOW | MEM_NOPANIC);
    if (!sBackground) return;
    memcpy(sBackground, pixels, 320 * 240 * 2);
    sSource = source;
    sAlpha = alpha;
    sValid = true;
}
