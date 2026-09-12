#include "shared.h"


void app_main(void)
{
    rg_app_t *app = rg_system_init(&(const rg_config_t){
        .sampleRate = 22050,
        .frameRate = 30,
        .storageRequired = true,
        .romRequired = false,
        .mallocAlwaysInternal = 1,
    });

    RG_LOGI("configNs=%s", app->configNs);

    if (strcmp(app->configNs, "cannonball") == 0)
        cannonball_main();
    else if (strcmp(app->configNs, "celeste") == 0)
        celeste_main();
    else if (strcmp(app->configNs, "opentyrian") == 0)
        opentyrian_main();
    else
    {
        RG_LOGE("Unknown application namespace: %s", app->configNs);
        rg_system_exit();
    }

    RG_PANIC("Never reached");
}
