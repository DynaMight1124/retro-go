#pragma once
#include <stdbool.h>
#include <stdint.h>

#define MK64_RG_AUDIO_RATE 16000
#define MK64_RG_AUDIO_NATIVE_RATE 26800

bool mk64_rg_audio_enabled(void);
bool mk64_rg_audio_requested(void);
void mk64_rg_audio_set_enabled(bool enabled);
void mk64_rg_audio_ensure_init(void);
void mk64_rg_audio_pause(bool paused);
void mk64_rg_audio_shutdown(void);
void mk64_rg_audio_report(void);
uint32_t port_audio_out_queued_bytes(void);
void mk64_rg_mix_submit(unsigned job);
void mk64_rg_mix_wait(unsigned target);
unsigned mk64_rg_audio_target_bytes(void);

void mk64_rg_audio_note_sequence(uint16_t sequence);
void mk64_rg_audio_note_spec(uint8_t mode, uint8_t spec);
void mk64_rg_audio_record_frame(uint32_t elapsed_us);
void mk64_rg_audio_record_queue(void);
