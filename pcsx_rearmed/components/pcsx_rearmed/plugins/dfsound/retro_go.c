#include <stddef.h>

#include <rg_audio.h>

#include "out.h"

static int retro_go_init(void)
{
    /* rg_system_init() owns and initializes the selected audio sink. */
    return 0;
}

static void retro_go_finish(void)
{
    /* The application lifecycle, not the SPU plugin, owns the sink. */
}

static int retro_go_busy(void)
{
    /* Tempo adjustment is disabled for this frontend, so no queue-depth
     * approximation is required here. */
    return 1;
}

static void retro_go_feed(void *data, int bytes)
{
    if (data != NULL && bytes >= (int)sizeof(rg_audio_frame_t))
        rg_audio_submit((const rg_audio_frame_t *)data,
                        (size_t)bytes / sizeof(rg_audio_frame_t));
}

void out_register_retro_go(struct out_driver *drv)
{
    drv->name = "retro-go";
    drv->init = retro_go_init;
    drv->finish = retro_go_finish;
    drv->busy = retro_go_busy;
    drv->feed = retro_go_feed;
}
