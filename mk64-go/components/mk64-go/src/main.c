#ifdef RETRO_GO
#include "port/rg/audio_rg.h"
#endif
#ifndef GCC
#define gRaceState_AS_U16
#endif
#include <ultra64.h>
#include <PR/os.h>
#include <PR/ucode.h>
#include <macros.h>
#include <decode.h>
#include <mk64.h>
#include <course.h>

#include "profiler.h"
#include "main.h"
#include "racing/memory.h"
#include "menus.h"
#include <segments.h>
#include <common_structs.h>
#include <defines.h>
#include "buffers.h"
#include "camera.h"
#include "profiler.h"
#include "race_logic.h"
#include "skybox_and_splitscreen.h"
#include "render_objects.h"
#include "effects.h"
#include "code_80281780.h"
#include "audio/external.h"
#include "code_800029B0.h"
#include "code_80280000.h"
#include "podium_ceremony_actors.h"
#include "menu_items.h"
#include "code_80057C60.h"
#include "profiler.h"
#include "player_controller.h"
#include "render_player.h"
#include "render_courses.h"
#include "actors.h"
#include "replays.h"
#include <debug.h>
#include "crash_screen.h"
#include "buffers/gfx_output_buffer.h"
#include "port/rg/phase_profile.h"
#ifndef TARGET_N64
#include <string.h>
#include "port/port.h"
#include "diagnostic.h"
#ifdef PORT_NET
#include "port/net/port_net.h"
#endif
static u32 sPortTraceFrames = 6;
#define PORT_TRACE(...) do { if (MK64_RG_PROFILE && sPortTraceFrames != 0) PORT_LOG(__VA_ARGS__); } while (0)
#ifndef PORT_ENABLE_AUDIO
#define PORT_ENABLE_AUDIO 1
#endif
#endif

void func_80091B78(void);
void audio_init(void);
void create_debug_thread(void);
void start_debug_thread(void);
struct SPTask* create_next_audio_frame_task(void);

struct VblankHandler* gVblankHandler1 = NULL;
struct VblankHandler* gVblankHandler2 = NULL;

struct SPTask* gActiveSPTask = NULL;
struct SPTask* sCurrentAudioSPTask = NULL;
struct SPTask* sCurrentDisplaySPTask = NULL;
struct SPTask* sNextAudioSPTask = NULL;
struct SPTask* sNextDisplaySPTask = NULL;

struct Controller gControllers[NUM_PLAYERS];
struct Controller* gControllerOne = &gControllers[0];
struct Controller* gControllerTwo = &gControllers[1];
struct Controller* gControllerThree = &gControllers[2];
struct Controller* gControllerFour = &gControllers[3];
struct Controller* gControllerFive = &gControllers[4]; // All physical controllers combined.`
struct Controller* gControllerSix = &gControllers[5];
struct Controller* gControllerSeven = &gControllers[6];
struct Controller* gControllerEight = &gControllers[7];

Player gPlayers[NUM_PLAYERS];
Player* gPlayerOne = &gPlayers[0];
Player* gPlayerTwo = &gPlayers[1];
Player* gPlayerThree = &gPlayers[2];
Player* gPlayerFour = &gPlayers[3];
Player* gPlayerFive = &gPlayers[4];
Player* gPlayerSix = &gPlayers[5];
Player* gPlayerSeven = &gPlayers[6];
Player* gPlayerEight = &gPlayers[7];

Player* gPlayerOneCopy = &gPlayers[0];
Player* gPlayerTwoCopy = &gPlayers[1];
UNUSED Player* gPlayerThreeCopy = &gPlayers[2];
UNUSED Player* gPlayerFourCopy = &gPlayers[3];

UNUSED s32 D_800FD850[3];
#if defined(RETRO_GO)
/* Display lists are CPU-written and consumed by the software renderer. */
__attribute__((section(".ext_ram.bss"), aligned(16)))
#endif
struct GfxPool gGfxPools[2];
struct GfxPool* gGfxPool;

UNUSED s32 gfxPool_padding; // is this necessary?
struct VblankHandler gGameVblankHandler;
struct VblankHandler sSoundVblankHandler;
OSMesgQueue gDmaMesgQueue, gGameVblankQueue, gGfxVblankQueue, unused_gMsgQueue, gIntrMesgQueue, gSPTaskMesgQueue;
OSMesgQueue sSoundMesgQueue;
OSMesg sSoundMesgBuf[1];
OSMesg gDmaMesgBuf[1], gGameMesgBuf;
OSMesg gGfxMesgBuf[1];
UNUSED OSMesg D_8014F010, D_8014F014;
OSMesg gIntrMesgBuf[16], gSPTaskMesgBuf[16];
OSMesg gMainReceivedMesg;
OSIoMesg gDmaIoMesg;
OSMesgQueue gSIEventMesgQueue;
OSMesg gSIEventMesgBuf[3];

OSContStatus gControllerStatuses[4];
OSContPad gControllerPads[4];
u8 gControllerBits;
// Contains a 32x32 grid of indices into gCollisionIndices containing indices into gCollisionMesh
CollisionGrid gCollisionGrid[1024];
u16 gNumActors;
u16 gMatrixObjectCount;
s32 gTickSpeed;
f32 D_80150118;

u16 wasSoftReset;
u16 D_8015011E;

s32 D_80150120;
s32 gGotoMode;
UNUSED s32 D_80150128;
UNUSED s32 D_8015012C;
f32 gCameraZoom[4]; // look like to be the fov of each character
UNUSED s32 D_80150140;
UNUSED s32 D_80150144;
f32 gScreenAspect;
f32 gCourseFarPersp;
f32 gCourseNearPersp;
UNUSED f32 D_80150154;

struct D_80150158 gD_80150158[16];
uintptr_t gSegmentTable[16];
Gfx* gDisplayListHead;

struct SPTask* gGfxSPTask;
s32 D_801502A0;
s32 D_801502A4;
u16* gPhysicalFramebuffers[3];
uintptr_t gPhysicalZBuffer;
UNUSED u32 D_801502B8;
UNUSED u32 D_801502BC;
Mat4 D_801502C0;

s32 padding[2048];

u16 D_80152300[4];
u16 D_80152308;

UNUSED OSThread paddingThread;
OSThread gIdleThread;
ALIGNED8 u8 gIdleThreadStack[STACKSIZE]; // Based on sm64 and padding between bss symbols.
OSThread gVideoThread;
ALIGNED8 u8 gVideoThreadStack[STACKSIZE];
UNUSED OSThread D_80156820;
UNUSED ALIGNED8 u8 D_8015680_Stack[STACKSIZE];
OSThread gGameLoopThread;
ALIGNED8 u8 gGameLoopThreadStack[STACKSIZE];
OSThread gAudioThread;
ALIGNED8 u8 gAudioThreadStack[STACKSIZE];
UNUSED OSThread D_8015CD30;
UNUSED ALIGNED8 u8 D_8015CD30_Stack[STACKSIZE / 2];

ALIGNED8 u8 gGfxSPTaskYieldBuffer[4352];
ALIGNED8 u32 gGfxSPTaskStack[256];
OSMesg gPIMesgBuf[32];
OSMesgQueue gPIMesgQueue;

s32 gGamestate = 0xFFFF;
// gRaceState is externed as an s32 in other files. D_800DC514 is only used in main.c, likely a developer mistake.
u16 gRaceState = RACE_NONE;
u16 D_800DC514 = 0;
u16 creditsRenderMode = 0; // Renders the whole track. Displays red if used in normal race mode.
u16 gDemoMode = DEMO_MODE_INACTIVE;
u16 gEnableDebugMode = ENABLE_DEBUG_MODE;
s32 gGamestateNext = 7; // = COURSE_DATA_MENU?;
UNUSED s32 D_800DC528 = 1;
s32 gActiveScreenMode = SCREEN_MODE_1P;
s32 gScreenModeSelection = SCREEN_MODE_1P;
UNUSED s32 D_800DC534 = 0;
s32 gPlayerCountSelection1 = 2;

s32 gModeSelection = GRAND_PRIX;
s32 D_800DC540 = 0;
s32 D_800DC544 = 0;
s32 gCCSelection = CC_50;
s32 gGlobalTimer = 0;
UNUSED s32 D_800DC550 = 0;
UNUSED s32 D_800DC554 = 0;
UNUSED s32 D_800DC558 = 0;
// Framebuffer rendering values (max 3)
u16 sRenderedFramebuffer = 0;
u16 sRenderingFramebuffer = 0;
UNUSED u16 D_800DC564 = 0;
s32 D_800DC568 = 0;
s32 D_800DC56C[8] = { 0 };
s16 sNumVBlanks = 0;
UNUSED s16 D_800DC590 = 0;
f32 gVBlankTimer = 0.0f;
f32 gCourseTimer = 0.0f;

void create_thread(OSThread* thread, OSId id, void (*entry)(void*), void* arg, void* sp, OSPri pri) {
    thread->next = NULL;
    thread->queue = NULL;
    osCreateThread(thread, id, entry, arg, sp, pri);
}
void isPrintfInit(void);
void main_func(void) {
#ifdef VERSION_EU
    osTvType = TV_TYPE_PAL;
#endif
    osInitialize();
#ifdef DEBUG
    isPrintfInit(); // init osSyncPrintf
#endif
    create_thread(&gIdleThread, 1, &thread1_idle, NULL, gIdleThreadStack + ARRAY_COUNT(gIdleThreadStack), 100);
    osStartThread(&gIdleThread);
}

/**
 * Initialize hardware, start main thread, then idle.
 */
