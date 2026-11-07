#include "rg_system.h"
#include "t386_config.h"
#include "t386_input.h"
#include "t386_controls.h"
#include "t386_platform.h"
#include "t386_clock.h"
#include "t386_runtime.h"
#include "t386_file.h"
#include "t386_audio_rate.h"
#include "t386_audio_output.h"
#include "t386_ui.h"
#include "fmopl.h"
#include "esp_memory_utils.h"
#include <string.h>
#include <limits.h>

static uint8_t *native_frame;
static uint16_t render_palette[256];

static rg_app_t *app;
static PC *pc;
static rg_surface_t *working,*present;
static rg_surface_t *rgb_present,*indexed_present;
static bool have_frame, shutting_down;
static T386Config config;
static T386Schedule schedule;
static T386Video video;
static unsigned batch_estimate=512;
static uint64_t busy;
static float speed;
static bool keyboard_delivering;
static uint64_t keyboard_next_step;
static void run_chunk(void);
static bool present_frame(void);
static void keyboard_session(void);

/* Reuse scheduler measurements; no per-instruction or I/O probes. */
#if !RG_BUILD_RELEASE
static void report_debug_summary(uint64_t now) {
    static uint64_t next_report;
    if(now<next_report)return;
    next_report=now+5000000;
    RG_LOGD("Tiny386: %ux%u %s; cap=%u batch512=%uus video=%uus audio=%s",
        have_frame?(unsigned)present->width:0u,have_frame?(unsigned)present->height:0u,
        have_frame&&present==indexed_present?"indexed":"RGB565",video.limit_fps,batch_estimate,
        (unsigned)video.cost_us,adlib_get_enabled(pc->adlib)?"AdLib":"PC speaker");
}
#else
#define report_debug_summary(now) ((void)0)
#endif

