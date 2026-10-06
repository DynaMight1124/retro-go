/**
 * Ad hoc multiplayer for the PSP port: every machine runs the whole game and
 * they exchange controller inputs in lockstep (see docs/adhoc.md).
 *
 *   transport   net_file.c   mailbox files on the shared memory stick (PPSSPP
 *                            tests: two instances on one host), -DPORT_NET_FILE
 *               net_adhoc.c  PSP ad hoc PDP broadcast, -DPORT_NET_ADHOC
 *   session     lockstep.c   slots, input delay, redundancy, state checksums
 *
 * Everything is compiled only with -DPORT_NET.
 */
#ifndef PORT_NET_H
#define PORT_NET_H

#include <PR/ultratypes.h>
#include <PR/os_cont.h>

#define NET_MAX_PLAYERS 4
#define NET_ID_LEN 6 /* MAC address (ad hoc) or a fixed name (files) */

enum { NET_ROLE_NONE = 0, NET_ROLE_HOST = 1, NET_ROLE_CLIENT = 2 };

/* Transport ---------------------------------------------------------------- */
/* role: NET_ROLE_HOST or a client number 2..NET_MAX_PLAYERS. */
int net_transport_init(int role, const char* group);
void net_transport_term(void);
const u8* net_transport_local_id(void);
int net_transport_send(const void* pkt, int len);                /* to every peer */
int net_transport_recv(void* buf, int max, u8 from[NET_ID_LEN]); /* bytes, 0 = nothing pending */
const char* net_transport_status(void);
const char* net_transport_stats(void); /* traffic / error counters for the log */                          /* one line for the waiting screen */

/* Session ------------------------------------------------------------------ */
int port_net_boot(void); /* nothing now: the session starts from the game select */
/* The lobby (lockstep.c): opened by the game-select OK press for 2-4 players;
 * while it is active the menu is frozen (main.c), updated and drawn by these. */
void port_net_lobby_open(void);
void port_net_lobby_host(void); /* straight to hosting, no HOST / JOIN choice */
/* Game select, in a session, the host's OK on another number of players: the
 * session ends on every machine in that lockstep frame.  1 = this machine is
 * the host and goes on to its new race, 0 = a joiner, now under the "HOST
 * DISCONNECTED" prompt. */
int port_net_end_for_new_race(void);
/* The pause menu's LEAVE MULTIPLAYER, chosen by `slot`'s pad: a lockstep input
 * too, so every machine acts in that frame.  The leaver goes to the main
 * menu alone; if it was the host the session ends for everyone ("HOST EXITED
 * THE GAME"), otherwise the others drop the slot there and the host gets the
 * drop-out prompt (CONTINUE / EXIT) over the still-paused race. */
void port_net_leave(int slot);
int port_net_lobby_active(void);
void port_net_lobby_update(void);
void port_net_lobby_draw(void);
/* Drop-out prompts (host: CONTINUE / EXIT; joiner: WAITING FOR HOST, HOST
 * EXITED THE GAME, YOU WERE DROPPED): update from the main loop every
 * iteration, draw from the pause-menu render (race) / the menu overlay. */
int port_net_modal_active(void);
void port_net_modal_update(void);
void port_net_modal_draw(void);
int port_net_active(void);
int port_net_players(void);
int port_net_local_slot(void);
/* Explicit race lifecycle: reset result identity before initializing a course.
 * Accepted standings stay locked until the next setup_race.  Hold only the
 * results countdown while waiting; gameplay frames must keep supplying pads. */
void port_net_race_begin(void);
int port_net_result_locked(void);
int port_net_results_waiting(void);
/* Once per game iteration, before the game reads the pads: 1 = every slot's
 * input for this frame is here, 0 = stall (call again after a short delay). */
int port_net_frame_begin(void);
/* Fill pads[0..3] for the current frame (osContGetReadData; any number of times). */
void port_net_pads(OSContPad* pads);
/* After the game iteration: the next frame begins. */
void port_net_frame_end(void);
/* The local pad as the game would read it without a session (shim). */
void port_local_pad(OSContPad* pad);

/* Game side (skybox_and_splitscreen.c): draw the local slot's camera full screen
 * in the single-player layout; install its HUD coordinates after init_hud(). */
void port_render_local_player(void);
void port_net_hud_layout(void);

#endif
