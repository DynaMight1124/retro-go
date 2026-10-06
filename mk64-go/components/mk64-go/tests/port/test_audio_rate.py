"""Check real synthesis configuration and pitch at the reduced output rate."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function

class AudioRateTests(unittest.TestCase):
    def test_engine_rate_update_count_tempo_and_pitch(self):
        heap = (ROOT / "src/audio/heap.c").read_text()
        reset = function(heap, "void audio_reset_session(")
        start = reset.index("    gSampleDmaNumListItems = 0;")
        end = reset.index("    gMaxAudioCmds =", start)
        config = reset[start:end]
        playback = (ROOT / "src/audio/playback.c").read_text()
        header = (ROOT / "src/port/rg/audio_rg.h").read_text()
        constants = "\n".join(re.findall(r'^#define MK64_RG_AUDIO_.*$',header,flags=re.M))
        prefix = r"""
#include <stdint.h>
#include <assert.h>
#include <math.h>
typedef float f32;
typedef int32_t s32;
typedef uint32_t u32;
#define RETRO_GO 1
#define ALIGN16(x) (((x)+15)&~15)
#define stubbed_printf(...) ((void)0)
struct Params {
 int frequency,aiFrequency,samplesPerFrameTarget,minAiBufferLength,maxAiBufferLength;
 int updatesPerFrame,samplesPerUpdate,samplesPerUpdateMax,samplesPerUpdateMin;
 float resampleRate,unkUpdatesPerFrameScaled,updatesPerFrameInv;
 int presetUnk4;
} gAudioBufferParameters;
struct Preset {int frequency,maxSimultaneousNotes,volume,unk1;};
static struct Preset preset={26800,24,32767,1};
static int gRefreshRate=60,gMaxSimultaneousNotes,gVolume,gTatumsPerBeat=48;
static float D_803B7178=16.713f;
static unsigned gTempoInternalToExternal,output_rate,gSampleDmaNumListItems;
static int osAiSetFrequency(unsigned rate) {output_rate=rate;return rate;}
struct NoteSubEu {int hasTwoAdpcmParts,resamplingRateFixedPoint;};
struct Note {struct NoteSubEu noteSubEu;};
"""
        harness = prefix + constants + "\nstatic void configure(void) { struct Preset *temp_s6=&preset;\n" + config + "\n}\n" + function(playback,"void note_set_resampling_rate(") + r"""
int main(void) {
 configure();
 assert(gAudioBufferParameters.frequency==16000 && output_rate==16000);
 assert(gAudioBufferParameters.samplesPerFrameTarget==272);
 assert(gAudioBufferParameters.minAiBufferLength==256 && gAudioBufferParameters.maxAiBufferLength==288);
 assert(gAudioBufferParameters.updatesPerFrame==2 && gMaxSimultaneousNotes==24);
 /* Tempo threshold must match the number of sequence updates, keeping BPM. */
 unsigned expected=(unsigned)((2*2880000.0f/48)/16.713f);
 assert(gTempoInternalToExternal==expected);
 for(int i=1;i<=20;++i) {
   float original=(float)i/10; struct Note note={0};
   note_set_resampling_rate(&note,original);
   float ratio=note.noteSubEu.resamplingRateFixedPoint/32768.0f;
   if(note.noteSubEu.hasTwoAdpcmParts) ratio*=2;
   float native_hz=original*26800,lower_hz=ratio*16000;
   assert(fabsf(native_hz-lower_hz)<1.1f);
 }
 return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness,Path(directory))
            result = subprocess.run([str(exe)],capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stderr)

if __name__ == "__main__": unittest.main()