void thread1_idle(void* arg) {
    osCreateViManager(OS_PRIORITY_VIMGR);
#ifdef VERSION_EU
    osViSetMode(&osViModeTable[OS_VI_PAL_LAN1]);
#else // VERSION_US
    if (osTvType == TV_TYPE_NTSC) {
        osViSetMode(&osViModeTable[OS_VI_NTSC_LAN1]);
    } else {
        osViSetMode(&osViModeTable[OS_VI_MPAL_LAN1]);
    }
#endif
    osViBlack(true);
    osViSetSpecialFeatures(OS_VI_GAMMA_OFF);
    osCreatePiManager(OS_PRIORITY_PIMGR, &gPIMesgQueue, gPIMesgBuf, ARRAY_COUNT(gPIMesgBuf));
    wasSoftReset = (s16) osResetType;
    create_debug_thread();
    start_debug_thread();
    create_thread(&gVideoThread, 3, &thread3_video, arg, gVideoThreadStack + ARRAY_COUNT(gVideoThreadStack), 100);
    osStartThread(&gVideoThread);
    osSetThreadPri(NULL, 0);

    // Halt
    while (true) {
        ;
    }
}

void setup_mesg_queues(void) {
    osCreateMesgQueue(&gDmaMesgQueue, gDmaMesgBuf, ARRAY_COUNT(gDmaMesgBuf));
    osCreateMesgQueue(&gSPTaskMesgQueue, gSPTaskMesgBuf, ARRAY_COUNT(gSPTaskMesgBuf));
    osCreateMesgQueue(&gIntrMesgQueue, gIntrMesgBuf, ARRAY_COUNT(gIntrMesgBuf));
    osViSetEvent(&gIntrMesgQueue, (OSMesg) MESG_VI_VBLANK, 1);
    osSetEventMesg(OS_EVENT_SP, &gIntrMesgQueue, (OSMesg) MESG_SP_COMPLETE);
    osSetEventMesg(OS_EVENT_DP, &gIntrMesgQueue, (OSMesg) MESG_DP_COMPLETE);
}

void start_sptask(s32 taskType) {
    if (taskType == M_AUDTASK) {
        gActiveSPTask = sCurrentAudioSPTask;
    } else {
        gActiveSPTask = sCurrentDisplaySPTask;
    }
    osSpTaskLoad(&gActiveSPTask->task);
    osSpTaskStartGo(&gActiveSPTask->task);
    gActiveSPTask->state = SPTASK_STATE_RUNNING;
}

/**
 * Initializes the Fast3D OSTask structure.
 * Loads F3DEX or F3DLX based on the number of players
 **/
void create_gfx_task_structure(void) {
    gGfxSPTask->msgqueue = &gGfxVblankQueue;
    gGfxSPTask->msg = (OSMesg) 2;
    gGfxSPTask->task.t.type = M_GFXTASK;
#ifndef TARGET_N64
    /* Port renderers consume this display list directly; there is no RSP
     * microcode, output buffer, or yield buffer to schedule. */
    gGfxSPTask->task.t.data_ptr = (u64*) gGfxPool->gfxPool;
    gGfxSPTask->task.t.data_size = (gDisplayListHead - gGfxPool->gfxPool) * sizeof(Gfx);
    return;
#endif
    gGfxSPTask->task.t.flags = OS_TASK_DP_WAIT;
    gGfxSPTask->task.t.ucode_boot = rspF3DBootStart;
    gGfxSPTask->task.t.ucode_boot_size = ((u8*) rspF3DBootEnd - (u8*) rspF3DBootStart);
    // The split-screen multiplayer racing state uses F3DLX which has a simple subpixel calculation.
    // Singleplayer race mode and all other game states use F3DEX.
    // http://n64devkit.square7.ch/n64man/ucode/gspF3DEX.htm
    if (gGamestate != RACING || gPlayerCountSelection1 == 1) {
        gGfxSPTask->task.t.ucode = gspF3DEXTextStart;
        gGfxSPTask->task.t.ucode_data = gspF3DEXDataStart;
    } else {
        gGfxSPTask->task.t.ucode = gspF3DLXTextStart;
        gGfxSPTask->task.t.ucode_data = gspF3DLXDataStart;
    }
    gGfxSPTask->task.t.flags = 0;
    gGfxSPTask->task.t.flags = OS_TASK_DP_WAIT;
    gGfxSPTask->task.t.ucode_size = SP_UCODE_SIZE;
    gGfxSPTask->task.t.ucode_data_size = SP_UCODE_DATA_SIZE;
    gGfxSPTask->task.t.dram_stack = (u64*) &gGfxSPTaskStack;
    gGfxSPTask->task.t.dram_stack_size = SP_DRAM_STACK_SIZE8;
    gGfxSPTask->task.t.output_buff = (u64*) &gGfxSPTaskOutputBuffer;
    gGfxSPTask->task.t.output_buff_size = (u64*) ((u8*) gGfxSPTaskOutputBuffer + sizeof(gGfxSPTaskOutputBuffer));
    gGfxSPTask->task.t.data_ptr = (u64*) gGfxPool->gfxPool;
    gGfxSPTask->task.t.data_size = (gDisplayListHead - gGfxPool->gfxPool) * sizeof(Gfx);
    func_8008C214();
    gGfxSPTask->task.t.yield_data_ptr = (u64*) &gGfxSPTaskYieldBuffer;
    gGfxSPTask->task.t.yield_data_size = OS_YIELD_DATA_SIZE;
}

void init_controllers(void) {
    osCreateMesgQueue(&gSIEventMesgQueue, &gSIEventMesgBuf[0], ARRAY_COUNT(gSIEventMesgBuf));
    osSetEventMesg(OS_EVENT_SI, &gSIEventMesgQueue, (OSMesg) 0x33333333);
    osContInit(&gSIEventMesgQueue, &gControllerBits, gControllerStatuses);
    if ((gControllerBits & 1) == 0) {
        sIsController1Unplugged = true;
    } else {
        sIsController1Unplugged = false;
    }
}

void update_controller(s32 index) {
    struct Controller* controller = &gControllers[index];
    u16 stick;

    if (sIsController1Unplugged) {
        return;
    }

    controller->rawStickX = gControllerPads[index].stick_x;
    controller->rawStickY = gControllerPads[index].stick_y;

    if ((gControllerPads[index].button & D_CBUTTONS) != 0) {
        gControllerPads[index].button |= Z_TRIG;
    }
    controller->buttonPressed = gControllerPads[index].button & (gControllerPads[index].button ^ controller->button);
    controller->buttonDepressed = controller->button & (gControllerPads[index].button ^ controller->button);
    controller->button = gControllerPads[index].button;

    stick = 0;
    if (controller->rawStickX < -50) {
        stick |= L_JPAD;
    }
    if (controller->rawStickX > 50) {
        stick |= R_JPAD;
    }
    if (controller->rawStickY < -50) {
        stick |= D_JPAD;
    }
    if (controller->rawStickY > 50) {
        stick |= U_JPAD;
    }
    controller->stickPressed = stick & (stick ^ controller->stickDirection);
    controller->stickDepressed = controller->stickDirection & (stick ^ controller->stickDirection);
    controller->stickDirection = stick;
}

void read_controllers(void) {
    OSMesg msg;

    osContStartReadData(&gSIEventMesgQueue);
    osRecvMesg(&gSIEventMesgQueue, &msg, OS_MESG_BLOCK);
    osContGetReadData(gControllerPads);
    update_controller(0);
    update_controller(1);
    update_controller(2);
    update_controller(3);
    gControllerFive->button = (s16) (((gControllerOne->button | gControllerTwo->button) | gControllerThree->button) |
                                     gControllerFour->button);
    gControllerFive->buttonPressed =
        (s16) (((gControllerOne->buttonPressed | gControllerTwo->buttonPressed) | gControllerThree->buttonPressed) |
               gControllerFour->buttonPressed);
    gControllerFive->buttonDepressed = (s16) (((gControllerOne->buttonDepressed | gControllerTwo->buttonDepressed) |
                                               gControllerThree->buttonDepressed) |
                                              gControllerFour->buttonDepressed);
    gControllerFive->stickDirection =
        (s16) (((gControllerOne->stickDirection | gControllerTwo->stickDirection) | gControllerThree->stickDirection) |
               gControllerFour->stickDirection);
    gControllerFive->stickPressed =
        (s16) (((gControllerOne->stickPressed | gControllerTwo->stickPressed) | gControllerThree->stickPressed) |
               gControllerFour->stickPressed);
    gControllerFive->stickDepressed =
        (s16) (((gControllerOne->stickDepressed | gControllerTwo->stickDepressed) | gControllerThree->stickDepressed) |
               gControllerFour->stickDepressed);
}

void func_80000BEC(void) {
    gPhysicalZBuffer = VIRTUAL_TO_PHYSICAL(&gZBuffer);
}

void dispatch_audio_sptask(struct SPTask* spTask) {
    osWritebackDCacheAll();
    osSendMesg(&gSPTaskMesgQueue, spTask, OS_MESG_NOBLOCK);
}

void exec_display_list(struct SPTask* spTask) {
#ifndef TARGET_N64
    // Display lists are executed synchronously in display_and_vsync().
    spTask->state = SPTASK_STATE_FINISHED;
    return;
#endif
    osWritebackDCacheAll();
    spTask->state = SPTASK_STATE_NOT_STARTED;
    if (sCurrentDisplaySPTask == NULL) {
        sCurrentDisplaySPTask = spTask;
        sNextDisplaySPTask = NULL;
        osSendMesg(&gIntrMesgQueue, (OSMesg) MESG_START_GFX_SPTASK, OS_MESG_NOBLOCK);
    } else {
        sNextDisplaySPTask = spTask;
    }
}

/**
 * Set default RCP (Reality Co-Processor) settings.
 */
