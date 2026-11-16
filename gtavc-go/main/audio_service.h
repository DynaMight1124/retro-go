#ifndef GTAVC_AUDIO_SERVICE_H
#define GTAVC_AUDIO_SERVICE_H
#include <stdbool.h>
#define GTAVC_AUDIO_RATE 22050
#define GTAVC_AUDIO_FRAMES 368
bool AudioService_start(void);
/* Nestable. Returns only after the current mix/submit is finished. */
void AudioService_pause(void);
void AudioService_resume(void);
/* Quiesce before audio/storage teardown. Repeated stop is safe. */
void AudioService_stop(void);
#endif