void t386_input_emit(int key,int down) {if(pc) ps2_put_keycode(pc->kbd,down,key);}
static bool flush_disks(void) {
    if(!pc) return true;
    bool ok=ide_flush_files(pc->ide);
    ok=ide_flush_files(pc->ide2)&&ok;
    ok=emulink_flush_files(pc->emulink)&&ok;
    return ok;
}
static void redraw(void *opaque,int x,int y,int w,int h) {
    (void)opaque;(void)x;(void)y;(void)w;(void)h;
}
static bool screenshot(const char *path,int width,int height) {
    return have_frame && rg_surface_save_image_file(present,path,width,height);
}
static bool reset(bool hard) {
    (void)hard;
    if(!flush_disks()) {rg_gui_alert(_("Disk error"),_("Could not flush the disk image. Reset cancelled."));return false;}
    /* A reboot resets every peripheral and allocation, not only CPU registers. */
    rg_system_switch_app(NULL,app->configNs,app->romPath,0,RG_BOOT_NORMAL);
    return false;
}
static void event(int id,void *data) {
    (void)data;
    if(id==RG_EVENT_REDRAW && have_frame && !shutting_down)
        rg_display_submit(present,0);
    if(id==RG_EVENT_SHUTDOWN) {
        shutting_down=true; t386_clock_pause(); t386_input_release_all();
        if(!flush_disks()) RG_LOGE("Disk image flush failed at shutdown");
        if(pc) {
            bool ok=ide_close_files(pc->ide);
            ok=ide_close_files(pc->ide2)&&ok;
            ok=emulink_close_files(pc->emulink)&&ok;
            if(!ok) RG_LOGE("Disk image close failed at shutdown");
            if(pc->adlib) {adlib_free(pc->adlib);pc->adlib=NULL;}
        }
    }
}
static void service_input(void) {
    uint32_t keys=rg_input_read_gamepad();
    if(keys&(RG_KEY_MENU|RG_KEY_OPTION)) {
        t386_clock_pause(); t386_input_release_all(); t386_audio_output_pause(true);
        if(!flush_disks()) rg_gui_alert(_("Disk error"),_("Disk image flush failed; changes may not be saved."));
        if(keys&RG_KEY_MENU) rg_gui_game_menu(); else rg_gui_options_menu();
        while(rg_input_read_gamepad()) {rg_system_tick(0);rg_task_delay(10);}
        /* Shared menus clear their DMA queue when unmuting. Re-prime through
         * the public API before resuming the guest. */
        t386_audio_output_prime();t386_audio_output_pause(false);
        t386_clock_set_speed(app->speed); t386_clock_resume();
        return;
    }
    bool open=false;
    keys=t386_input_shortcuts(keys,t386_host_time_us(),&open);
    if(open) {keyboard_session();return;}
    t386_input_poll(keys);
}
static bool present_frame(void) {
    int x,y,w,h; vga_get_viewport(pc->vga,&x,&y,&w,&h);
    int stride=vga_get_render_stride(pc->vga);
    const uint16_t *palette=vga_get_render_palette(pc->vga);
    unsigned pixel_bytes=palette?1:2;
    rg_surface_t *target=palette?indexed_present:rgb_present;
    if(!target)return false;
    const unsigned char *src=vga_get_render_data(pc->vga)+y*stride+x*pixel_bytes;
    bool busy=rg_display_is_busy();
    bool copied=palette?t386_present_copy_indexed(src,stride,target->data,w,w,h,palette,target->palette,busy):
        t386_present_copy_rows(src,stride,target->data,w*2,w*2,h,busy);
    if(!copied)return false;
    /* Copy checks ownership before touching pixels/palette. Descriptors and the
     * last complete-frame pointer change only after that snapshot succeeds. */
    if(!have_frame || present!=target || target->width!=w || target->height!=h) {
        RG_LOGD("Video output: %dx%d %s, copy=%u bytes",w,h,palette?"indexed8":"RGB565",(unsigned)(w*h*pixel_bytes));
    }
    target->width=w; target->height=h; target->stride=w*pixel_bytes; target->offset=0;
    present=target;
    have_frame=true;
    rg_display_submit(present,0);
    return true;
}
/* Text and special chords share the same paced make/break delivery. */
static void deliver_queued_input(void) {
    keyboard_delivering=true;keyboard_next_step=0;
    t386_clock_resume();
    while(t386_input_pending())run_chunk();
    uint64_t released_until=t386_clock_now_us()+50000;
    while(t386_clock_now_us()<released_until)run_chunk();
    t386_clock_pause();keyboard_delivering=false;
}
static void refresh_guest_frame(void) {
    while(rg_display_is_busy())rg_task_yield();
    vga_refresh(pc->vga,redraw,NULL,pc->full_update!=0);
    if(pc->full_update==2)pc->full_update=0;
    present_frame();
}
static void special_key_from_menu(int key,unsigned modifiers) {
    while(rg_input_read_gamepad()) {rg_system_tick(0);rg_task_delay(10);}
    if(!t386_input_queue_key(key,modifiers)) {
        rg_gui_alert(_("Special keys"),_("Finish sending the current key first."));return;
    }
    bool was_paused=t386_audio_output_is_paused();
    t386_audio_output_pause(true);
    deliver_queued_input();t386_input_release_all();refresh_guest_frame();
    t386_audio_output_pause(was_paused);busy=0;
    /* The caller is still inside RetroGo's modal options menu. */
    t386_clock_pause();
}
/* While choosing a key DOS is paused. Each selection runs through the normal
 * chunk loop with audio muted, then shows the updated DOS screen before the
 * picker is redrawn. This also gives repeated keys a distinct released period. */