void init_rcp(void) {
    move_segment_table_to_dmem();
    init_rdp();
    set_viewport();
    select_framebuffer();
    init_z_buffer();
}

/**
 * End the master display list and initialize the graphics task structure for the next frame to be rendered.
 */
void end_master_display_list(void) {
    gDPFullSync(gDisplayListHead++);
    gSPEndDisplayList(gDisplayListHead++);
    create_gfx_task_structure();
}

// clear_frame_buffer from SM64, with a few edits
//! @todo Why did void* work for matching
void* clear_framebuffer(s32 color) {
    gDPPipeSync(gDisplayListHead++);

    gDPSetRenderMode(gDisplayListHead++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(gDisplayListHead++, G_CYC_FILL);

    gDPSetFillColor(gDisplayListHead++, color);
    gDPFillRectangle(gDisplayListHead++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

    gDPPipeSync(gDisplayListHead++);

    gDPSetCycleType(gDisplayListHead++, G_CYC_1CYCLE);
    return NULL;
}

void rendering_init(void) {
    gGfxPool = &gGfxPools[0];
    set_segment_base_addr(1, gGfxPool);
    gGfxSPTask = &gGfxPool->spTask;
    gDisplayListHead = gGfxPool->gfxPool;
    init_rcp();
    clear_framebuffer(0);
    end_master_display_list();
    exec_display_list(&gGfxPool->spTask);
    sRenderingFramebuffer++;
    gGlobalTimer++;
}

void config_gfx_pool(void) {
    gGfxPool = &gGfxPools[gGlobalTimer & 1];
    set_segment_base_addr(1, gGfxPool);
    gDisplayListHead = gGfxPool->gfxPool;
    gGfxSPTask = &gGfxPool->spTask;
}

/**
 * Send current master display list for rendering.
 * Tell the VI which colour framebuffer to display.
 * Yields to the VI framerate twice, locking the game at 30 FPS.
 * Selects the next framebuffer to be rendered and displayed.
 */
void display_and_vsync(void) {
#ifndef TARGET_N64
    static s32 sTracedStartMenu = 0;
    s32 verbose = 0;
    if (gGamestate == START_MENU_FROM_QUIT && gMenuSelection == START_MENU && sTracedStartMenu == 0) {
        sTracedStartMenu = 1;
        verbose = 1;
        PORT_LOG("first start-menu frame: %d gfx cmds\n", (int) (gDisplayListHead - gGfxPool->gfxPool));
    }
#ifdef PORT_PROFILE
    {
        extern u32 port_time_us(void);
        extern void port_profile_add(int slot, u32 us);
        u32 t0 = port_time_us(), t1, t2, t3;
        port_gfx_start_frame();
        port_gfx_run(gGfxPool->gfxPool);
        t1 = port_time_us();
        port_gfx_end_frame();
        t2 = port_time_us();
        if (gPortHalfFrame != 1) { /* once per game frame, as below */
            port_audio_frame();
        }
        t3 = port_time_us();
        port_profile_add(1, t1 - t0);
        port_profile_add(2, t2 - t1);
        port_profile_add(3, t3 - t2);
    }
#else
    port_gfx_start_frame();
    port_gfx_run(gGfxPool->gfxPool);
    port_gfx_end_frame();
    if (gPortHalfFrame != 1) { /* once per game frame: the mixer is paced by it, a split frame is two pictures */
        port_audio_frame();
    }
    if (verbose) PORT_LOG(" audio done\n");
#endif
    // Two VI retraces per game frame (30 fps).
    gVBlankTimer += 2 * V_BlANK_TIMER_ITER;
    sNumVBlanks += 2;
    if (++sRenderedFramebuffer == 3) {
        sRenderedFramebuffer = 0;
    }
    if (++sRenderingFramebuffer == 3) {
        sRenderingFramebuffer = 0;
    }
    gGlobalTimer++;
    return;
#endif
    profiler_log_thread5_time(BEFORE_DISPLAY_LISTS);
    osRecvMesg(&gGfxVblankQueue, &gMainReceivedMesg, OS_MESG_BLOCK);
    exec_display_list(&gGfxPool->spTask);
    profiler_log_thread5_time(AFTER_DISPLAY_LISTS);
    osRecvMesg(&gGameVblankQueue, &gMainReceivedMesg, OS_MESG_BLOCK);
    osViSwapBuffer((void*) PHYSICAL_TO_VIRTUAL(gPhysicalFramebuffers[sRenderedFramebuffer]));
    profiler_log_thread5_time(THREAD5_END);
    osRecvMesg(&gGameVblankQueue, &gMainReceivedMesg, OS_MESG_BLOCK);
    crash_screen_set_framebuffer(gPhysicalFramebuffers[sRenderedFramebuffer]);

    if (++sRenderedFramebuffer == 3) {
        sRenderedFramebuffer = 0;
    }
    if (++sRenderingFramebuffer == 3) {
        sRenderingFramebuffer = 0;
    }
    gGlobalTimer++;
}

void init_segment_ending_sequences(void) {
#ifndef TARGET_N64
    return; // no code overlays on ported targets
#else
    bzero((void*) SEG_ENDING, SEG_ENDING_SIZE);
    osWritebackDCacheAll();
    dma_copy((u8*) SEG_ENDING, (u8*) SEG_ENDING_ROM_START, SEG_ENDING_ROM_SIZE);
    osInvalICache((void*) SEG_ENDING, SEG_ENDING_SIZE);
    osInvalDCache((void*) SEG_ENDING, SEG_ENDING_SIZE);
#endif
}

void init_segment_racing(void) {
#ifndef TARGET_N64
    return; // no code overlays on ported targets
#else
    bzero((void*) SEG_RACING, SEG_RACING_SIZE);
    osWritebackDCacheAll();
    dma_copy((u8*) SEG_RACING, (u8*) SEG_RACING_ROM_START, SEG_RACING_ROM_SIZE);
    osInvalICache((void*) SEG_RACING, SEG_RACING_SIZE);
    osInvalDCache((void*) SEG_RACING, SEG_RACING_SIZE);
#endif
}

void dma_copy(u8* dest, u8* romAddr, size_t size) {
#ifndef TARGET_N64
    // The cartridge is linked into the executable.
    if (size != 0) {
        memcpy(dest, romAddr, size);
    }
    return;
#endif

    osInvalDCache(dest, size);
    while (size > 0x100) {
        osPiStartDma(&gDmaIoMesg, 0, 0, (uintptr_t) romAddr, dest, 0x100, &gDmaMesgQueue);
        osRecvMesg(&gDmaMesgQueue, &gMainReceivedMesg, 1);
        size -= 0x100;
        romAddr += 0x100;
        dest += 0x100;
    }
    if (size != 0) {
        osPiStartDma(&gDmaIoMesg, 0, 0, (uintptr_t) romAddr, dest, size, &gDmaMesgQueue);
        osRecvMesg(&gDmaMesgQueue, &gMainReceivedMesg, 1);
    }
}

/**
 * Setup main segments and framebuffers.
 */
void setup_game_memory(void) {
    UNUSED u32 pad[2];
    ptrdiff_t commonCourseDataSize; // Compressed mio0 size
    uintptr_t textureSegSize;
    ptrdiff_t textureSegStart;
    uintptr_t allocatedMemory;
    UNUSED s32 unknown_padding;

#ifndef TARGET_N64
    // Everything the N64 DMAs here (trig tables, data_segment2, common
    // textures) is linked in and reached through the port's segment tables.
    gHeapEndPtr = SEG_RACING;
    set_segment_base_addr(0, NULL);
    initialize_memory_pool(MEMORY_POOL_START, MEMORY_POOL_END);
    func_80000BEC();
    set_segment_base_addr(2, NULL);
#ifdef TARGET_PSP
    port_set_segment_table(0xD, &gPortCommonDataSegTable);
#endif
    set_segment_base_addr(0xD, NULL);
    gFreeMemoryResetAnchor = gNextFreeMemoryAddress;
    return;
#endif
    init_segment_racing();
    gHeapEndPtr = SEG_RACING;
    set_segment_base_addr(0, (void*) SEG_START);

    // Memory pool size of 0xAB630
    initialize_memory_pool(MEMORY_POOL_START, MEMORY_POOL_END);

    func_80000BEC();

    // Initialize trig tables segment
    osInvalDCache((void*) TRIG_TABLES, TRIG_TABLES_SIZE);
    osPiStartDma(&gDmaIoMesg, 0, 0, TRIG_TABLES_ROM_START, (void*) TRIG_TABLES, TRIG_TABLES_SIZE, &gDmaMesgQueue);
    osRecvMesg(&gDmaMesgQueue, &gMainReceivedMesg, OS_MESG_BLOCK);

    set_segment_base_addr(2, (void*) load_data(SEG_DATA_START, SEG_DATA_END));

    commonCourseDataSize = COMMON_TEXTURES_SIZE;
    commonCourseDataSize = ALIGN16(commonCourseDataSize);

#ifdef AVOID_UB
    textureSegStart = (ptrdiff_t) SEG_RACING - commonCourseDataSize;
#else
    textureSegStart = SEG_RACING - commonCourseDataSize;
#endif
    osPiStartDma(&gDmaIoMesg, 0, 0, COMMON_TEXTURES_ROM_START, (void*) textureSegStart, commonCourseDataSize,
                 &gDmaMesgQueue);
    osRecvMesg(&gDmaMesgQueue, &gMainReceivedMesg, OS_MESG_BLOCK);

    textureSegSize = *(uintptr_t*) (textureSegStart + 4);
    textureSegSize = ALIGN16(textureSegSize);
    allocatedMemory = gNextFreeMemoryAddress;
    mio0decode((u8*) textureSegStart, (u8*) allocatedMemory);
    set_segment_base_addr(0xD, (void*) allocatedMemory);

    gNextFreeMemoryAddress += textureSegSize;

    // Common course data does not get reloaded when the race state resets.
    // Therefore, only reset the memory ptr to after the common course data.
    gFreeMemoryResetAnchor = gNextFreeMemoryAddress;
}

/**
 * @brief
 *
 */
void game_init_clear_framebuffer(void) {
    gGamestateNext = 0; // = START_MENU_FROM_QUIT?
    clear_framebuffer(0);
}

#ifndef TARGET_N64
/* Where a split-frame picture's time goes outside the display-list interpreter
 * (max_fps_experiments; the ~5.5 ms block the data/exp runs could not look
 * into).  PORT_PROFILE builds add up each segment per half and log the averages
 * with the frame profile. */
enum { SEG_TICK, SEG_KARTLOAD, SEG_OBJECTS, SEG_RENDER3D, SEG_HUD, SEG_MENUITEMS, SEG_PRE, SEG_COUNT };
#ifdef PORT_PROFILE
static u32 sSegSum[2][SEG_COUNT], sSegPics[2], sSegT;
#include <stdio.h>
extern u32 port_time_us(void);
#define PORT_SEG_START() (sSegT = port_time_us())
#define SEG_END(seg) do { u32 now_ = port_time_us(); if (gPortHalfFrame != 0) sSegSum[gPortHalfFrame - 1][seg] += now_ - sSegT; sSegT = now_; } while (0)
void port_seg_report(void) {
    static const char* names[SEG_COUNT] = { "tick", "kart sprites", "objects", "3D display list", "HUD display list", "menu items", "pads+audio cmds" };
    s32 h, k;
    for (h = 0; h < 2; h++) {
        char line[256];
        s32 n = 0;
        if (sSegPics[h] == 0) continue;
        n += snprintf(line + n, sizeof(line) - n, "split half %c (%u pictures), us each:", h ? 'B' : 'A', (unsigned) sSegPics[h]);
        for (k = 0; k < SEG_COUNT; k++) n += snprintf(line + n, sizeof(line) - n, " %s %u,", names[k], (unsigned) (sSegSum[h][k] / sSegPics[h]));
        PORT_LOG("%s\n", line);
        sSegPics[h] = 0;
        for (k = 0; k < SEG_COUNT; k++) sSegSum[h][k] = 0;
    }
}
#else
#define PORT_SEG_START() ((void) 0)
#define SEG_END(seg) ((void) 0)
#endif
#endif

void race_logic_loop(void) {
    s16 i;
    u16 rotY;

    gMatrixObjectCount = 0;
    gMatrixEffectCount = 0;
#ifdef TARGET_PSP
    /* A split frame (port.h, gPortHalfFrame): its second half picks up after
     * tick 1 -- whatever that tick did (pause, quit) takes effect next frame,
     * as it would have. */
    if (gPortHalfFrame == 2) {
        goto port_second_half;
    }
#endif
    if (gIsGamePaused != 0) {
        func_80290B14();
    }
    if (gIsInQuitToMenuTransition != 0) {
        func_802A38B4();
        return;
    }

    if (sNumVBlanks >= 6) {
        sNumVBlanks = 5;
    }
    if (sNumVBlanks < 0) {
        sNumVBlanks = 1;
    }
    func_802A4EF4();
#ifdef TARGET_PSP
port_second_half:
#endif

    switch (gActiveScreenMode) {
        case SCREEN_MODE_1P:
            gTickSpeed = 2;
#ifdef TARGET_PSP
            if (gPortHalfFrame != 0) {
                /* the 1P frame below, one tick per picture */
                sSegPics[gPortHalfFrame - 1]++;
                PORT_SEG_START();
                if (gPortHalfFrame == 1) {
                    replays_loop();
                }
                if (gIsGamePaused == 0 || gPortHalfFrame == 2) {
                    if (D_8015011E) {
                        gCourseTimer += COURSE_TIMER_ITER;
                    }
                    func_802909F0();
                    evaluate_collision_for_players_and_actors();
                    handle_a_press_for_all_players_during_race();
                    func_8001EE98(gPlayerOneCopy, camera1, 0);
                    func_80028F70();
                    func_8028F474();
                    func_80059AC8();
                    update_course_actors();
                    course_update_water();
                    func_8028FCBC();
                    SEG_END(SEG_TICK);
                    /* Kart animation, particles and the sprite-decode queue: once
                     * per game frame, as the game does (after both ticks).  It ran
                     * in both halves for a while -- added while chasing a crash
                     * that was the debug texture flush -- which cost 1.3 ms a
                     * picture on hardware and stepped the kart animation twice a
                     * frame.  The first half draws the sprites decoded for the
                     * last picture: an empty queue tells the renderer not to
                     * decode them again. */
                    if (gPortHalfFrame == 2) {
                        func_80022744();
                    } else {
                        gPlayersToRenderCount = 0;
                    }
                    SEG_END(SEG_KARTLOAD);
                }
                /* Objects and HUD: updated once per frame (second half); the
                 * first half only prepares this picture's render state, as the
                 * paused game does. */
                func_8005A070();
                SEG_END(SEG_OBJECTS);
                if (gPortHalfFrame == 2) {
                    sNumVBlanks = 0;
                }
                profiler_log_thread5_time(LEVEL_SCRIPT_EXECUTE);
                D_8015F788 = 0;
                render_player_one_1p_screen();
                SEG_END(SEG_RENDER3D);
                break;
            }
#endif
            replays_loop();
            if (gIsGamePaused == 0) {
                for (i = 0; i < gTickSpeed; i++) {
                    if (D_8015011E) {
                        gCourseTimer += COURSE_TIMER_ITER;
                    }
                    func_802909F0();
                    evaluate_collision_for_players_and_actors();
                    handle_a_press_for_all_players_during_race();
                    func_8001EE98(gPlayerOneCopy, camera1, 0);
                    func_80028F70();
                    func_8028F474();
                    func_80059AC8();
                    update_course_actors();
                    course_update_water();
                    func_8028FCBC();
                }
                port_rg_profile_mark(RG_PHASE_SIM);
                func_80022744();
            }
            /* Paused frames put their small setup cost in the kart bucket. */
            port_rg_profile_mark(RG_PHASE_KART);
            func_8005A070();
            port_rg_profile_mark(RG_PHASE_OBJECTS);
            sNumVBlanks = 0;
            profiler_log_thread5_time(LEVEL_SCRIPT_EXECUTE);
            D_8015F788 = 0;
            render_player_one_1p_screen();
            port_rg_profile_mark(RG_PHASE_LIST);
            if (!gEnableDebugMode) {
                D_800DC514 = false;
            } else {
                if (D_800DC514) {

                    if ((gControllerOne->buttonPressed & R_TRIG) && (gControllerOne->button & A_BUTTON) &&
                        (gControllerOne->button & B_BUTTON)) {
                        D_800DC514 = false;
                    }

                    rotY = camera1->rot[1];
                    gDebugPathCount = D_800DC5EC->pathCounter;
                    if (rotY < DEGREES(45)) {
                        func_80057A50(40, 100, "SOUTH  ", gDebugPathCount);
                    } else if (rotY < DEGREES(135)) {
                        func_80057A50(40, 100, "EAST   ", gDebugPathCount);
                    } else if (rotY < DEGREES(225)) {
                        func_80057A50(40, 100, "NORTH  ", gDebugPathCount);
                    } else if (rotY < DEGREES(315)) {
                        func_80057A50(40, 100, "WEST   ", gDebugPathCount);
                    } else {
                        func_80057A50(40, 100, "SOUTH  ", gDebugPathCount);
                    }

                } else {
                    if ((gControllerOne->buttonPressed & L_TRIG) && (gControllerOne->button & A_BUTTON) &&
                        (gControllerOne->button & B_BUTTON)) {
                        D_800DC514 = true;
                    }
                }
            }
            break;

        case SCREEN_MODE_2P_SPLITSCREEN_VERTICAL:
            if (gCurrentCourseId == COURSE_DK_JUNGLE) {
                gTickSpeed = 3;
            } else {
                gTickSpeed = 2;
            }
            if (gIsGamePaused == 0) {
                for (i = 0; i < gTickSpeed; i++) {
                    if (D_8015011E != 0) {
                        gCourseTimer += COURSE_TIMER_ITER;
                    }
                    func_802909F0();
                    evaluate_collision_for_players_and_actors();
                    handle_a_press_for_all_players_during_race();
                    func_8001EE98(gPlayerOneCopy, camera1, 0);
                    func_80029060();
                    func_8001EE98(gPlayerTwoCopy, camera2, 1);
                    func_80029150();
                    func_8028F474();
                    func_80059AC8();
                    update_course_actors();
                    course_update_water();
                    func_8028FCBC();
                }
                func_80022744();
            }
            func_8005A070();
            profiler_log_thread5_time(LEVEL_SCRIPT_EXECUTE);
            sNumVBlanks = 0;
            move_segment_table_to_dmem();
            init_rdp();
            if (D_800DC5B0 != 0) {
                select_framebuffer();
            }
            D_8015F788 = 0;
#ifdef PORT_NET
            if (port_net_active()) {
                port_render_local_player(); // ad hoc: this machine's player, full screen
            } else
#endif
            if (gPlayerWinningIndex == 0) {
                render_player_two_2p_screen_vertical();
                render_player_one_2p_screen_vertical();
            } else {
                render_player_one_2p_screen_vertical();
                render_player_two_2p_screen_vertical();
            }
            break;

        case SCREEN_MODE_2P_SPLITSCREEN_HORIZONTAL:

            if (gCurrentCourseId == COURSE_DK_JUNGLE) {
                gTickSpeed = 3;
            } else {
                gTickSpeed = 2;
            }

            if (gIsGamePaused == 0) {
                for (i = 0; i < gTickSpeed; i++) {
                    if (D_8015011E != 0) {
                        gCourseTimer += COURSE_TIMER_ITER;
                    }
                    func_802909F0();
                    evaluate_collision_for_players_and_actors();
                    handle_a_press_for_all_players_during_race();
                    func_8001EE98(gPlayerOneCopy, camera1, 0);
                    func_80029060();
                    func_8001EE98(gPlayerTwoCopy, camera2, 1);
                    func_80029150();
                    func_8028F474();
                    func_80059AC8();
                    update_course_actors();
                    course_update_water();
                    func_8028FCBC();
                }
                func_80022744();
            }
            profiler_log_thread5_time(LEVEL_SCRIPT_EXECUTE);
            sNumVBlanks = (u16) 0;
            func_8005A070();
            move_segment_table_to_dmem();
            init_rdp();
            if (D_800DC5B0 != 0) {
                select_framebuffer();
            }
            D_8015F788 = 0;
#ifdef PORT_NET
            if (port_net_active()) {
                port_render_local_player(); // ad hoc: this machine's player, full screen
            } else
#endif
            if (gPlayerWinningIndex == 0) {
                render_player_two_2p_screen_horizontal();
                render_player_one_2p_screen_horizontal();
            } else {
                render_player_one_2p_screen_horizontal();
                render_player_two_2p_screen_horizontal();
            }

            break;

        case SCREEN_MODE_3P_4P_SPLITSCREEN:
            if (gPlayerCountSelection1 == 3) {
                switch (gCurrentCourseId) {
                    case COURSE_BOWSER_CASTLE:
                    case COURSE_MOO_MOO_FARM:
                    case COURSE_SKYSCRAPER:
                    case COURSE_DK_JUNGLE:
                        gTickSpeed = 3;
                        break;
                    default:
                        gTickSpeed = 2;
                        break;
                }
            } else {
                // Four players
                switch (gCurrentCourseId) {
                    case COURSE_BLOCK_FORT:
                    case COURSE_DOUBLE_DECK:
                    case COURSE_BIG_DONUT:
                        gTickSpeed = 2;
                        break;
                    case COURSE_DK_JUNGLE:
                        gTickSpeed = 4;
                        break;
                    default:
                        gTickSpeed = 3;
                        break;
                }
            }
            if (gIsGamePaused == 0) {
                for (i = 0; i < gTickSpeed; i++) {
                    if (D_8015011E != 0) {
                        gCourseTimer += COURSE_TIMER_ITER;
                    }
                    func_802909F0();
                    evaluate_collision_for_players_and_actors();
                    handle_a_press_for_all_players_during_race();
                    func_8001EE98(gPlayerOneCopy, camera1, 0);
                    func_80029158();
                    func_8001EE98(gPlayerTwo, camera2, 1);
                    func_800291E8();
                    func_8001EE98(gPlayerThree, camera3, 2);
                    func_800291F0();
                    func_8001EE98(gPlayerFour, camera4, 3);
                    func_800291F8();
                    func_8028F474();
                    func_80059AC8();
                    update_course_actors();
                    course_update_water();
                    func_8028FCBC();
                }
                func_80022744();
            }
            func_8005A070();
            sNumVBlanks = 0;
            profiler_log_thread5_time(LEVEL_SCRIPT_EXECUTE);
            move_segment_table_to_dmem();
            init_rdp();
            if (D_800DC5B0 != 0) {
                select_framebuffer();
            }
            D_8015F788 = 0;
#ifdef PORT_NET
            if (port_net_active()) {
                port_render_local_player(); // ad hoc: this machine's player, full screen
            } else
#endif
            if (gPlayerWinningIndex == 0) {
                render_player_two_3p_4p_screen();
                render_player_three_3p_4p_screen();
                render_player_four_3p_4p_screen();
                render_player_one_3p_4p_screen();
            } else if (gPlayerWinningIndex == 1) {
                render_player_one_3p_4p_screen();
                render_player_three_3p_4p_screen();
                render_player_four_3p_4p_screen();
                render_player_two_3p_4p_screen();
            } else if (gPlayerWinningIndex == 2) {
                render_player_one_3p_4p_screen();
                render_player_two_3p_4p_screen();
                render_player_four_3p_4p_screen();
                render_player_three_3p_4p_screen();
            } else {
                render_player_one_3p_4p_screen();
                render_player_two_3p_4p_screen();
                render_player_three_3p_4p_screen();
                render_player_four_3p_4p_screen();
            }
            break;
    }

    if (!gEnableDebugMode) {
        gEnableResourceMeters = 0;
    } else {
        if (gEnableResourceMeters) {
            resource_display();
            if ((!(gControllerOne->button & L_TRIG)) && (gControllerOne->button & R_TRIG) &&
                (gControllerOne->buttonPressed & B_BUTTON)) {
                gEnableResourceMeters = 0;
            }
        } else {
            if ((!(gControllerOne->button & L_TRIG)) && (gControllerOne->button & R_TRIG) &&
                (gControllerOne->buttonPressed & B_BUTTON)) {
                gEnableResourceMeters = 1;
            }
        }
    }
#ifdef TARGET_PSP
    PORT_SEG_START();
#endif
    func_802A4300();
    func_800591B4();
#ifdef TARGET_PSP
    SEG_END(SEG_HUD);
#endif
    func_80093E20();
#ifdef TARGET_PSP
    SEG_END(SEG_MENUITEMS);
#endif
#if DVDL
    display_dvdl();
#endif
    gDPFullSync(gDisplayListHead++);
    gSPEndDisplayList(gDisplayListHead++);
}

/**
 * mk64's game loop depends on a series of states.
 * It runs a wide branching series of code based on these states.
 * State 1) Clear framebuffer
 * State 2) Run menus
 * State 3) Process race related logic
 * State 4) Ending sequence
 * State 5) Credits
 *
 * Note that the state doesn't flip-flop at random but is permanent
 * until the state changes (ie. Exit menus and start a race).
 */

void game_state_handler(void) {
#if DVDL
    if ((gControllerOne->button & L_TRIG) && (gControllerOne->button & R_TRIG) && (gControllerOne->button & Z_TRIG) &&
        (gControllerOne->button & A_BUTTON)) {
        gGamestateNext = CREDITS_SEQUENCE;
    } else if ((gControllerOne->button & L_TRIG) && (gControllerOne->button & R_TRIG) &&
               (gControllerOne->button & Z_TRIG) && (gControllerOne->button & B_BUTTON)) {
        gGamestateNext = ENDING;
    }
#endif

    switch (gGamestate) {
        case 7:
            game_init_clear_framebuffer();
            break;
        case START_MENU_FROM_QUIT:
        case MAIN_MENU_FROM_QUIT:
        case PLAYER_SELECT_MENU_FROM_QUIT:
        case COURSE_SELECT_MENU_FROM_QUIT:
            // Display black
            osViBlack(0);
#ifdef PORT_NET
            if (port_net_lobby_active() || port_net_modal_active()) {
                // the lobby / drop-out prompt over the frozen menu (docs/adhoc.md)
                if (port_net_lobby_active()) port_net_lobby_update();
                init_rcp();
                func_80094A64(gGfxPool);
                if (port_net_lobby_active()) port_net_lobby_draw();
                else port_net_modal_draw();
                break;
            }
#endif
            PORT_TRACE("  update_menus\n");
            update_menus();
            PORT_TRACE("  init_rcp\n");
            init_rcp();
            PORT_TRACE("  func_80094A64\n");
            func_80094A64(gGfxPool);
            PORT_TRACE("  menus done\n");
#if DVDL
            display_dvdl();
#endif
            break;
        case RACING:
            PORT_TRACE("  race_logic_loop\n");
            race_logic_loop();
            PORT_TRACE("  race_logic_loop done\n");
            break;
        case ENDING:
            podium_ceremony_loop();
            break;
        case CREDITS_SEQUENCE:
            credits_loop();
            break;
    }
}

void interrupt_gfx_sptask(void) {
    if (gActiveSPTask->task.t.type == M_GFXTASK) {
        gActiveSPTask->state = SPTASK_STATE_INTERRUPTED;
        osSpTaskYield();
    }
}

void receive_new_tasks(void) {
    UNUSED s32 pad;
    struct SPTask* spTask;

    while (osRecvMesg(&gSPTaskMesgQueue, (OSMesg*) &spTask, OS_MESG_NOBLOCK) != -1) {
        spTask->state = SPTASK_STATE_NOT_STARTED;
        switch (spTask->task.t.type) {
            case 2:
                sNextAudioSPTask = spTask;
                break;
            case 1:
                sNextDisplaySPTask = spTask;
                break;
        }
    }

    if (sCurrentAudioSPTask == NULL && sNextAudioSPTask != NULL) {
        sCurrentAudioSPTask = sNextAudioSPTask;
        sNextAudioSPTask = NULL;
    }
    if (sCurrentDisplaySPTask == NULL && sNextDisplaySPTask != NULL) {
        sCurrentDisplaySPTask = sNextDisplaySPTask;
        sNextDisplaySPTask = NULL;
    }
}

void set_vblank_handler(s32 index, struct VblankHandler* handler, OSMesgQueue* queue, OSMesg* msg) {
    handler->queue = queue;
    handler->msg = msg;
    switch (index) {
        case 1:
            gVblankHandler1 = handler;
            break;
        case 2:
            gVblankHandler2 = handler;
            break;
    }
}

void start_gfx_sptask(void) {
    if (gActiveSPTask == NULL && sCurrentDisplaySPTask != NULL &&
        sCurrentDisplaySPTask->state == SPTASK_STATE_NOT_STARTED) {
        profiler_log_gfx_time(TASKS_QUEUED);
        start_sptask(M_GFXTASK);
    }
}

void handle_vblank(void) {
    gVBlankTimer += V_BlANK_TIMER_ITER;
    sNumVBlanks++;

    receive_new_tasks();

    // First try to kick off an audio task. If the gfx task is currently
    // running, we need to asynchronously interrupt it -- handle_sp_complete
    // will pick up on what we're doing and start the audio task for us.
    // If there is already an audio task running, there is nothing to do.
    // If there is no audio task available, try a gfx task instead.
    if (sCurrentAudioSPTask != NULL) {
        if (gActiveSPTask != NULL) {
            interrupt_gfx_sptask();
        } else {
            profiler_log_vblank_time();
            start_sptask(M_AUDTASK);
        }
    } else {
        if (gActiveSPTask == NULL && sCurrentDisplaySPTask != NULL &&
            sCurrentDisplaySPTask->state != SPTASK_STATE_FINISHED) {
            profiler_log_gfx_time(TASKS_QUEUED);
            start_sptask(M_GFXTASK);
        }
    }

/* This is where I would put my rumble code... If I had any. */
#if ENABLE_RUMBLE
    rumble_thread_update_vi();
#endif

    if (gVblankHandler1 != NULL) {
        osSendMesg(gVblankHandler1->queue, gVblankHandler1->msg, OS_MESG_NOBLOCK);
    }
    if (gVblankHandler2 != NULL) {
        osSendMesg(gVblankHandler2->queue, gVblankHandler2->msg, OS_MESG_NOBLOCK);
    }
}

void handle_dp_complete(void) {
    // Gfx SP task is completely done.
    if (sCurrentDisplaySPTask->msgqueue != NULL) {
        osSendMesg(sCurrentDisplaySPTask->msgqueue, sCurrentDisplaySPTask->msg, OS_MESG_NOBLOCK);
    }
    profiler_log_gfx_time(RDP_COMPLETE);
    sCurrentDisplaySPTask->state = SPTASK_STATE_FINISHED_DP;
    sCurrentDisplaySPTask = NULL;
}

void handle_sp_complete(void) {
    struct SPTask* curSPTask = gActiveSPTask;

    gActiveSPTask = NULL;

    if (curSPTask->state == SPTASK_STATE_INTERRUPTED) {
        // handle_vblank tried to start an audio task while there was already a
        // gfx task running, so it had to interrupt the gfx task. That interruption
        // just finished.
        if (osSpTaskYielded((OSTask*) curSPTask) == 0) {
            // The gfx task completed before we had time to interrupt it.
            // Mark it finished, just like below.
            curSPTask->state = SPTASK_STATE_FINISHED;
            profiler_log_gfx_time(RSP_COMPLETE);
        }
        // Start the audio task, as expected by handle_vblank.
        profiler_log_vblank_time();
        start_sptask(M_AUDTASK);
    } else {
        curSPTask->state = SPTASK_STATE_FINISHED;
        if (curSPTask->task.t.type == M_AUDTASK) {
            // After audio tasks come gfx tasks.
            profiler_log_vblank_time();
            if (sCurrentDisplaySPTask != NULL) {
                if (sCurrentDisplaySPTask->state != SPTASK_STATE_FINISHED) {
                    if (sCurrentDisplaySPTask->state != SPTASK_STATE_INTERRUPTED) {
                        profiler_log_gfx_time(TASKS_QUEUED);
                    }
                    start_sptask(M_GFXTASK);
                }
            }
            sCurrentAudioSPTask = NULL;
            if (curSPTask->msgqueue != NULL) {
                osSendMesg(curSPTask->msgqueue, curSPTask->msg, OS_MESG_NOBLOCK);
            }
        } else {
            // The SP process is done, but there is still a Display Processor notification
            // that needs to arrive before we can consider the task completely finished and
            // null out sCurrentDisplaySPTask. That happens in handle_dp_complete.
            profiler_log_gfx_time(RSP_COMPLETE);
        }
    };
}

void thread3_video(UNUSED void* arg0) {
    s32 i;
    OSMesg msg;
    UNUSED s32 pad[4];

    gPhysicalFramebuffers[0] = (u16*) &gFramebuffer0;
    gPhysicalFramebuffers[1] = (u16*) &gFramebuffer1;
    gPhysicalFramebuffers[2] = (u16*) &gFramebuffer2;

    // Clear framebuffer.
    for (i = 0; i < PORT_N64_FB_TEXELS; i++) {
        gFramebuffer1[i] = 0;
    }
    setup_mesg_queues();
    setup_game_memory();

    create_thread(&gAudioThread, 4, &thread4_audio, 0, gAudioThreadStack + ARRAY_COUNT(gAudioThreadStack), 20);
    osStartThread(&gAudioThread);

    create_thread(&gGameLoopThread, 5, &thread5_game_loop, 0, gGameLoopThreadStack + ARRAY_COUNT(gGameLoopThreadStack),
                  10);
    osStartThread(&gGameLoopThread);

    while (true) {
        osRecvMesg(&gIntrMesgQueue, &msg, OS_MESG_BLOCK);
        switch ((u32) msg) {
            case MESG_VI_VBLANK:
                handle_vblank();
                break;
            case MESG_SP_COMPLETE:
                handle_sp_complete();
                break;
            case MESG_DP_COMPLETE:
                handle_dp_complete();
                break;
            case MESG_START_GFX_SPTASK:
                start_gfx_sptask();
                break;
        }
    }
}

void func_800025D4(void) {
    func_80091B78();
    gActiveScreenMode = SCREEN_MODE_1P;
    set_perspective_and_aspect_ratio();
}

void func_80002600(void) {
    func_80091B78();
    gActiveScreenMode = SCREEN_MODE_1P;
    set_perspective_and_aspect_ratio();
}

void func_8000262C(void) {
    func_80091B78();
    gActiveScreenMode = SCREEN_MODE_1P;
    set_perspective_and_aspect_ratio();
}

void func_80002658(void) {
    PORT_TRACE(" func_80091B78\n");
    func_80091B78();
    gActiveScreenMode = SCREEN_MODE_1P;
    PORT_TRACE(" set_perspective_and_aspect_ratio\n");
    set_perspective_and_aspect_ratio();
    PORT_TRACE(" func_80002658 done\n");
}

/**
 * Sets courseId to NULL if
 *
 *
 */
void update_gamestate(void) {
    switch (gGamestate) {
        case START_MENU_FROM_QUIT:
            func_80002658();
            gCurrentlyLoadedCourseId = COURSE_NULL;
            break;
        case MAIN_MENU_FROM_QUIT:
            func_800025D4();
            gCurrentlyLoadedCourseId = COURSE_NULL;
            break;
        case PLAYER_SELECT_MENU_FROM_QUIT:
            func_80002600();
            gCurrentlyLoadedCourseId = COURSE_NULL;
            break;
        case COURSE_SELECT_MENU_FROM_QUIT:
            func_8000262C();
            gCurrentlyLoadedCourseId = COURSE_NULL;
            break;
        case RACING:
            /**
             * @bug Reloading this segment makes random_u16() deterministic for player spawn order.
             * In laymens terms, random_u16() outputs the same value every time.
             */
            init_segment_racing();
            PORT_LOG("setup_race (course %d)\n", gCurrentCourseId);
            setup_race();
            PORT_LOG("setup_race done\n");
            sPortTraceFrames = 3;
            {
                extern int gfx_trace_frames;
                gfx_trace_frames = 0; /* was 1: debug DL trace, disabled */
            }
            break;
        case ENDING:
            gCurrentlyLoadedCourseId = COURSE_NULL;
            init_segment_ending_sequences();
            load_ceremony_cutscene();
            break;
        case CREDITS_SEQUENCE:
            gCurrentlyLoadedCourseId = COURSE_NULL;
            init_segment_racing();
            init_segment_ending_sequences();
            load_credits();
            break;
    }
}

#ifndef TARGET_N64
/**
 * Ported targets run the game on one thread: port_game_init() does what
 * thread3_video/thread4_audio/thread5_game_loop set up, and
 * port_game_loop_one_iteration() is one trip round thread5's loop.
 */
void port_game_init(void) {
    gPhysicalFramebuffers[0] = (u16*) &gFramebuffer0;
    gPhysicalFramebuffers[1] = (u16*) &gFramebuffer1;
    gPhysicalFramebuffers[2] = (u16*) &gFramebuffer2;
    setup_mesg_queues();
    setup_game_memory();
#if PORT_ENABLE_AUDIO
#ifdef RETRO_GO
    mk64_rg_audio_ensure_init();
#else
    PORT_LOG("audio_init\n");
    audio_init();
#endif
#endif

    osCreateMesgQueue(&gGfxVblankQueue, gGfxMesgBuf, 1);
    osCreateMesgQueue(&gGameVblankQueue, &gGameMesgBuf, 1);
    init_controllers();
    if (!wasSoftReset) {
        clear_nmi_buffer();
    }
    set_vblank_handler(2, &gGameVblankHandler, &gGameVblankQueue, (OSMesg) OS_EVENT_SW2);
    nmi_gVersusResults2P = &pAppNmiBuffer[0];
    nmi_gVersusResults3P = &pAppNmiBuffer[2];
    nmi_gVersusResults4P = &pAppNmiBuffer[11];
    gNmiUnknown4 = &pAppNmiBuffer[23];
    gNmiUnknown5 = &pAppNmiBuffer[25];
    gNmiUnknown6 = &pAppNmiBuffer[28];
    rendering_init();
    read_controllers();
#if PORT_ENABLE_AUDIO && !defined(RETRO_GO)
    func_800C5CB8();
#endif
}

/* The N64 audio thread builds one audio frame per VI retrace; the game runs
 * at 30 fps, so two per game frame.  synthesis_execute() runs the RSP
 * command list through the C mixer straight into the AI buffers. */
void port_audio_frame(void) {
#if PORT_ENABLE_AUDIO
#ifdef RETRO_GO
    if (!mk64_rg_audio_enabled()) return;
    mk64_rg_audio_ensure_init();
    extern unsigned int port_time_us(void);
    unsigned audio_started = port_time_us();
    mk64_rg_audio_record_queue(); /* Sample before replenishing after loading. */
    /* Sequences also drive demo cameras. Advance exactly two VI ticks per
     * simulation frame, never extra ticks to fill a wall-clock audio FIFO. */
    create_next_audio_frame_task();
    create_next_audio_frame_task();
    mk64_rg_audio_record_frame(port_time_us() - audio_started);
#else
    unsigned target_bytes = 2600 * 4;
    // Keep ~50 ms queued at the output regardless of the game frame rate (the
    // N64 audio thread ran per VI retrace, not per game frame): mix until the
    // FIFO holds enough, at most 3 audio frames per game frame.
    // Two VI ticks per 30Hz game frame. Presets can skip generation ticks.
    extern u32 port_audio_out_queued_bytes(void);
    s32 i;
    for (i = 0; i < 3; i++) {
        if (i > 0 && port_audio_out_queued_bytes() >= target_bytes) {
            break;
        }
        create_next_audio_frame_task();
        create_next_audio_frame_task();
    }
#endif
#endif
}

int gPortTraceArm; /* debug: set to re-arm the per-phase trace for a few frames */
#include <stdio.h>
s32 gPortHalfFrame = 0;
static u32 sStallT[4];
extern u32 port_time_us(void);
s32 gPortVblanksPerFrame = 2;
s32 gPortLastFrameVblanks = 2;
u32 gPortLastFrameBusyUs = 0;
static s32 sPortSplitHoldoff; /* iterations left at 30 fps after 60 could not be held */
static s32 sExpRunning;        /* PORT_EXP builds: the data/exp measurement is rotating its experiments */
UNUSED static s32 sNoDirect = -1;
extern int gPortNoDirectEmit;  /* gfx_pc.c */

/* A frame may be split when it is a plain 1P race frame: the 2P-4P loops and
 * the lockstep (one network frame per iteration) keep whole frames. */
static s32 port_frame_can_split(void) {
#ifndef TARGET_PSP
    /* Only the PSP race handler and backend implement half-frame simulation
     * and publish the timings used by its governor. RG renders whole frames. */
    return 0;
#else
#ifdef PORT_DEBUG_KNOBS
    static s32 sOff = -1;
    if (sOff < 0) { /* test knob: a data/fps30 file keeps whole frames, for A/B runs */
        FILE* f = fopen(port_save_path("fps30"), "rb");
        sOff = f != NULL;
        if (f != NULL) fclose(f);
        PORT_LOG("fps: split frames (60 fps in 1P races) %s\n", sOff ? "off (data/fps30)" : "on");
    }
    if (sOff) {
        return 0;
    }
#endif
    if (gGamestate != RACING || gActiveScreenMode != SCREEN_MODE_1P || gIsGamePaused != 0 ||
        gIsInQuitToMenuTransition != 0 || sPortSplitHoldoff > 0) {
        return 0;
    }
#ifdef PORT_NET
    if (port_net_active()) {
        return 0;
    }
#endif
    return 1;
#endif
}

/* Halves that miss their vblank run the game slow: past a quarter of them in
 * two seconds, fall back to whole frames for a while, then try again. */
static void port_split_stats(void) {
    static u32 sHalves, sMissed, sBusySum, sBusyMax, sLogHalves;
    if (sPortSplitHoldoff > 0) {
        /* Back at 30 fps because 60 was not held.  Ten seconds was arbitrary
         * and usually far longer than the heavy stretch: a whole frame is one
         * 60 fps picture plus one more tick (~2 ms), so its busy time says
         * whether 60 would fit again.  After at least a second, return as soon
         * as half a second of frames averages under that; the ten seconds stay
         * as the upper limit. */
        static u32 sWholeN, sWholeBusy;
        sPortSplitHoldoff--;
        if (gPortHalfFrame == 0 && gGamestate == RACING && gIsGamePaused == 0) {
            sWholeN++;
            sWholeBusy += gPortLastFrameBusyUs;
            if (sWholeN == 15) {
                u32 avg = sWholeBusy / 15;
                if (sPortSplitHoldoff <= 300 - 30 && avg < 17500) {
                    PORT_LOG("fps: back to 60 after %d frames at 30 (whole frames now %u us)\n", (int) (300 - sPortSplitHoldoff), (unsigned) avg);
                    sPortSplitHoldoff = 0;
                }
                sWholeN = sWholeBusy = 0;
            }
        } else {
            sWholeN = sWholeBusy = 0;
        }
    }
    if (gPortHalfFrame == 0) {
        return;
    }
    sHalves++; sLogHalves++;
    sBusySum += gPortLastFrameBusyUs;
    if (gPortLastFrameBusyUs > sBusyMax) sBusyMax = gPortLastFrameBusyUs;
    if (gPortLastFrameVblanks > 1) sMissed++;
    /* The governor, once the race itself runs (the flyover and the countdown
     * are always late and say nothing about the race): every second, pull the
     * draw distance in a little only if pictures really were late, never below
     * GOV_FLOOR, and let it straight back out after a clean second.  (Its first
     * version also reacted to the average busy time and sat at 1200 of 3000 for
     * whole races with no late picture at all: far too visible.) */
#define GOV_FLOOR ((float) PORT_DRAW_DIST) /* never nearer than the 30 fps build draws: on DK's Jungle Parkway a floor of
                                            * 2600 sat right on the far backdrop, which popped in and out as this breathed */
#define GOV_CEIL 5000.0f
    {
        static u32 sGovN, sGovLate, sGovBusy, sGovHold;
        if (gRaceState < RACE_IN_PROGRESS || sExpRunning) {
            gPortDrawDist = (float) PORT_DRAW_DIST;
            sGovN = sGovLate = sGovBusy = sGovHold = 0;
        } else {
            sGovN++;
            sGovBusy += gPortLastFrameBusyUs;
            if (gPortLastFrameVblanks > 1) sGovLate++;
            if (sGovN == 60) {
                u32 avg = sGovBusy / 60;
                if (sGovHold > 0) sGovHold--;
                if (gPortDrawDist > GOV_FLOOR && (sGovLate >= 2 || avg > 15300)) {
                    gPortDrawDist *= 0.93f;
                    sGovHold = 15; /* no growing again for 15 s: the edge must not move in and out */
                } else if (sGovLate == 0 && avg < 13800 && sGovHold == 0) {
                    gPortDrawDist *= 1.04f;
                }
                if (gPortDrawDist < GOV_FLOOR) gPortDrawDist = GOV_FLOOR;
                if (gPortDrawDist > GOV_CEIL) gPortDrawDist = GOV_CEIL;
                sGovN = sGovLate = sGovBusy = 0;
            }
        }
    }
#ifdef PORT_DEBUG_KNOBS
    {
        /* data/nodirect: batches go through the staging copy again (the switch
         * to pull if direct vertex emit ever misbehaves on some hardware) */
        if (sNoDirect < 0) {
            FILE* nf = fopen(port_save_path("nodirect"), "rb");
            sNoDirect = nf != NULL;
            if (nf != NULL) fclose(nf);
            if (sNoDirect) PORT_LOG("gfx: direct vertex emit off (data/nodirect)\n");
            gPortNoDirectEmit = sNoDirect;
        }
    }
#endif
#ifdef PORT_EXP
    /* Hardware cost breakdown (-DPORT_EXP builds): with an empty data/exp file, a race rotates
     * through eight renderer experiments ONE SECOND at a time and adds each
     * one's busy time up over the whole run, logging the averages every 24
     * seconds.  (Five-second windows compared different stretches of track:
     * the scene changed more than the stages cost.)  The first 12 pictures
     * after a switch are left out: caches refill.  What is left when a stage
     * is switched off is what the others cost.  The picture is wrong while it
     * runs. */
    {
        extern s32 gPortExpMode;
        static s32 sExpOn = -1;
        static u32 sExpPic, sExpTotal, sExpSum[8], sExpN[8], sExpLate[8];
        if (sExpOn < 0) {
            FILE* f = fopen(port_save_path("exp"), "rb");
            sExpOn = f != NULL;
            if (f != NULL) fclose(f);
        }
        sExpRunning = sExpOn && gRaceState >= RACE_IN_PROGRESS;
        if (sExpOn && gRaceState >= RACE_IN_PROGRESS) {
            if (sExpPic >= 12) {
                sExpSum[gPortExpMode] += gPortLastFrameBusyUs;
                sExpN[gPortExpMode]++;
                if (gPortLastFrameVblanks > 1) sExpLate[gPortExpMode]++;
            }
            if (++sExpPic == 60) {
                sExpPic = 0;
                gPortExpMode = (gPortExpMode + 1) % 8;
            }
            sPortSplitHoldoff = 0; /* the measurement needs 60 fps pictures: no 30 fps fallback while it runs */
            if (++sExpTotal == 1440) { /* every 24 s of pictures: three turns of the eight modes */
                static const char* names[] = { "everything", "no triangles (vertices only)", "no vertices, no triangles", "display list not run", "triangles culled and clipped but not drawn", "no texture imports (and so no texture changes)", "everything, batches through the staging copy", "everything, vertex flags from C compares" };
                s32 m;
                {
                    extern u32 gPortExpCount[10];
                    u32 p = sExpTotal / 8, *c = gPortExpCount; /* counted in mode 0 only: a seventh of the report's pictures */
                    PORT_LOG("exp counts per normal picture: tris in %u, rejected %u, culled %u, clipper %u, emitted %u | rebuilds %u, imports %u, uploads %u, flushes %u | vertices %u\n",
                             (unsigned) (c[0] / p), (unsigned) (c[1] / p), (unsigned) (c[2] / p), (unsigned) (c[3] / p), (unsigned) (c[4] / p),
                             (unsigned) (c[5] / p), (unsigned) (c[6] / p), (unsigned) (c[7] / p), (unsigned) (c[8] / p), (unsigned) (c[9] / p));
                    for (m = 0; m < 10; m++) c[m] = 0;
                }
                for (m = 0; m < 8; m++) {
                    PORT_LOG("exp %d (%s): busy %u us avg per picture over %u pictures, %u late\n", (int) m, names[m],
                             (unsigned) (sExpN[m] ? sExpSum[m] / sExpN[m] : 0), (unsigned) sExpN[m], (unsigned) sExpLate[m]);
                    sExpSum[m] = sExpN[m] = sExpLate[m] = 0;
                }
                sExpTotal = 0;
            }
        } else {
            gPortExpMode = 0;
        }
        gPortNoDirectEmit = sNoDirect || gPortExpMode == 6;
    }
#endif
    if (sHalves == 120) {
        if (sMissed > 14 && !sExpRunning) { /* over an eighth late: the game would run visibly slow */
            sPortSplitHoldoff = 300;
            PORT_LOG("fps: 60 not held (%u of 120 pictures late): 30 fps until the frames are light enough again (10 s at most)\n", (unsigned) sMissed);
        }
        if (sLogHalves >= 600 || sMissed > 14) {
            PORT_LOG("fps: split frames: busy %u us avg, %u max per picture (16667 = 60 fps), %u of 120 late, draw distance %d\n",
                     (unsigned) (sBusySum / sHalves), (unsigned) sBusyMax, (unsigned) sMissed, (int) gPortDrawDist);
            sLogHalves = 0;
        }
        sHalves = sMissed = sBusySum = sBusyMax = 0;
    }
}

void port_game_loop_one_iteration(void) {
    if (gPortTraceArm) { sPortTraceFrames = gPortTraceArm; gPortTraceArm = 0; }
    { /* A periodic heartbeat, and every phase change as it happens. */
        static int n, lastState = -1, lastMenu = -1, lastCourse = -1, lastRace = -1;
        ++n;
        if (gGamestate != lastState || gMenuSelection != lastMenu || gCurrentCourseId != lastCourse || gRaceState != lastRace) {
            lastState = gGamestate; lastMenu = gMenuSelection; lastCourse = gCurrentCourseId; lastRace = gRaceState;
            PORT_LOG("game: iteration %d: state %d menu %d course %d race %d players %d\n", n, gGamestate, gMenuSelection, gCurrentCourseId, gRaceState, gPlayerCount);
        } else if ((n % MK64_RG_REPORT_INTERVAL) == 0) {
            PORT_LOG("game: iteration %d (state %d menu %d)\n", n, gGamestate, gMenuSelection);
        }
    }
    PORT_TRACE("iteration start (timer %d)\n", gGlobalTimer);
    /* Stall detector: a race iteration that takes over 300 ms logs where the
     * time went (five clock reads an iteration).  A 2-3 s freeze was seen on
     * hardware once; this names the phase if it comes back. */
    sStallT[0] = port_time_us();
    port_split_stats();
    sStallT[1] = port_time_us();
    PORT_SEG_START();
    if (gPortHalfFrame == 1) {
        /* The second half of a split frame: the frame's audio commands, state
         * change and pad read were its first half's. */
        gPortHalfFrame = 2;
        config_gfx_pool();
    } else {
#if PORT_ENABLE_AUDIO
    func_800CB2C4();
    PORT_TRACE(" func_800CB2C4 done\n");
#endif
    if (gGamestateNext != gGamestate) {
        PORT_TRACE("gamestate %d -> %d\n", gGamestate, gGamestateNext);
        gGamestate = gGamestateNext;
        update_gamestate();
        PORT_TRACE(" update_gamestate done\n");
    }
    config_gfx_pool();
    read_controllers();
    gPortHalfFrame = port_frame_can_split() ? 1 : 0;
    }
    gPortVblanksPerFrame = gPortHalfFrame != 0 ? 1 : 2;
    sStallT[2] = port_time_us();
    SEG_END(SEG_PRE); /* audio commands, state change, gfx pool, pads -- a first half's; a second half's is the pool alone */
    gPortLogDefer = gGamestate == RACING && gIsGamePaused == 0; /* no memory-stick stalls in a race */
    if (!gPortLogDefer) {
        port_log_flush();
        gPortDrawDist = (float) PORT_DRAW_DIST; /* the governor starts every race from the full distance */
    }
    PORT_TRACE(" game_state_handler (state %d, menu %d)\n", gGamestate, gMenuSelection);
    port_rg_profile_begin(gGamestate == RACING && gActiveScreenMode == SCREEN_MODE_1P,
                          gCurrentCourseId);
#ifdef PORT_PROFILE
    {
        extern u32 port_time_us(void);
        extern void port_profile_add(int slot, u32 us);
        u32 t0 = port_time_us();
        game_state_handler();
        port_profile_add(0, port_time_us() - t0);
    }
#else
    game_state_handler();
#endif
    sStallT[3] = port_time_us();
    port_rg_profile_mark(RG_PHASE_OTHER);
    PORT_TRACE(" end_master_display_list\n");
    end_master_display_list();
    PORT_TRACE(" display_and_vsync (%d gfx cmds)\n", (int) (gDisplayListHead - gGfxPool->gfxPool));
    display_and_vsync();
    port_rg_profile_mark(RG_PHASE_DRAW);
    port_rg_profile_finish();
    {
        u32 end = port_time_us();
        if (gGamestate == RACING && end - sStallT[0] > 300000u && sStallT[3] != 0) {
            PORT_LOG("stall: %u ms in one race iteration (half %d): stats+log %u, pads+audio cmds %u, game logic %u, renderer+GE+vsync+mixer %u\n",
                     (unsigned) ((end - sStallT[0]) / 1000u), (int) gPortHalfFrame, (unsigned) ((sStallT[1] - sStallT[0]) / 1000u),
                     (unsigned) ((sStallT[2] - sStallT[1]) / 1000u), (unsigned) ((sStallT[3] - sStallT[2]) / 1000u),
                     (unsigned) ((end - sStallT[3]) / 1000u));
        }
    }
    PORT_TRACE(" frame done\n");
    if (sPortTraceFrames != 0) {
        sPortTraceFrames--;
    }
}
#endif

void thread5_game_loop(UNUSED void* arg) {
    osCreateMesgQueue(&gGfxVblankQueue, gGfxMesgBuf, 1);
    osCreateMesgQueue(&gGameVblankQueue, &gGameMesgBuf, 1);
    init_controllers();
    if (!wasSoftReset) {
        clear_nmi_buffer();
    }

    set_vblank_handler(2, &gGameVblankHandler, &gGameVblankQueue, (OSMesg) OS_EVENT_SW2);
    // These variables track stats such as player wins.
    // In the event of a console reset, it remembers them.
    nmi_gVersusResults2P = &pAppNmiBuffer[0]; // 2  u8's, tracks number of times player 1/2 won a VS race
    nmi_gVersusResults3P =
        &pAppNmiBuffer[2]; // 9  u8's, 3x3, tracks number of times player 1/2/3   has placed in 1st/2nd/3rd in a VS race
    nmi_gVersusResults4P = &pAppNmiBuffer[11]; // 12 u8's, 4x3, tracks number of times player 1/2/3/4 has placed in 1st/2nd/3rd
                                       // in a VS race
    gNmiUnknown4 = &pAppNmiBuffer[23]; // 2  u8's, tracking number of Battle mode wins by player 1/2
    gNmiUnknown5 = &pAppNmiBuffer[25]; // 3  u8's, tracking number of Battle mode wins by player 1/2/3
    gNmiUnknown6 = &pAppNmiBuffer[28]; // 4  u8's, tracking number of Battle mode wins by player 1/2/3/4
    rendering_init();
    read_controllers();
    func_800C5CB8();

    while (true) {
        func_800CB2C4();

        // Update the gamestate if it has changed (racing, menus, credits, etc.).
        if (gGamestateNext != gGamestate) {
            gGamestate = gGamestateNext;
            update_gamestate();
        }
        profiler_log_thread5_time(THREAD5_START);
        config_gfx_pool();
        read_controllers();
        game_state_handler();
        end_master_display_list();
        display_and_vsync();
    }
}

/**
 * Sound processing thread. Runs at 50 or 60 FPS according to osTvType.
 */
void thread4_audio(UNUSED void* arg) {
    UNUSED u32 unused[3];
    audio_init();
    osCreateMesgQueue(&sSoundMesgQueue, sSoundMesgBuf, ARRAY_COUNT(sSoundMesgBuf));
    set_vblank_handler(1, &sSoundVblankHandler, &sSoundMesgQueue, (OSMesg) 512);

    while (true) {
        OSMesg msg;
        struct SPTask* spTask;

        osRecvMesg(&sSoundMesgQueue, &msg, OS_MESG_BLOCK);

        profiler_log_thread4_time();

        spTask = create_next_audio_frame_task();
        if (spTask != NULL) {
            dispatch_audio_sptask(spTask);
        }
        profiler_log_thread4_time();
    }
}
