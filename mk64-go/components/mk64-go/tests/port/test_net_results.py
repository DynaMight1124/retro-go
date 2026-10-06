#!/usr/bin/env python3
"""Run the production lockstep/result code over an in-memory lossy transport.

Requires Python 3 and a host C compiler (CC, default clang), no PSP SDK.
Each shared library is a console with independent C globals. Functions are
extracted verbatim so the test exercises the actual input ring, relay, packet
handlers, result protocol, and rank writers. Platform/UI calls and driving are
stubbed; this is not a substitute for a two-PSP race and results-screen test.
"""

import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def harness_source():
    net = (ROOT / "src/port/net/lockstep.c").read_text()
    ranks = (ROOT / "src/cpu_vehicles_camera_path.c").read_text()
    race = (ROOT / "src/racing/race_logic.c").read_text()
    # Compile the real RACE_DONE case, including its results-countdown gate.
    race = race[race.index("void func_8028FCBC(void) {"):]
    done = race[race.index("        case RACE_DONE:"):race.index("        case RACE_QUITTING:")]
    prefix = r'''
#include <stdint.h>
#include <string.h>
#include <assert.h>
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32; typedef float f32;
typedef struct { u16 button; s8 stick_x, stick_y; u8 errno; } OSContPad;
#define PORT_NET 1
const uint16_t gPortBuildId = 0x1234; /* tools/psp/gen_build_id.py in the real build */
#define PORT_NET_ADHOC 1
#define NET_MAX_PLAYERS 4
#define NET_ID_LEN 6
#define NUM_PLAYERS 8
#define NET_ROLE_NONE 0
#define NET_ROLE_HOST 1
#define NET_ROLE_CLIENT 2
#define RACE_IN_PROGRESS 3
#define RACE_HUMAN_FINISHED 4
#define RACE_DONE 5
#define RACE_QUADRANT_RESULTS 7
#define RACING 4
#define GRAND_PRIX 0
#define TIME_TRIALS 1
#define VERSUS 2
#define BATTLE 3
#define SCREEN_MODE_1P 0
#define PLAYER_CINEMATIC_MODE 0x800
#define UNUSED
#define PORT_LOG(...) ((void)0)
#define API __attribute__((visibility("default")))
static u32 clock_us;
static u32 sceKernelGetSystemTimeLow(void) { return clock_us; }
static int gGamestate=RACING, gModeSelection, gPlayerCount, gPlayerCountSelection1;
static int gScreenModeSelection=1, gIsGamePaused, gIsInQuitToMenuTransition;
static int gPauseTriggered, gCycleFlashMenu, gMenuTimingCounter;
s32 gGlobalTimer, gDemoTimer, gPlayerWinningIndex, D_80150120;
static u16 gRaceState, gRandomSeed16;
static s32 gGPCurrentRaceRankByPlayerId[8],gGPCurrentRaceRankByPlayerIdDup[8],gPreviousGPCurrentRaceRankByPlayerId[8];
static s16 gGPCurrentRacePlayerIdByRank[8],gPrevPlayerIdByRank[8],gPlayerPositionLUT[8];
static s8 gGPPointsByCharacterId[8];
static u8 nmi[64], *pAppNmiBuffer=nmi;
static struct { u16 type; s16 currentRank; } gPlayers[8];
static f32 gCourseCompletionPercentByRank[8],gCourseCompletionPercentByPlayerId[8],gTimePlayerLastTouchedFinishLine[8];
static s16 D_8016348C;
int port_net_active(void);
static void func_8028DF00(void) {}
static void func_8028DF38(void) {}
static void func_800C9F90(int on) {}
static void port_local_pad(OSContPad *pad) { memset(pad,0,sizeof(*pad)); }
u32 port_log_cost_ms(void) { return 0; }
static void log_state(const char *why,u32 frame) {}
static void dump_player(u32 frame,int slot) {}
static u32 state_checksum(u32 parts[4]) { memset(parts,0,4*sizeof(u32)); return 0; }
static const char* net_transport_stats(void) { return "test"; }
'''
    globals_ = net[net.index("#define RING "):net.index("static u32 now_us(void)")]
    transport = r'''
typedef struct { NetPkt p; u8 from[6]; } Wire;
static Wire incoming[4096]; static NetPkt outgoing[4096];
static int in_read,in_write,out_read,out_write;
static u8 local_id[6];
static const u8* net_transport_local_id(void) { return local_id; }
static int net_transport_send(const void *p,int len) {
    assert(len==sizeof(NetPkt));assert(out_write-out_read<4096);
    outgoing[out_write++%4096]=*(const NetPkt*)p;return len;
}
static int net_transport_recv(void *p,int max,u8 from[6]) {
    if(in_read==in_write)return 0;
    Wire *w=&incoming[in_read++%4096];memcpy(p,&w->p,sizeof(NetPkt));memcpy(from,w->from,6);return sizeof(NetPkt);
}
static int criteria_match(const NetPkt *p) { return 1; }
static void session_begin(void) { sRunning=1; }
static void end_session_to_menu(void) { sRunning=0; }
static NetPkt sStartPkt; static int sStartSaved, sBeganMidIteration;
'''
    signatures = [
        "static u32 now_us(void) {", "static u32 fnv(",
        "static int slot_of(", "static void store_input(", "static int is_dropped(",
        "static int have_input(", "static void fill_drop(", "static void send_pkt(",
        "static void send_start(void) {", "static void send_hello(void) {",
        "static void latest_check(", "static void send_slot_inputs(",
        "static void send_inputs(void) {", "static void apply_drops(",
        "static void handle_pkt(", "static void poll(void) {",
        "static void session_reset(void) {", "int port_net_active(void) {",
        "void port_net_race_begin(void) {", "int port_net_result_locked(void) {",
        "int port_net_results_waiting(void) {", "static void send_result(u8 mode) {",
        "static void apply_result(void) {", "static void receive_result(const NetPkt* p, int slot) {",
        "static int result_frame_begin(void) {", "static int race_can_pause(void) {",
        "static void net_pause_hard(void) {", "static void net_pause(int on) {",
        "static void modal_open(int which) {", "static void drop_slot(",
        "int port_net_frame_begin(void) {", "void port_net_frame_end(void) {",
    ]
    actual = "\n".join(function(net, signature) for signature in signatures)
    actual += "\n" + "\n".join(function(ranks, signature) for signature in [
        "void set_places(void) {", "void update_player_rankings(void) {",
        "void set_places_end_course_with_time(void) {",
    ])
    wrappers = r'''
static int shown,first_countdown=-1,finish_at,local_winner;
static void func_8028E678(void) { shown=1; }
static void func_80092564(void) { shown=1; }
static void func_8028E438(void) { shown=1; }
static void func_8028F4E8(void) {}
static void game_tick(void) {
    if(gIsGamePaused)return;
    if(gRaceState==RACE_IN_PROGRESS && sFrame>=(u32)finish_at) {
        gRaceState=RACE_DONE;gPlayerWinningIndex=local_winner;gDemoTimer=10;
        for(int i=0;i<8;i++) gGPCurrentRaceRankByPlayerId[i]=(i+8-local_winner)%8;
        // VS/Battle only rank participating slots; GP includes all eight.
        int count=gModeSelection==GRAND_PRIX?8:sPlayers;
        for(int i=0;i<count;i++) gGPCurrentRaceRankByPlayerId[i]=(i+count-local_winner)%count;
        int offset=gModeSelection==BATTLE?(sPlayers==2?23:sPlayers==3?25:28):(sPlayers==2?0:sPlayers==3?2:11);
        int stride=gModeSelection==VERSUS && sPlayers>2?3:1;
        nmi[offset+local_winner*stride]++;
    }
    if(gRaceState==RACE_DONE && !port_net_results_waiting() && first_countdown<0)first_countdown=sFrame;
    switch(gRaceState) {
''' + done + r'''
    }
}
API void test_init(int slot,int players,int mode,int finish,int winner) {
    session_reset(); sRole=slot?NET_ROLE_CLIENT:NET_ROLE_HOST;sSlot=slot;sPlayers=players;sRunning=1;
    sSessionId=12345;sBeganMidIteration=0;sLastHostPktUs=clock_us=100000;
    in_read=in_write=out_read=out_write=0;gGamestate=RACING;gRaceState=RACE_IN_PROGRESS;
    gModeSelection=mode;gPlayerCount=gPlayerCountSelection1=players;gIsGamePaused=0;
    finish_at=finish;local_winner=winner;shown=0;first_countdown=-1;gDemoTimer=10;gPlayerWinningIndex=winner;
    memset(nmi,0,sizeof(nmi)); memset(gGPPointsByCharacterId,0,sizeof(gGPPointsByCharacterId));
    memset(local_id,0,6);local_id[0]=slot+1;
    for(int i=0;i<4;i++){sKnown[i]=1;memset(sIds[i],0,6);sIds[i][0]=i+1;}
    for(int i=0;i<8;i++){
        gGPCurrentRaceRankByPlayerId[i]=gGPCurrentRacePlayerIdByRank[i]=gPrevPlayerIdByRank[i]=i;
        gPlayers[i].type=0;gCourseCompletionPercentByPlayerId[i]=8-i;
    }
    port_net_race_begin();
}
API int test_step(u32 now) {
    clock_us=now;
    if(!port_net_frame_begin())return 0;
    game_tick();game_tick();port_net_frame_end();return 1;
}
API void test_receive(const void *p,int sender) {
    assert(in_write-in_read<4096);Wire *w=&incoming[in_write++%4096];
    memcpy(&w->p,p,sizeof(NetPkt));memset(w->from,0,6);w->from[0]=sender+1;
}
API void test_poll(void) { poll(); }
API void test_hello(void) { memcpy(sHostId,sIds[0],6);send_hello(); }
API int test_send(void *p) {
    if(out_read==out_write)return 0;
    memcpy(p,&outgoing[out_read++%4096],sizeof(NetPkt));return sizeof(NetPkt);
}
API int test_packet_size(void) { return sizeof(NetPkt); }
API int test_packet_field(const void *data,int field) {
    const NetPkt *p=data;
    switch(field){case 0:return p->type;case 1:return p->mode;case 2:return p->frame;case 3:return p->race;default:return p->session;}
}
API int test_get(int field,int index) {
    switch(field){
    case 0:return sFrame;case 1:return sEnded;case 2:return sDropped;case 3:return sResultGot;
    case 4:return gPlayerWinningIndex;case 5:return gGPCurrentRaceRankByPlayerId[index];
    case 6:return nmi[index];case 7:return first_countdown;case 8:return shown;
    case 9:return sRaceId;case 10:return sResultAck;case 11:return gRaceState;
    case 12:return gDemoTimer;case 13:return sResultAllAcked;case 14:return gGPPointsByCharacterId[index];
    case 15:return sModal;case 16:return gIsGamePaused;case 17:return gGPCurrentRacePlayerIdByRank[index];
    case 18:return gRandomSeed16;default:return -1;
    }
}
API void test_next_race(void) {
    port_net_race_begin();gRaceState=RACE_IN_PROGRESS;finish_at=sFrame+25;
    first_countdown=-1;shown=0;gPlayerWinningIndex=local_winner;
}
API void test_rank_updates(int which) {
    for(int i=0;i<8;i++) {
        gCourseCompletionPercentByPlayerId[i]=gGPCurrentRaceRankByPlayerId[i];
        gTimePlayerLastTouchedFinishLine[i]=-gGPCurrentRaceRankByPlayerId[i];
        gPlayers[i].type=0;
    }
    if(which==0)set_places();
    if(which==1)update_player_rankings();
    if(which==2)set_places_end_course_with_time();
}
API void test_edit(int field,int value) {
    if(field==0) gPlayerWinningIndex=value;
    if(field==1) nmi[0]=value;
    if(field==2) gGPPointsByCharacterId[0]=value;
    if(field==3) sSessionId=value;
    if(field==4) sDesyncHits=value;
}
'''
    return prefix + globals_ + transport + actual + wrappers


