/**
 * Mailbox-file transport (PPSSPP tests).  Two or more PPSSPP instances on one
 * host share the emulated memory stick, so each sender appends its packets to
 * its own file under ms0:/mk64net/ and every receiver tails the others.
 * Records are <u16 length><bytes>.  The host clears the directory at start.
 */
#if defined(PORT_NET) && defined(PORT_NET_FILE)
#include <ultra64.h>
#include <macros.h>
#include <stdio.h>
#include <string.h>
#include <pspiofilemgr.h>
#include "port_net.h"
#include "../port.h"

#define NET_FILE_DIR "ms0:/mk64net"
static const char* sNames[NET_MAX_PLAYERS] = { "host", "c1", "c2", "c3" };
static int sMe = -1;                       /* index into sNames */
static long sOffset[NET_MAX_PLAYERS];      /* read offset per sender */
static u8 sIds[NET_MAX_PLAYERS][NET_ID_LEN];
static char sStatus[96];

static void path_of(int i, char* out, int len) {
    snprintf(out, len, NET_FILE_DIR "/%s.pkt", sNames[i]);
}

int net_transport_init(int role, UNUSED const char* group) {
    char p[64];
    int i;
    sMe = role == NET_ROLE_HOST ? 0 : role - 1; /* client 2 -> c1 */
    if (sMe < 0 || sMe >= NET_MAX_PLAYERS) return 0;
    sceIoMkdir(NET_FILE_DIR, 0777);
    for (i = 0; i < NET_MAX_PLAYERS; i++) {
        memset(sIds[i], 0, NET_ID_LEN);
        strncpy((char*) sIds[i], sNames[i], NET_ID_LEN);
        sOffset[i] = 0;
        if (sMe == 0) { path_of(i, p, sizeof(p)); sceIoRemove(p); } /* fresh session */
    }
    path_of(sMe, p, sizeof(p));
    {
        FILE* f = fopen(p, "wb"); /* own mailbox: create / truncate */
        if (f == NULL) { snprintf(sStatus, sizeof(sStatus), "cannot create %s", p); return 0; }
        fclose(f);
    }
    snprintf(sStatus, sizeof(sStatus), "mailbox %s", sNames[sMe]);
    PORT_LOG("net: file transport as %s\n", sNames[sMe]);
    return 1;
}

void net_transport_term(void) {
}

const u8* net_transport_local_id(void) {
    return sIds[sMe];
}

int net_transport_send(const void* pkt, int len) {
    char p[64];
    FILE* f;
    u16 l = (u16) len;
    path_of(sMe, p, sizeof(p));
    f = fopen(p, "ab");
    if (f == NULL) return -1;
    fwrite(&l, 2, 1, f);
    fwrite(pkt, 1, len, f);
    fclose(f);
    return len;
}

/* Reads go through a per-sender buffer: one open per refill, many records per
 * refill.  (PPSSPP charges tens of milliseconds per open; one record per open
 * could not keep up with the host.)  A sender whose file does not exist yet is
 * retried only every 30 polls. */
static u8 sBuf[NET_MAX_PLAYERS][4096];
static int sFill[NET_MAX_PLAYERS], sPos[NET_MAX_PLAYERS], sMissing[NET_MAX_PLAYERS];

static int take_record(int i, void* buf, int max) {
    u16 l;
    if (sPos[i] + 2 > sFill[i]) return 0;
    memcpy(&l, sBuf[i] + sPos[i], 2);
    if (l == 0 || l > max || sPos[i] + 2 + l > sFill[i]) return 0; /* partial: re-read from sOffset later */
    memcpy(buf, sBuf[i] + sPos[i] + 2, l);
    sPos[i] += 2 + l;
    sOffset[i] += 2 + l;
    return l;
}

int net_transport_recv(void* buf, int max, u8 from[NET_ID_LEN]) {
    static int next;
    int n;
    for (n = 0; n < NET_MAX_PLAYERS; n++) {
        int i = (next + n) % NET_MAX_PLAYERS;
        char p[64];
        FILE* f;
        int got;
        if (i == sMe) continue;
        got = take_record(i, buf, max);
        if (got == 0) {
            if (sMissing[i] > 0) { sMissing[i]--; continue; }
            path_of(i, p, sizeof(p));
            f = fopen(p, "rb");
            if (f == NULL) { sMissing[i] = 30; continue; }
            fseek(f, sOffset[i], SEEK_SET);
            sFill[i] = (int) fread(sBuf[i], 1, sizeof(sBuf[i]), f);
            sPos[i] = 0;
            fclose(f);
            got = take_record(i, buf, max);
            if (got == 0) continue;
        }
        memcpy(from, sIds[i], NET_ID_LEN);
        next = i + 1; /* round-robin */
        return got;
    }
    return 0;
}

const char* net_transport_status(void) {
    return sStatus;
}
const char* net_transport_stats(void) { return "file transport"; }
#endif
