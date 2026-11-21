#ifndef _LIBRETRO_LINK_H_
#define _LIBRETRO_LINK_H_

// Link interface: the frontend-hosted bus that lets two core instances in this
// process emulate machines joined by a cable.
//
// Not part of upstream libretro yet (libretro/RetroArch#19454), and
// deliberately NOT patched into the vendored libretro-common, which tracks
// upstream and should stay updatable. The block below is maintained
// byte-identically in libretro-godot's src/LinkInterface.hpp and mGBA's
// bundled libretro.h, so the copies can be diffed directly.

#include <libretro.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
 * Link interface (retroXR extension; candidate upstream environment 94).
 *
 * Deliberately one contiguous block rather than following libretro.h's usual
 * split of "environment defines grouped near the top, structs further down".
 * This block is maintained identically in two headers of very different
 * vintage (mGBA's bundled copy and libretro-godot's vendored libretro-common),
 * and keeping it in one piece makes them trivial to diff.
 * ------------------------------------------------------------------------- */

/* struct retro_link_interface * --
 * Lets a core join a link bus hosted by the frontend, so that two or more core
 * instances running in the same process can emulate machines that were
 * originally joined by a cable: a GBA link cable, a Game Boy serial cable, a
 * GameCube-to-GBA cable, and so on.
 *
 * This exists because core instances cannot reach each other on their own. A
 * frontend that runs several cores at once generally loads each from its own
 * copy of the shared library, so each instance has its own copy of every
 * global and no in-core coordinator can be shared between them. The frontend
 * is the only thing they have in common, so the frontend hosts the bus.
 *
 * The frontend arbitrates emulated time and moves opaque payloads. All
 * knowledge of the wire protocol stays in the cores; the frontend never
 * interprets a payload.
 *
 * Should be called in retro_init or retro_load_game. The returned function
 * pointers remain valid until retro_deinit, whether or not anything ever
 * attaches, so a core has exactly one code path and still runs standalone.
 *
 * During development this is flagged EXPERIMENTAL so as not to squat on plain
 * 94 should upstream assign it elsewhere. A core should probe the experimental
 * form first and fall back to the plain one, so a core built today keeps
 * working against a frontend that later adopts the unflagged number.
 */
#define RETRO_ENVIRONMENT_GET_LINK_INTERFACE       (94 | RETRO_ENVIRONMENT_EXPERIMENTAL)
#define RETRO_ENVIRONMENT_GET_LINK_INTERFACE_FINAL 94

/* Returned by retro_link_advance_t when nothing bounds this core: nothing is
 * attached to the port, or every peer has detached. The core must then run
 * freely. This is what lets a core that uses this interface behave exactly as
 * it does today when no cable is plugged in. */
#define RETRO_LINK_UNBOUNDED ((uint64_t)-1)

/* Peer id meaning "every other peer on this port's bus". */
#define RETRO_LINK_BROADCAST 0xFF

/* Why advance returned before request_tick became available. Multiple reasons
 * may be present. The core should drain recv on MESSAGE and refresh peers on
 * TOPOLOGY before asking to advance again. */
#define RETRO_LINK_WAKE_NONE     0u
#define RETRO_LINK_WAKE_MESSAGE  (1u << 0)
#define RETRO_LINK_WAKE_TOPOLOGY (1u << 1)
#define RETRO_LINK_WAKE_DETACHED (1u << 2)

/* An attached port, as the frontend knows it.
 *
 * Handed back by attach and passed to every other call, the way a file handle
 * is by the VFS interface's open. The core never looks inside it.
 *
 * It names a PORT rather than a core, and that is what makes it necessary: a
 * machine with more than one socket -- a GameCube has four -- must say which
 * one it means on every call, so an argument of some kind was always going to
 * be here. Naming the port with a token the frontend issued, rather than a
 * bare number, lets the frontend validate it and tells it which core is
 * calling for free.
 *
 * That second part is worth having. libretro's function pointers carry no
 * userdata, so the usual way to recover the caller is a thread-local set when
 * the core's emulation thread starts, which holds only while a core does its
 * work on that one thread. Plenty do not: Dolphin runs its CPU on a thread of
 * its own whenever dual-core is enabled, and that is a user-facing setting. */
typedef struct retro_link_port retro_link_port_t;

/* Join the bus on `port`.
 *
 * `protocol_id` names the emulated wire protocol, e.g. "gba-sio". Peers
 * whose ids differ are never connected to each other, which is what allows
 * two different cores emulating two different machines to share one cable
 * while keeping unrelated cores apart.
 *
 * `clock_rate` is the number of ticks of this core's link timeline that
 * elapse per second of emulated time (e.g. 16777216 for a Game Boy Advance),
 * so the frontend can convert exactly between participants' units. It must not
 * be zero.
 *
 * Returns a handle to pass to every other call on this port, or NULL if it
 * could not be attached. The core keeps it for as long as the port is attached
 * and hands it back unchanged; nothing about its contents is a core's business.
 *
 * The bus index a core needs for addressing comes from peers(), not from here,
 * because a cable can be plugged or pulled at any time and an index handed out
 * once would be stale by the next transfer. */