class Network:
    def __init__(self, libraries, count=2, mode=2, finishes=None):
        self.peers = libraries[:count]
        self.now = 100000
        self.packets = []
        self.filter = lambda sender, recipient, packet: True
        for slot, peer in enumerate(self.peers):
            finish = finishes[slot] if finishes else 15
            peer.test_init(slot, count, mode, finish, slot)
        self.size = self.peers[0].test_packet_size()

    def field(self, packet, field):
        return self.peers[0].test_packet_field(packet, field)

    def tick(self, order=None):
        self.now += 10000
        for slot in order if order is not None else range(len(self.peers)):
            peer = self.peers[slot]
            peer.test_step(self.now)
            buffer = ctypes.create_string_buffer(self.size)
            while peer.test_send(buffer):
                packet = buffer.raw
                self.packets.append((slot, packet))
                for recipient, target in enumerate(self.peers):
                    if recipient != slot and self.filter(slot, recipient, packet):
                        target.test_receive(packet, slot)

    def run(self, condition, limit=1500, order=None):
        for _ in range(limit):
            self.tick(order)
            if condition():
                return
        raise AssertionError("network did not reach expected state: " + str([
            [p.test_get(f, 0) for f in (0, 1, 2, 3, 7, 8, 12)] for p in self.peers]))


class ResultsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="mk64-net-results-")
        directory = Path(cls.temp.name)
        source = directory / "net.c"
        source.write_text(harness_source())
        binary = directory / "net.so"
        subprocess.run([os.environ.get("CC", "clang"), "-shared", "-fPIC", "-fvisibility=hidden",
                        "-std=gnu99", "-O1", "-g", "-Wall", "-Wno-unused-function",
                        "-Wno-unused-variable", str(source), "-o", str(binary)], check=True)
        cls.libraries = []
        for slot in range(4):
            target = directory / f"peer{slot}.so"
            shutil.copyfile(binary, target)
            cls.libraries.append(ctypes.CDLL(str(target)))

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def assert_results(self, net):
        host = net.peers[0]
        for peer in net.peers:
            self.assertEqual(peer.test_get(1, 0), 0, "terminal disconnect")
            self.assertEqual(peer.test_get(2, 0), 0, "peer falsely dropped")
            self.assertEqual(peer.test_get(4, 0), host.test_get(4, 0))
            self.assertEqual(peer.test_get(7, 0), host.test_get(7, 0), "countdown began on different frames")
            for i in range(8):
                self.assertEqual(peer.test_get(5, i), host.test_get(5, i))
            for i in range(32):
                self.assertEqual(peer.test_get(6, i), host.test_get(6, i))

    def test_finish_order_and_star_relay(self):
        for count in (2, 3, 4):
            for finishes in ([35] + [5] * (count - 1), [5] + [35] * (count - 1)):
                with self.subTest(players=count, finishes=finishes):
                    net = Network(self.libraries, count, finishes=finishes)
                    # Clients have no direct connection to one another.
                    net.filter = lambda sender, recipient, packet: sender == 0 or recipient == 0
                    net.run(lambda: all(p.test_get(8, 0) for p in net.peers), order=list(reversed(range(count))))
                    self.assert_results(net)

    def test_burst_loss_of_result_and_acks(self):
        net = Network(self.libraries, 4, finishes=[15, 5, 25, 30])
        losses = {}
        def deliver(sender, recipient, packet):
            if net.field(packet, 0) == 5:
                key = (sender, recipient, net.field(packet, 1))
                losses[key] = losses.get(key, 0) + 1
                return losses[key] > 3
            return sender == 0 or recipient == 0
        net.filter = deliver
        net.run(lambda: all(p.test_get(8, 0) for p in net.peers))
        self.assert_results(net)
        self.assertGreater(losses[(1, 0, 3)], 3)

    def test_standings_stay_locked(self):
        net = Network(self.libraries)
        net.run(lambda: all(p.test_get(3, 0) for p in net.peers))
        for peer in net.peers:
            expected = [peer.test_get(5, i) for i in range(8)]
            for writer in range(3):
                peer.test_rank_updates(writer)
                self.assertEqual([peer.test_get(5, i) for i in range(8)], expected)
        net.run(lambda: all(p.test_get(8, 0) for p in net.peers))
        self.assert_results(net)

    def test_result_received_while_stalled_for_input(self):
        net = Network(self.libraries, finishes=[15, 35])
        net.filter = lambda sender, recipient, packet: net.field(packet, 0) != 5
        net.run(lambda: net.peers[0].test_get(3, 0))
        packet = next(packet for sender, packet in net.packets if sender == 0 and net.field(packet, 0) == 5)
        release_frame = net.field(packet, 2)
        net.run(lambda: net.peers[1].test_get(0, 0) == release_frame)
        client = net.peers[1]
        client.test_receive(packet, 0)
        net.tick()
        self.assertEqual(client.test_get(0, 0), release_frame, "client should still lack the release frame's host input")
        self.assertEqual(client.test_get(3, 0), 1)
        self.assertEqual(client.test_get(11, 0), 5)
        self.assertTrue(any(sender == 1 and net.field(p, 0) == 5 and net.field(p, 1) == 3
                            for sender, p in net.packets))
        net.filter = lambda sender, recipient, packet: True
        net.run(lambda: all(p.test_get(8, 0) for p in net.peers))
        self.assert_results(net)

    def test_final_result_takes_precedence_over_pending_desync(self):
        net = Network(self.libraries, finishes=[5, 35])
        net.filter = lambda sender, recipient, packet: net.field(packet, 0) != 5
        net.run(lambda: net.peers[0].test_get(3, 0))
        packet = next(packet for sender, packet in net.packets if sender == 0 and net.field(packet, 0) == 5)
        client = net.peers[1]
        client.test_edit(4, 3)
        client.test_receive(packet, 0)
        net.filter = lambda sender, recipient, packet: True
        net.run(lambda: all(p.test_get(8, 0) for p in net.peers))
        self.assert_results(net)
        # The same diagnostic during active racing still ends the session.
        client.test_init(1, 2, 2, 35, 1)
        client.test_edit(4, 3)
        client.test_step(110000)
        self.assertEqual(client.test_get(1, 0), 1)

    def test_joiner_can_retry_start_without_session_id(self):
        net = Network(self.libraries)
        host, client = net.peers
        client.test_edit(3, 0)  # START was lost, so the joiner has no session yet.
        client.test_hello()
        buffer = ctypes.create_string_buffer(net.size)
        self.assertEqual(client.test_send(buffer), net.size)
        host.test_receive(buffer.raw, 1)
        host.test_poll()
        self.assertEqual(host.test_send(buffer), net.size)
        self.assertEqual(net.field(buffer.raw, 0), 2)
        self.assertEqual(net.field(buffer.raw, 4), 12345)

    def test_battle_scores(self):
        for count in (2, 3, 4):
            net = Network(self.libraries, count, mode=3, finishes=[25] + [5] * (count - 1))
            net.run(lambda: all(p.test_get(8, 0) for p in net.peers))
            self.assert_results(net)
            offset = {2: 23, 3: 25, 4: 28}[count]
            self.assertEqual(net.peers[1].test_get(6, offset), 1)
            self.assertEqual(net.peers[1].test_get(6, offset + 1), 0)

    def test_successive_races(self):
        for count, mode in ((2, 0), (2, 2), (3, 2), (4, 2), (2, 3)):
            net = Network(self.libraries, count, mode)
            for race_id in range(1, 4):
                net.run(lambda: all(p.test_get(8, 0) for p in net.peers))
                self.assert_results(net)
                for peer in net.peers:
                    self.assertEqual(peer.test_get(9, 0), race_id)
                if race_id < 3:
                    # Align the test's explicit setup call to one lockstep frame.
                    frame = max(p.test_get(0, 0) for p in net.peers)
                    net.run(lambda: all(p.test_get(0, 0) == frame for p in net.peers),
                            order=[i for i, p in enumerate(net.peers) if p.test_get(0, 0) < frame])
                    for peer in net.peers:
                        peer.test_next_race()

    def test_duplicate_does_not_reapply_scores(self):
        net = Network(self.libraries, mode=0)
        net.run(lambda: all(p.test_get(8, 0) for p in net.peers))
        original = next(packet for sender, packet in net.packets if sender == 0 and net.field(packet, 0) == 5)
        client = net.peers[1]
        client.test_edit(2, 9)  # GP results animation has already awarded points.
        client.test_receive(original, 0)
        net.tick()
        self.assertEqual(client.test_get(14, 0), 9)
        self.assertTrue(any(sender == 1 and net.field(packet, 0) == 5 and net.field(packet, 1) == 3
                            for sender, packet in net.packets[-10:]))

    def test_previous_race_and_session_packets_are_ignored(self):
        net = Network(self.libraries)
        net.run(lambda: all(p.test_get(8, 0) for p in net.peers))
        old = [(sender, packet) for sender, packet in net.packets if net.field(packet, 0) == 5]
        for peer in net.peers:
            peer.test_next_race()
        self.assertEqual(net.peers[0].test_get(18, 0), net.peers[1].test_get(18, 0))
        for sender, packet in old:
            net.peers[1 - sender].test_receive(packet, sender)
        net.tick()
        self.assertEqual(net.peers[1].test_get(3, 0), 0)
        self.assertEqual(net.peers[0].test_get(10, 0), 0)
        # Same race number but a different host session must also reject them.
        client = net.peers[1]
        client.test_init(1, 2, 2, 35, 1)
        client.test_edit(3, 98765)
        for sender, packet in old:
            if sender == 0: client.test_receive(packet, 0)
        client.test_step(net.now + 10000)
        self.assertEqual(client.test_get(3, 0), 0)

    def test_permanent_loss_never_publishes_local_results(self):
        for lost_mode in (1, 3):
            net = Network(self.libraries, finishes=[15, 5])
            net.filter = lambda sender, recipient, packet: not (net.field(packet, 0) == 5 and net.field(packet, 1) == lost_mode)
            net.run(lambda: all(p.test_get(1, 0) for p in net.peers))
            for peer in net.peers:
                self.assertEqual(peer.test_get(8, 0), 0)
                self.assertEqual(peer.test_get(16, 0), 1)
            # A late snapshot cannot unpause a terminal failure.
            packet = next(packet for sender, packet in net.packets if sender == 0 and net.field(packet, 0) == 5 and net.field(packet, 1) == 1)
            net.peers[1].test_receive(packet, 0)
            net.tick()
            self.assertEqual(net.peers[1].test_get(16, 0), 1)

    def test_retries_use_frozen_host_snapshot(self):
        net = Network(self.libraries)
        net.filter = lambda sender, recipient, packet: not (sender == 1 and net.field(packet, 0) == 5)
        net.run(lambda: net.peers[0].test_get(3, 0))
        original = next(packet for sender, packet in net.packets if sender == 0 and net.field(packet, 0) == 5)
        net.peers[0].test_edit(0, 1)
        net.peers[0].test_edit(1, 99)
        for _ in range(10): net.tick()
        retries = [packet for sender, packet in net.packets if sender == 0 and net.field(packet, 0) == 5]
        self.assertGreater(len(retries), 1)
        self.assertTrue(all(packet == original for packet in retries))


if __name__ == "__main__":
    unittest.main(verbosity=2)
