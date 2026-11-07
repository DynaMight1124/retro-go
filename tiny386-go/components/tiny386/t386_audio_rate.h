#pragma once
/* Application output contract; guest-programmed DSP/PIT rates remain separate. */
#define T386_AUDIO_SAMPLE_RATE 22050u
#define T386_FM_SAMPLE_RATE 11025u
#define T386_FM_REPEAT (T386_AUDIO_SAMPLE_RATE / T386_FM_SAMPLE_RATE)