static void keyboard_session(void) {
    int cursor=0;bool shift=false;
    bool was_paused=t386_audio_output_is_paused();
    t386_clock_pause();t386_input_release_all();t386_audio_output_pause(true);
    for(;;) {
        while(rg_display_is_busy())rg_task_yield();
        if(have_frame) {
            rg_display_submit(present,0);
            while(rg_display_is_busy())rg_task_yield();
        }
        int key=t386_ui_keyboard_pick(&cursor,&shift);
        if(key<0)break;
        char text[2]={(char)key,0};
        if(!t386_input_queue_text(text))break;
        deliver_queued_input();refresh_guest_frame();
    }
    t386_input_release_all();
    while(rg_input_read_gamepad()) {rg_system_tick(0);rg_task_delay(10);}
    while(rg_display_is_busy())rg_task_yield();
    if(have_frame)rg_display_submit(present,0);
    t386_clock_resume();t386_audio_output_pause(was_paused);
    busy=0;
}
static void keyboard_from_menu(void) {
    keyboard_session();
    /* The caller is still inside Retro-Go's modal options menu. */
    t386_clock_pause();
}
void app_main(void) {
    const rg_config_t system_config={.sampleRate=T386_AUDIO_SAMPLE_RATE,.frameRate=60,.storageRequired=true,.romRequired=true,
        .mallocAlwaysInternal=1,.handlers={.loadState=t386_ui_unsupported_state,.saveState=t386_ui_unsupported_state,
            .reset=reset,.screenshot=screenshot,.event=event,.options=t386_ui_options}};
    app=rg_system_init(&system_config);
    /* Do this before image parsing and core/BIOS allocation can leave the
     * internal DAC playing an unprimed stream for a long startup interval. */
    t386_audio_output_init();
    RG_LOGD("Audio output: signed-silence priming and software keyboard mute");
    char error[256];
    if(!t386_config_load(app->romPath,&config,error,sizeof(error))) {
        rg_gui_alert(_("Tiny386 configuration"),error); rg_system_exit();
    }
    t386_platform_init(config.pc.mem_size);
    working=rg_surface_create(640,480,RG_PIXEL_565_LE,MEM_SLOW);
    rgb_present=rg_surface_create(640,480,RG_PIXEL_565_LE,MEM_SLOW);
    present=rgb_present;
    RG_ASSERT(working&&rgb_present,"Cannot allocate Tiny386 framebuffers");
    /* Reserve native render storage before core/FM allocations. The large
     * PSRAM buffer remains available for text/VBE/unsupported VGA layouts.
     * MEM_FAST may fall back, so verify actual internal placement. */
    native_frame=rg_alloc(320*200*2,MEM_FAST|MEM_NOPANIC);
    if(!native_frame || !esp_ptr_internal(native_frame)) {
        free(native_frame);native_frame=NULL;
        RG_LOGW("Internal native framebuffer unavailable; reverting to internal FM tables");
    }
    OPLSetTableMemorySlow(native_frame!=NULL);
    /* Keep total-level and compact waveform tables fast; remaining tables slow. */
    OPLSetLevelTableFast(native_frame!=NULL);
    OPLSetWaveTableFast(native_frame!=NULL);
    OPLSetEnvelopeTableFast(0);
    RG_LOGD("Memory placement: native framebuffer=%s; FM tables prefer %s",
        native_frame?"internal":"PSRAM",native_frame?"PSRAM":"internal");
    RG_LOGD("FM tables prefer internal: total-level=65536 bytes, compact waveform=16384 bytes");
    pc=pc_new(redraw,NULL,working->data,&config.pc);
    if(native_frame)RG_ASSERT(vga_set_native_buffer(pc->vga,native_frame,320*200*2),"Invalid native framebuffer");
    RG_ASSERT(pc->adlib,"Cannot initialize OPL2 audio");
    /* Keep the tested128KiB render reservation/FM policy. An optional cold
     * indexed submission surface owns immutable copied pixels and palette. */
    if(native_frame) {
        indexed_present=rg_surface_create(320,200,RG_PIXEL_PAL565_LE,MEM_SLOW|MEM_NOPANIC);
        /* MEM_SLOW can fall back internally; optional cold storage must not
         * consume the internal heap needed by the core/audio/tasks. */
        if(indexed_present && !esp_ptr_external_ram(indexed_present)) {
            rg_surface_free(indexed_present);indexed_present=NULL;
        }
        if(indexed_present && !vga_set_native_indexed_buffer(pc->vga,native_frame,128000,render_palette,256)) {
            rg_surface_free(indexed_present);indexed_present=NULL;
        }
        if(!indexed_present)RG_LOGW("Indexed submission unavailable; retaining native RGB565 output");
    }
    RG_LOGD("Native indexed output: %s",indexed_present?"enabled":"disabled (RGB565 fallback)");
    t386_ui_init(pc->adlib);
    t386_ui_set_video(&video);
    t386_ui_set_keyboard_handler(keyboard_from_menu);
    t386_ui_set_special_handler(special_key_from_menu);
    RG_LOGI("Audio: %s; OPL2 guest ports %s; Sound Blaster digital + PC speaker available, %u Hz stereo; OPL2 synthesis %u Hz",
        adlib_get_enabled(pc->adlib)?"AdLib":"PC Speaker",
        adlib_get_enabled(pc->adlib)?"enabled":"absent",
        T386_AUDIO_SAMPLE_RATE,T386_FM_SAMPLE_RATE);
    t386_controls_init(app->romPath,config.keycodes); load_bios_and_reset(pc);
    t386_clock_set_speed(app->speed); t386_clock_resume();
    pc->boot_start_time=get_uticks();
    if(app->bootFlags&RG_BOOT_RESUME) RG_LOGW("VM resume is not supported; booting DOS normally");
    app->frameskip=-1;
    speed=app->speed;
    for(;;) {
        service_input();
        run_chunk();
    }
}

