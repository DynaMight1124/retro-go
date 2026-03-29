/*
 * vfx.h - Visual effects (snow, clouds, particles, etc.)
 */

#ifndef VFX_H
#define VFX_H

#include <stdbool.h>

/* Set to false to disable effects for performance */
extern bool snow_enabled;
extern bool clouds_enabled;

/* Initialize visual effects */
void vfx_init(void);

/* Update particle systems (call once per frame) */
void vfx_update(void);

/* Draw background effects (clouds) - call before level */
void vfx_draw_bg(void);

/* Draw foreground effects (snow, particles) - call after level/player */
void vfx_draw_fg(int cam_x, int cam_y);

/* === Smoke/Puff Particles === */
/* Spawn a smoke puff at world position (used for landing, dashing, etc.) */
void vfx_smoke(float x, float y);

/* === Death Particles === */
/* Spawn death burst particles at world position */
void vfx_death_burst(float x, float y);

/* === Dash Particles === */
/* Spawn dash trail particles */
void vfx_dash_particle(float x, float y, int color);

/* === Lifeup Text (1000 points) === */
/* Spawn floating "1000" text at world position (strawberry pickup) */
void vfx_lifeup(float x, float y);

/* === Freeze/Shake System === */
/* Global freeze counter - when > 0, game logic is paused */
extern int vfx_freeze;

/* Global shake counter - when > 0, camera shakes */
extern int vfx_shake;

/* Get camera shake offset (returns random offset when shaking) */
void vfx_get_shake_offset(int *ox, int *oy);

/* === Orb Effect === */
/* Spawn rotating particle ring effect at world position */
void vfx_orb_init(float x, float y);
void vfx_orb_update(void);
void vfx_orb_draw(int cam_x, int cam_y);
bool vfx_orb_active(void);
bool vfx_orb_check_player(int px, int py, int pw, int ph);

#endif /* VFX_H */
