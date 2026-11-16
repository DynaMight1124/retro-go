#ifndef GTAVC_SND_H
#define GTAVC_SND_H

#include <stdint.h>
#include <stdbool.h>

#define SND_PUNCH 0
#define SND_GUN 1
#define SND_BAT 2
#define SND_SWING 3
#define SND_CRASH 4
#define SND_DOOR_OPEN 5
#define SND_DOOR_CLOSE 6
#define SND_HORN 7
#define SND_PICKUP 8
#define SND_EXPLODE 9
#define SND_SPLASH 10
#define SND_HITPED 11
#define SND_SIREN 12
#define SND_IDLE 13
#define SND_REV 14
#define SND_N 15

#define SND_V_PANIC 0
#define SND_V_DEATH 1
#define SND_V_FIGHT 2
#define SND_V_CHAT  3
#define SND_V_TOMMY 4

extern bool Snd_on;
extern bool Snd_ok;

void Snd_init(void);
void Snd_play(int id);
void Snd_playVol(int id, int vol);
void Snd_play3d(int id, int x, int z, int lx, int lz);
void Snd_voice(int kind, bool female);
void Snd_engine(bool running, int pct);
void Snd_siren(bool onNow);
void Snd_stop(int id);
void Snd_stopAll(void);
void Snd_radioPrepare(void);
void Snd_radioStart(void);
void Snd_radioStop(void);
void Snd_radioTick(int dtMs);
bool Snd_radioActive(void);
void Snd_mix(int16_t* out, int frames, int rate);
void Snd_cleanup(void);

#endif /* GTAVC_SND_H */