/* Shared by gameplay and keyboard delivery: one scheduler/device/audio path. */
static void run_chunk(void) {
    rg_audio_frame_t audio[64];
    app->frameskip=-1; /* Retain measured scheduling after speed/menu changes. */
    if(speed!=app->speed) {speed=app->speed;t386_clock_set_speed(speed);}
    unsigned count=t386_schedule_chunk(&schedule);
    uint64_t deadline=t386_schedule_target(&schedule,t386_clock_now_us());
    uint64_t start=t386_host_time_us();
    if(!keyboard_delivering || t386_clock_now_us()>=keyboard_next_step) {
        t386_input_step();
        keyboard_next_step=t386_clock_now_us()+50000;
    }
    do {
        uint64_t guest_now=t386_clock_now_us();
        unsigned estimated_guest_us=speed==1.f?batch_estimate:(unsigned)(batch_estimate*speed);
        unsigned budget=t386_cpu_budget(guest_now,deadline,estimated_guest_us);
        uint64_t batch_start=t386_host_time_us();
        pc_service_devices(pc);
        pc_execute_cpu(pc,(int)budget);
        vga_step(pc->vga);
        /* This measurement controls the CPU budget, not a diagnostic report. */
        batch_estimate=t386_cpu_observe(batch_estimate,t386_host_time_us()-batch_start,budget);
        if(pc->shutdown_state==8)rg_system_exit();
    } while(t386_clock_now_us()<deadline);
    memset(audio,0,sizeof(audio));mixer_callback(pc,(uint8_t*)audio,count*sizeof(*audio));
    uint64_t guest_now=t386_clock_now_us();
    uint64_t lag=guest_now>deadline?guest_now-deadline:0;
    unsigned late=lag>UINT_MAX?UINT_MAX:(unsigned)lag;
    bool tick=t386_schedule_advance(&schedule,count);
    if(tick) {
        uint64_t video_started=t386_host_time_us();
        if(t386_video_due(&video,video_started,late,rg_display_is_busy())) {
            vga_refresh(pc->vga,redraw,NULL,pc->full_update!=0);
            if(pc->full_update==2)pc->full_update=0;
            present_frame();
            /* Measured video cost is required by the existing frameskip policy. */
            t386_video_record(&video,video_started,t386_host_time_us());
        }
        report_debug_summary(video_started);
    }
    busy+=t386_host_time_us()-start;
    if(tick) {rg_system_tick((int)(busy>INT_MAX?INT_MAX:busy));busy=0;}
    t386_audio_output_prepare(audio,count);rg_audio_submit(audio,count);
}
