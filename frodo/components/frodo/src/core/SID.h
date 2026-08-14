#pragma once

#include <cstddef>
#include <cstdint>

struct MOS6581State {
    uint8_t registers[32];
    uint32_t phase[3];
    uint32_t phase_step[3];
    uint32_t noise[3];
    uint32_t envelope_counter[3];
    uint8_t envelope[3];
    uint8_t previous_gate[3];
    uint8_t stage[3];
    uint32_t line_sample_accumulator;
};

class MOS6581 {
public:
    explicit MOS6581(uint32_t sample_rate);

    void Reset();
    uint8_t ReadRegister(uint16_t adr);
    void WriteRegister(uint16_t adr, uint8_t byte);
    void BeginFrame();
    void EmulateLine();
    const int16_t *AudioSamples(size_t &stereo_frame_count) const;
    void GetState(MOS6581State *state) const;
    void SetState(const MOS6581State *state);

private:
    enum EnvelopeStage : uint8_t { ATTACK, DECAY, SUSTAIN, RELEASE };
    struct Voice {
        uint32_t phase;
        uint32_t phase_step;
        uint32_t noise;
        uint32_t envelope_counter;
        uint8_t envelope;
        uint8_t previous_gate;
        EnvelopeStage stage;
    };

    int16_t render_voice(unsigned voice_index);
    void render_sample();
    void update_phase_step(unsigned voice_index);

    static constexpr size_t MAX_AUDIO_FRAMES = 512;
    static constexpr uint32_t PAL_LINES_PER_SECOND = 50 * 312;

    uint8_t registers[32];
    Voice voices[3];
    uint32_t sample_rate;
    uint32_t line_sample_accumulator;
    uint32_t attack_period[16];
    uint32_t decay_release_period[16];
    size_t audio_frame_count;
    int16_t audio_buffer[MAX_AUDIO_FRAMES * 2];
};
