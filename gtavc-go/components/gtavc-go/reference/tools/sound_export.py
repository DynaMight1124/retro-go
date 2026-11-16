#!/usr/bin/env python3
"""Pulls the few sound effects the game uses out of Vice City's sfx.raw/sfx.SDT (indices from reVC's AudioSamples.h)
into small 8 kHz-16 kHz mono WAVs in res/ (s_<name>.wav).
  python3 tools/sound_export.py path/to/Audio
"""
import os, sys, struct, wave
import numpy as np
RES = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'res')
# name: (sample index, max seconds, out rate)
PICK = {'punch': (167, 0.4, 11025), 'gun': (50, 0.6, 11025), 'bat': (165, 0.5, 11025), 'swing': (168, 0.4, 11025),
        'crash': (136, 0.8, 11025), 'dooropen': (17, 0.6, 11025), 'doorclose': (18, 0.6, 11025), 'horn': (1, 0.5, 11025),
        'pickup': (366, 0.9, 11025), 'explode': (47, 1.6, 11025), 'splash': (324, 0.8, 11025), 'hitped': (115, 0.5, 11025),
        'siren': (27, 3.8, 11025), 'idle': (288, 0.5, 11025), 'rev': (268, 0.33, 11025)}

VOICES = {  # out name: sample enum name (reVC AudioSamples.h)
    'v_mpanic0': 'SFX_GENERIC_MALE_PANIC_1', 'v_mpanic1': 'SFX_GENERIC_MALE_PANIC_5', 'v_mpanic2': 'SFX_GENERIC_MALE_PANIC_9',
    'v_fpanic0': 'SFX_GENERIC_FEMALE_PANIC_1', 'v_fpanic1': 'SFX_GENERIC_FEMALE_PANIC_4', 'v_fpanic2': 'SFX_GENERIC_FEMALE_PANIC_7',
    'v_mdeath0': 'SFX_GENERIC_MALE_DEATH_1', 'v_mdeath1': 'SFX_GENERIC_MALE_DEATH_6',
    'v_fdeath0': 'SFX_GENERIC_FEMALE_DEATH_1', 'v_fdeath1': 'SFX_GENERIC_FEMALE_DEATH_4',
    'v_fight0': 'SFX_DEFAULT_VOICE_FIGHT_1', 'v_fight1': 'SFX_DEFAULT_VOICE_FIGHT_3',
    'v_chat0': 'SFX_DEFAULT_VOICE_CHAT_1', 'v_chat1': 'SFX_DEFAULT_VOICE_CHAT_5', 'v_chat2': 'SFX_DEFAULT_VOICE_CHAT_9',
    'v_tommy0': 'SFX_PLAYER_ANGRY_FIGHT_1', 'v_tommy1': 'SFX_PLAYER_PISSED_OFF_FIGHT_2'}

def enum_index():
    import re
    p = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'assets', 'data', 'AudioSamples.h')
    s = open(p).read(); body = s[s.index('enum'):s.index('};', s.index('enum'))]
    idx, out = 0, {}
    for l in body.splitlines()[1:]:
        m = re.match(r'(\w+)\s*(=\s*(\d+))?,?$', l.split('//')[0].strip())
        if not m: continue
        if m.group(3): idx = int(m.group(3))
        out[m.group(1)] = idx; idx += 1
    return out

def wav(path, a, rate, bits=16):
    w = wave.open(path, 'wb'); w.setnchannels(1); w.setframerate(rate)
    if bits == 8: w.setsampwidth(1); w.writeframes((np.clip(a, -32768, 32767) / 256 + 128).astype(np.uint8).tobytes())
    else: w.setsampwidth(2); w.writeframes(np.clip(a, -32768, 32767).astype('<i2').tobytes())
    w.close()

def main(audio):
    sdt = open(os.path.join(audio, 'sfx.SDT'), 'rb').read()
    raw = open(os.path.join(audio, 'sfx.RAW'), 'rb')
    for name, (idx, secs, rate) in PICK.items():
        off, size, freq, ls, le = struct.unpack_from('<5i', sdt, idx * 20)
        raw.seek(off); a = np.frombuffer(raw.read(size), '<i2').astype(np.float32)
        if name in ('idle', 'rev'):               # repeat the sample's loop region to ~2 s: smoother MMAPI looping
            s0, s1 = (ls // 2 if 0 < ls // 2 < len(a) else 0), (le // 2 if 0 < le // 2 <= len(a) else len(a))
            if s1 <= s0 + 100: s0, s1 = 0, len(a)
            seg = a[s0:s1]; a = np.tile(seg, int(2.0 * freq / len(seg)) + 1); secs = 2.0
        a = a[:int(secs * freq)]
        n = int(len(a) * rate / freq)
        a = np.interp(np.linspace(0, len(a) - 1, n), np.arange(len(a)), a)
        w = wave.open(os.path.join(RES, 's_%s.wav' % name), 'wb')
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
        w.writeframes(np.clip(a, -32768, 32767).astype('<i2').tobytes()); w.close()
    ix = enum_index()
    for name, sname in VOICES.items():
        off, size, freq, ls, le = struct.unpack_from('<5i', sdt, ix[sname] * 20)
        raw.seek(off); a = np.frombuffer(raw.read(size), '<i2').astype(np.float32)[:int(2.2 * freq)]
        n = int(len(a) * 8000 / freq)
        wav(os.path.join(RES, '%s.wav' % name), np.interp(np.linspace(0, len(a) - 1, n), np.arange(len(a)), a), 8000, 8)
    print('sounds:', ', '.join(PICK), '+', len(VOICES), 'voices')

if __name__ == '__main__':
    main(sys.argv[1])
