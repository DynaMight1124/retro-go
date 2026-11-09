#pragma once
typedef struct {
  float d_sdivzstepu;
  float d_tdivzstepu;
  float d_zistepu;
  float d_sdivzstepv;
  float d_tdivzstepv;
  float d_zistepv;
  float d_sdivzorigin;
  float d_tdivzorigin;
  float d_ziorigin;
  fixed16_t sadjust;
  fixed16_t tadjust;
  fixed16_t bbextents;
  fixed16_t bbextentt;
  int cachewidth;
  pixel_t * cacheblock;
} world_span_state_t;
void D_WorldSpans(espan_t *, const world_span_state_t *, int);
void D_WorldZ(espan_t *, const world_span_state_t *, int);
void D_WorldTurb(espan_t *, const world_span_state_t *, int);
void D_WorldFlow(espan_t *, const world_span_state_t *, int);
