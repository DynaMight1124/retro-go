"""Exercise the actual adapter with a deterministic worker/output scheduler."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
from test_rg_render import ROOT, compile_host, function

class AudioLifecycleTests(unittest.TestCase):
    def test_disabled_frame_and_sequence_tick_do_no_work(self):
        game = (ROOT / "src/main.c").read_text()
        external = (ROOT / "src/audio/external.c").read_text()
        tick = function(external, "void func_800CB2C4(")
        calls = re.findall(r"^    (func_\w+)\([^;]*\);", tick, flags=re.M)
        harness = """
#include <stdbool.h>
#include <assert.h>
typedef unsigned u32;
typedef int s32;
#define RETRO_GO 1
#define PORT_ENABLE_AUDIO 1
#define RG_AUDIO_GUARD() do { if (!mk64_rg_audio_enabled()) return; } while (0)
static bool enabled;
static unsigned sequence_calls, synthesis_calls, inits, queued;
static bool mk64_rg_audio_enabled(void) { return enabled; }
static void mk64_rg_audio_ensure_init(void) { ++inits; }
static unsigned mk64_rg_audio_target_bytes(void) { return 5360; }
static unsigned port_time_us(void) {return 0;}
static void mk64_rg_audio_record_frame(unsigned us) {(void)us;}
static void mk64_rg_audio_record_queue(void) {}
static u32 port_audio_out_queued_bytes(void) { return queued; }
static void create_next_audio_frame_task(void) { ++synthesis_calls; queued+=1800; }
"""
        for name in calls:
            harness += "static void " + name + "() { ++sequence_calls; }\n"
        harness += function(game, "void port_audio_frame(") + tick + """