typedef retro_link_port_t *(RETRO_CALLCONV *retro_link_attach_t)(unsigned port,
      const char *protocol_id, uint64_t clock_rate);

/* Leave the bus. Peers observe this as a detach at the current tick. */
typedef void (RETRO_CALLCONV *retro_link_detach_t)(retro_link_port_t *handle);

/* Current membership of this port's bus.
 *
 * Returns this core's index on the bus (0 or greater), or -1 when the port is
 * not attached or is not cabled to anything. If `count` is non-NULL it
 * receives the number of participants, including this one.
 *
 * Cores generally need this per transfer rather than once at attach time: the
 * host decides what a port is cabled to, and a cable can be plugged or pulled
 * while the machine is running, so a core's index and peer count can change
 * during a session. */
typedef int (RETRO_CALLCONV *retro_link_peers_t)(retro_link_port_t *handle, unsigned *count);

/* Queue `buf` for delivery to peer `to` (or RETRO_LINK_BROADCAST), stamped at
 * `tick` on this core's timeline. The frontend copies the payload and does not
 * interpret it. Returns false if the port is not attached. */
typedef bool (RETRO_CALLCONV *retro_link_send_t)(retro_link_port_t *handle, uint64_t tick,
      unsigned to, const void *buf, size_t len);

/* Pop the next queued message, oldest first, with its tick translated into this
 * core's own units. Returns false when the queue is empty. `len` is in/out:
 * buffer capacity on entry, bytes written on return.
 *
 * The tick says when the event LANDS, not when the message became readable. A
 * core is told about a transfer ahead of time precisely so it can schedule its
 * own side of it for that tick, so messages are handed over as soon as they
 * arrive rather than held until the clock catches up. What stops one landing in
 * the past is the sender's commit horizon: it promised not to originate before
 * that tick, and the bus will not have let this core run beyond it. */
typedef bool (RETRO_CALLCONV *retro_link_recv_t)(retro_link_port_t *handle, uint64_t *tick,
      unsigned *from, void *buf, size_t *len);

/* Publish this core's position and its commit horizon, then ask how far it may
 * advance. Blocks until `request_tick` can be granted or until every peer has
 * detached.
 *
 * `local_tick` is where the core is now. `safe_tick` is a promise that the core
 * will not originate any event a peer must observe before that tick; it must be
 * greater than or equal to `local_tick` and, once published, may never be
 * retracted.
 *
 * The horizon is what keeps the bus deadlock-free, and it is required, not an
 * optimization. A core that always passes `safe_tick == local_tick` promises
 * nothing, so if every participant does that they all sit at the same tick
 * waiting for someone else to move and none of them ever can. Publishing a
 * horizon some way ahead of the current position lets peers run to it without
 * asking, and the core makes the promise true by scheduling anything it
 * originates at the horizon rather than at the instant the guest asked for it.
 *
 * The horizon is therefore a tuning knob, traded against fidelity: a larger one
 * means fewer rendezvous and more emulated delay before an originated event
 * lands. Within a single process it only has to be large enough to avoid
 * synchronizing every few cycles, so a small fraction of a frame is plenty.
 *
 * Returns the tick this core may advance to: `request_tick`, or more only
 * where a previous grant or the core's own position already stood further on.
 * The grant is never larger than what was asked for even when more headroom
 * happens to be available, because how far a peer has run at this instant is a
 * wall-clock accident and returning it would make two machines replaying
 * identical inputs disagree. Ask for more to get more.
 *
 * The result is always at least `local_tick`, never less than a previously
 * returned value, or RETRO_LINK_UNBOUNDED.
 *
 * The grant MUST be a pure function of the participants' published ticks. It
 * must never depend on wall-clock time, nor on the order in which threads
 * happen to arrive. A frontend that violates this will desync any peer that
 * replays the same inputs expecting the same result, which is exactly what
 * rollback and lockstep multiplayer do. For the same reason there is
 * deliberately no timeout: a peer that is behind is waited for, not guessed at.
 *
 * Thread-safe. Cores generally run on their own emulation threads, and this
 * call is the rendezvous between them. */
typedef uint64_t (RETRO_CALLCONV *retro_link_advance_t)(retro_link_port_t *handle,
      uint64_t local_tick, uint64_t safe_tick, uint64_t request_tick,
      uint32_t *wake_flags);

/* @see RETRO_ENVIRONMENT_GET_LINK_INTERFACE */
struct retro_link_interface
{
   /* All set by the frontend; none are NULL if the environment call
    * returned true. */
   retro_link_attach_t  attach;
   retro_link_detach_t  detach;
   retro_link_peers_t   peers;
   retro_link_send_t    send;
   retro_link_recv_t    recv;
   retro_link_advance_t advance;
};

#ifdef __cplusplus
}
#endif

#endif // _LIBRETRO_LINK_H_
