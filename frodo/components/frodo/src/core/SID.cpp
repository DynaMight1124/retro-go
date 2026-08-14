#include "SID.h"

#include <algorithm>
#include <cstring>

namespace {

constexpr uint32_t PAL_CLOCK_HZ = 985248;
constexpr uint16_t attack_ms[16] = {
    2, 8, 16, 24, 38, 56, 68, 80, 100, 250, 500, 800, 1000, 3000, 5000, 8000
};
constexpr uint16_t decay_release_ms[16] = {
    6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400,
    3000, 9000, 15000, 24000
};

// The 6581 noise DAC is fed by non-contiguous bits of a 23-bit LFSR. The
// oscillator accumulator is stored here shifted left by eight bits, so SID
// accumulator bit 19 corresponds to phase bit 27.
constexpr uint32_t NOISE_CLOCK_PERIOD = 1u << 28;
constexpr uint32_t NOISE_CLOCK_EDGE = 1u << 27;

uint8_t noise_output(uint32_t shift_register)
{
    return static_cast<uint8_t>(
        (((shift_register >> 22) & 1u) << 7) |
        (((shift_register >> 20) & 1u) << 6) |
        (((shift_register >> 16) & 1u) << 5) |
        (((shift_register >> 13) & 1u) << 4) |
        (((shift_register >> 11) & 1u) << 3) |
        (((shift_register >>  7) & 1u) << 2) |
        (((shift_register >>  4) & 1u) << 1) |
        (((shift_register >>  2) & 1u) << 0));
}

void clock_noise(uint32_t &shift_register)
{
    const uint32_t feedback =
        ((shift_register >> 22) ^ (shift_register >> 17)) & 1u;
    shift_register = ((shift_register << 1) | feedback) & 0x7fffffu;
}

} // namespace

MOS6581::MOS6581(uint32_t requested_sample_rate)
    : sample_rate(requested_sample_rate), line_sample_accumulator(0),
      audio_frame_count(0)
{
    for (unsigned rate = 0; rate < 16; ++rate) {
        attack_period[rate] = std::max<uint32_t>(
            1, (uint32_t(attack_ms[rate]) * sample_rate) / (255u * 1000u));
        decay_release_period[rate] = std::max<uint32_t>(
            1, (uint32_t(decay_release_ms[rate]) * sample_rate) / (255u * 1000u));
    }
    Reset();
}

void MOS6581::Reset()
{
    std::memset(registers, 0, sizeof(registers));
    std::memset(audio_buffer, 0, sizeof(audio_buffer));
    line_sample_accumulator = 0;
    audio_frame_count = 0;
    for (auto &voice : voices) {
        voice = {};
        voice.noise = 0x7ffff8;
        voice.stage = RELEASE;
    }
}

uint8_t MOS6581::ReadRegister(uint16_t adr)
{
    adr &= 0x1f;
    if (adr == 0x1b) return static_cast<uint8_t>(voices[2].phase >> 24);
    if (adr == 0x1c) return voices[2].envelope;
    return 0xff;
}

void MOS6581::WriteRegister(uint16_t adr, uint8_t byte)
{
    adr &= 0x1f;
    registers[adr] = byte;
    if (adr < 21 && (adr % 7) < 2) {
        update_phase_step(adr / 7);
    }
}

void MOS6581::BeginFrame()
{
    audio_frame_count = 0;
}

void MOS6581::GetState(MOS6581State *state) const
{
    std::memcpy(state->registers, registers, sizeof(registers));
    for (unsigned i = 0; i < 3; ++i) {
        state->phase[i] = voices[i].phase;
        state->phase_step[i] = voices[i].phase_step;
        state->noise[i] = voices[i].noise;
        state->envelope_counter[i] = voices[i].envelope_counter;
        state->envelope[i] = voices[i].envelope;
        state->previous_gate[i] = voices[i].previous_gate;
        state->stage[i] = static_cast<uint8_t>(voices[i].stage);
    }
    state->line_sample_accumulator = line_sample_accumulator;
}

void MOS6581::SetState(const MOS6581State *state)
{
    std::memcpy(registers, state->registers, sizeof(registers));
    for (unsigned i = 0; i < 3; ++i) {
        voices[i].phase = state->phase[i];
        voices[i].phase_step = state->phase_step[i];
        voices[i].noise = state->noise[i];
        voices[i].envelope_counter = state->envelope_counter[i];
        voices[i].envelope = state->envelope[i];
        voices[i].previous_gate = state->previous_gate[i];
        voices[i].stage = static_cast<EnvelopeStage>(state->stage[i]);
    }
    line_sample_accumulator = state->line_sample_accumulator;
    audio_frame_count = 0;
}