int main(void) {
    for(int i=0;i<1000;++i) {port_audio_frame();func_800CB2C4();}
    assert(!sequence_calls && !synthesis_calls && !inits);
    enabled=true;
    port_audio_frame();func_800CB2C4();
    assert(inits==1 && synthesis_calls==2 && sequence_calls>0);
    unsigned a=synthesis_calls,b=sequence_calls;
    queued=1000000;port_audio_frame();
    assert(synthesis_calls==a+2);a=synthesis_calls;
    enabled=false;
    for(int i=0;i<1000;++i) {port_audio_frame();func_800CB2C4();}
    assert(synthesis_calls==a && sequence_calls==b && inits==2);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            result = subprocess.run([str(exe)],capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stderr)

    def test_owned_pcm_disable_drain_lazy_init_and_music_resume(self):
        source = (ROOT / "src/port/rg/audio_rg.c").read_text()
        source = re.sub(r'^#include[^\n]*', '', source, flags=re.M)
        header = (ROOT / "src/port/rg/audio_rg.h").read_text()
        header = re.sub(r'^#(?:include|pragma)[^\n]*', '', header, flags=re.M)
        harness = (ROOT / "src/port/rg/menu_timing.h").read_text() + r"""
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
Mk64MenuTiming sMk64MenuTiming;
#define __ATOMIC_ACQUIRE 0
#define __ATOMIC_RELEASE 0
#define __ATOMIC_RELAXED 0
#define __ATOMIC_ACQ_REL 0
#define __atomic_load_n(p,o) (*(p))
#define __atomic_store_n(p,v,o) (*(p)=(v))
#define __atomic_add_fetch(p,v,o) (*(p)+=(v))
#define __atomic_sub_fetch(p,v,o) (*(p)-=(v))
#define __atomic_or_fetch(p,v,o) (*(p)|=(v))
#define RG_PANIC(x) abort()
#define RG_LOGD(...)
#define RG_LOGI(...) ((void)0)
#define MEM_FAST 1
#define RG_TASK_PRIORITY_2 2
#define RG_TASK_MSG_STOP -1
#define AIBUFFER_LEN 2560
#define NUMAIBUFFERS 3
#define MIXJ_JOBS 4
struct MixJob { int dummy; };
typedef struct MixJob MixJob;
static MixJob gMixJobs[4];
static struct { unsigned submitted, completed; } gMixShared;
static int16_t gAiBufferLengths[3];
static int gAudioHeapSize=0x48c00;
typedef struct {int16_t left,right;} rg_audio_frame_t;
typedef struct {int dummy;} rg_task_t;
typedef struct {int type; union {unsigned dataInt; const void *dataPtr;};} rg_task_msg_t;
static rg_task_t tasks[2];
static struct {rg_task_msg_t message; int destination; bool consumed;} queue[32];
static unsigned head,tail,clock_us,init_count,reset_count,outputs,mixed,rate,mute,music;
static bool inspect_pcm, mix_during_pcm;
static void (*worker_fn[2])(void *);
static unsigned worker_count,current_worker;
static int64_t rg_system_timer(void) { return clock_us; }
static bool rg_task_receive(rg_task_msg_t *m,int t) {
    (void)t;
    for(unsigned i=0;i<tail;++i) {
        if(!queue[i].consumed && queue[i].destination==(int)current_worker) {
            *m=queue[i].message; queue[i].consumed=true; ++head; return true;
        }
    }
    return false;
}
static bool rg_task_send(rg_task_t *t,const rg_task_msg_t *m,int timeout) {
    (void)timeout; assert(tail<32);
    queue[tail].message=*m;queue[tail].destination=(int)(t-tasks);++tail;return true;
}
static void run_workers(void) {
    for(current_worker=0;current_worker<worker_count;++current_worker) worker_fn[current_worker](NULL);
}
static void rg_task_delay(unsigned ms) { (void)ms; assert(head<tail);run_workers(); }
static rg_task_t *rg_task_create(const char *name,void(*fn)(void*),void*arg,int stack,int slots,int priority,int core) {
    (void)name;(void)arg;assert(core==1 && stack>=3072 && slots>=4 && priority==2);
    assert(worker_count<2);worker_fn[worker_count]=fn;return &tasks[worker_count++];
}
static void *rg_alloc(size_t n,int flags){assert(flags==MEM_FAST);return calloc(1,n);}
static void mix_job_execute(const MixJob *j,unsigned mask) { (void)j;assert(!mask);++mixed;clock_us+=100; }
static void rg_audio_set_sample_rate(unsigned r){rate=r;}
static void rg_audio_set_mute(bool m){mute=m;}
static void audio_init(void){++init_count;}
static void func_800C5CB8(void){++reset_count;}
static void func_800CA008(uint8_t m,uint8_t s){(void)m;(void)s;}
static void play_sequence(uint16_t s){music=s;}
static void rg_audio_submit(const rg_audio_frame_t*,size_t);
""" + header + source + r"""
static void rg_audio_submit(const rg_audio_frame_t *samples,size_t count) {
    if(inspect_pcm) {
        assert(sSlots[0].owned==1); /* ownership lasts THROUGH submission */
        assert(count==4 && samples[0].left==123 && samples[3].right==-99);
    }
    if(mix_during_pcm) {
        /* Model a blocked PCM driver while a later DSP job needs to run. */
        assert(sSlots[0].owned);
        unsigned previous=current_worker;current_worker=0;worker_fn[0](NULL);current_worker=previous;
        assert(gMixShared.completed==1 && sSlots[0].owned);
    }
    outputs+=(unsigned)count;
    clock_us+=(unsigned)(count*1000000/rate);
}
int main(void) {
    rate=16000;
    mk64_rg_audio_set_enabled(false);
    mk64_rg_audio_ensure_init();assert(init_count==0 && !sWorker);
    int16_t samples[8]={123,2,3,4,5,6,7,-99};
    port_audio_out_push(samples,sizeof(samples));assert(tail==0);
    mk64_rg_audio_note_sequence(17);
    mk64_rg_audio_note_spec(0,2);
    mk64_rg_audio_set_enabled(true);
    mk64_rg_audio_ensure_init();mk64_rg_audio_ensure_init();
    assert(init_count==1 && reset_count==1 && music==17);
    inspect_pcm=true;
    port_audio_out_push(samples,sizeof(samples));
    memset(samples,0,sizeof(samples)); /* engine may reuse its source */
    assert(sSlots[0].owned && outputs==0 && port_audio_out_queued_bytes()==16);
    mix_during_pcm=true;mk64_rg_mix_submit(0);
    current_worker=1;worker_fn[1](NULL);
    assert(!sSlots[0].owned && outputs==4 && !sQueuedFrames);
    inspect_pcm=mix_during_pcm=false;
    mk64_rg_mix_submit(1);
    port_audio_out_push(samples,sizeof(samples));
    mk64_rg_audio_set_enabled(false); /* must finish DSP and discard queued PCM */
    assert(mixed==2 && gMixShared.completed==2 && outputs==4 && !sQueuedFrames);
    unsigned before=tail;
    port_audio_out_push(samples,sizeof(samples));mk64_rg_audio_ensure_init();
    assert(tail==before && init_count==1 && port_audio_out_queued_bytes()==0);
    mk64_rg_audio_note_sequence(23);
    mk64_rg_audio_pause(true);mk64_rg_audio_set_enabled(true);
    mk64_rg_audio_ensure_init();assert(music==17); /* menu still pauses sound */
    mk64_rg_audio_pause(false);mk64_rg_audio_ensure_init();assert(music==23 && !mute);
    port_audio_out_push(samples,sizeof(samples));
    mk64_rg_audio_pause(true);assert(!sQueuedFrames && outputs==4);
    mk64_rg_audio_pause(false);
    port_audio_out_set_rate(32000);assert(rate==32000 && mk64_rg_audio_target_bytes()==6400);
    sStopped=0;mk64_rg_audio_shutdown();assert(!sWorker && !sOutputWorker && sStopped==3);
    assert(head==tail);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as directory:
            exe = compile_host(harness, Path(directory))
            result = subprocess.run([str(exe)],capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stderr)

if __name__ == "__main__": unittest.main()
