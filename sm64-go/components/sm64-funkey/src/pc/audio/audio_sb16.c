#ifdef TARGET_DOS

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <allegro.h>

#include "macros.h"
#include "audio_api.h"

#define SNDPACKETLEN (8 * 1024)
#define OUTFREQ 32000
#define NUMSAMPLES 512
#define BUFSIZE (NUMSAMPLES * 2 * 2)

static AUDIOSTREAM *stream;

// this is basically SDL_dataqueue but slightly less generic

typedef struct sndpacket {
    size_t datalen;         /* bytes currently in use in this packet. */
    size_t startpos;        /* bytes currently consumed in this packet. */
    struct sndpacket *next; /* next item in linked list. */
    uint8_t data[];         /* packet data */
} sndpacket_t;

static sndpacket_t *qhead;
static sndpacket_t *qtail;
static sndpacket_t *qpool;
static volatile size_t queued;

static void sndqueue_init(const size_t bufsize) {
    const size_t wantpackets = (bufsize + (SNDPACKETLEN - 1)) / SNDPACKETLEN;
    for (size_t i = 0; i < wantpackets; ++i) {
        sndpacket_t *packet = malloc(sizeof(sndpacket_t) + SNDPACKETLEN);
        if (packet) {
            packet->datalen = 0;
            packet->startpos = 0;
            packet->next = qpool;
            qpool = packet;
        }
    }
}

static inline void xorcpy16(uint16_t *dst, const uint16_t *src, const size_t len) {
    register uint16_t *outp = dst;
    register const uint16_t *inp = src;
    const uint16_t *end = outp + len;
    for (; outp < end; ++outp, ++inp)
        *outp = (*inp) ^ 0x8000;
}

static size_t sndqueue_read(void *buf, size_t len) {
    sndpacket_t *packet;
    uint8_t *ptr = buf;

    while ((len > 0) && ((packet = qhead) != NULL)) {
        const size_t avail = packet->datalen - packet->startpos;
        const size_t tx = (len < avail) ? len : avail;

        xorcpy16((uint16_t *)ptr, (uint16_t *)(packet->data + packet->startpos), tx >> 1);
        packet->startpos += tx;
        ptr += tx;
        queued -= tx;
        len -= tx;

        if (packet->startpos == packet->datalen) {
            qhead = packet->next;
            packet->next = qpool;
            qpool = packet;
        }
    }

    if (qhead == NULL)
        qtail = NULL;

    return (size_t)(ptr - (uint8_t*)buf);
}

static inline sndpacket_t *alloc_sndpacket(void) {
    sndpacket_t *packet = qpool;

    if (packet) {
        qpool = packet->next;
    } else {
        packet = malloc(sizeof(sndpacket_t) + SNDPACKETLEN);
        if (!packet) return NULL;
    }

    packet->datalen = 0;
    packet->startpos = 0;
    packet->next = NULL;

    if (qtail == NULL)
        qhead = packet;
    else
        qtail->next = packet;
    qtail = packet;

    return packet;
}

static int sndqueue_push(const void *data, size_t len) {
    const uint8_t *ptr = data;

    while (len > 0) {
        sndpacket_t *packet = qtail;
        if (!packet || (packet->datalen >= SNDPACKETLEN)) {
            packet = alloc_sndpacket();
            if (!packet) {
                // out of memory, fuck everything
                return -1;
            }
        }

        const size_t room = SNDPACKETLEN - packet->datalen;
        const size_t datalen = (len < room) ? len : room;
        memcpy(packet->data + packet->datalen, ptr, datalen);
        ptr += datalen;
        len -= datalen;
        packet->datalen += datalen;
        queued += datalen;
    }

    return 0;
}

static void audio_int(void) {
    uint8_t *buf = get_audio_stream_buffer(stream);
    if (buf) {
        int len = BUFSIZE;
        const size_t tx = sndqueue_read(buf, len);
        buf += tx;
        len -= (int)tx;
        if (len > 0) memset(buf, 0, len);
        free_audio_stream_buffer(stream);
    }
}
END_OF_FUNCTION(audio_int)

static bool audio_sb_init(void) {
    reserve_voices(1, 0);

    if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL) == -1) {
        printf("install_sound() failed: %s\n", allegro_error);
        return false;
    }

    stream = play_audio_stream(NUMSAMPLES, 16, 1, OUTFREQ, 64, 128);
    if (!stream) {
        printf("play_audio_stream() failed: %s\n", allegro_error);
        remove_sound();
        return false;
    }

    LOCK_VARIABLE(queued);
    LOCK_FUNCTION(audio_int);
    install_int(audio_int, 3);

    return true;
}

static int audio_sb_buffered(void) {
    return queued / 4;
}

static int audio_sb_get_desired_buffered(void) {
    return 1100;
}

static void audio_sb_play(const uint8_t *buf, size_t len) {
    // Don't fill the audio buffer too much in case this happens
    if (stream && queued < 24000)
        sndqueue_push(buf, len);
}

static void audio_sb_shutdown(void) {
    if (stream) {
        remove_int(audio_int);
        stop_audio_stream(stream);
        remove_sound();
        stream = NULL;
    }
}

struct AudioAPI audio_sb = {
    audio_sb_init,
    audio_sb_buffered,
    audio_sb_get_desired_buffered,
    audio_sb_play,
    audio_sb_shutdown
};

#endif