void MOS6581::update_phase_step(unsigned voice_index)
{
    const unsigned base = voice_index * 7;
    const uint16_t frequency = uint16_t(registers[base]) |
                               (uint16_t(registers[base + 1]) << 8);
    voices[voice_index].phase_step = sample_rate == 0 ? 0 :
        static_cast<uint32_t>(
            (uint64_t(frequency) * PAL_CLOCK_HZ * 256u) / sample_rate);
}

int16_t MOS6581::render_voice(unsigned voice_index)
{
    Voice &voice = voices[voice_index];
    const unsigned base = voice_index * 7;
    const uint8_t control = registers[base + 4];
    const uint8_t gate = control & 1;
    if (gate != voice.previous_gate) {
        voice.stage = gate ? ATTACK : RELEASE;
        voice.previous_gate = gate;
        voice.envelope_counter = 0;
    }

    const uint8_t ad = registers[base + 5];
    const uint8_t sr = registers[base + 6];
    const uint8_t sustain = (sr >> 4) * 17;
    const unsigned rate = voice.stage == ATTACK ? (ad >> 4) :
                          voice.stage == DECAY ? (ad & 15) : (sr & 15);
    const uint32_t period = voice.stage == ATTACK
        ? attack_period[rate & 15]
        : decay_release_period[rate & 15];
    if (++voice.envelope_counter >= period) {
        voice.envelope_counter = 0;
        if (voice.stage == ATTACK) {
            if (voice.envelope < 255) ++voice.envelope;
            else voice.stage = DECAY;
        } else if (voice.stage == DECAY) {
            if (voice.envelope > sustain) --voice.envelope;
            else voice.stage = SUSTAIN;
        } else if (voice.stage == RELEASE && voice.envelope > 0) {
            --voice.envelope;
        }
    }

    const uint32_t old_phase = voice.phase;
    if (control & 0x08) {
        // TEST holds the oscillator accumulator at zero and stops the noise
        // clock. This is used by some sound effects to restart a waveform.
        voice.phase = 0;
    } else {
        voice.phase += voice.phase_step;
    }

    uint8_t waveform = 0x80;
    if (control & 0x80) {
        if (!(control & 0x08)) {
            // Clock the LFSR on every rising edge of SID accumulator bit 19.
            // A high programmed frequency can cross more than one edge during
            // a 22.05 kHz output sample, so count edges rather than testing a
            // single before/after bit.
            const uint64_t start = uint64_t(old_phase) + NOISE_CLOCK_EDGE;
            const uint64_t end = uint64_t(old_phase) + voice.phase_step +
                                 NOISE_CLOCK_EDGE;
            unsigned clocks = static_cast<unsigned>(
                end / NOISE_CLOCK_PERIOD - start / NOISE_CLOCK_PERIOD);
            while (clocks-- > 0) clock_noise(voice.noise);
        }
        waveform = noise_output(voice.noise);
    } else if (control & 0x40) {
        const uint16_t pulse_width = uint16_t(registers[base + 2]) |
                                     (uint16_t(registers[base + 3] & 15) << 8);
        waveform = (voice.phase >> 20) < pulse_width ? 0xff : 0x00;
    } else if (control & 0x20) {
        waveform = static_cast<uint8_t>(voice.phase >> 24);
    } else if (control & 0x10) {
        const uint8_t ramp = static_cast<uint8_t>(voice.phase >> 23);
        waveform = (voice.phase & 0x80000000u) ? static_cast<uint8_t>(~ramp) : ramp;
    }

    return static_cast<int16_t>((int(waveform) - 128) * voice.envelope);
}

void MOS6581::render_sample()
{
    int32_t mixed = 0;
    for (unsigned voice = 0; voice < 3; ++voice) {
        if (voice == 2 && (registers[0x18] & 0x80)) continue;
        mixed += render_voice(voice);
    }
    mixed = (mixed * (registers[0x18] & 15)) / (15 * 3);
    mixed = std::clamp<int32_t>(mixed, -32768, 32767);
    if (audio_frame_count < MAX_AUDIO_FRAMES) {
        audio_buffer[audio_frame_count * 2] = static_cast<int16_t>(mixed);
        audio_buffer[audio_frame_count * 2 + 1] = static_cast<int16_t>(mixed);
        ++audio_frame_count;
    }
}

void MOS6581::EmulateLine()
{
    line_sample_accumulator += sample_rate;
    while (line_sample_accumulator >= PAL_LINES_PER_SECOND) {
        line_sample_accumulator -= PAL_LINES_PER_SECOND;
        render_sample();
    }
}

const int16_t *MOS6581::AudioSamples(size_t &stereo_frame_count) const
{
    stereo_frame_count = audio_frame_count;
    return audio_buffer;
}
