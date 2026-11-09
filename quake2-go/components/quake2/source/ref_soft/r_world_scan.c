#include "r_local.h"
#include "r_world.h"
#include "esp_attr.h"
#define WRITEPDEST(i) { pdest[i] = *(pbase + (s >> 16) + (t >> 16) * st->cachewidth); s+=sstep; t+=tstep;}
void IRAM_ATTR D_WorldSpans(espan_t *pspan, const world_span_state_t *st, int rows)
{
int          count, spancount;
byte         *pbase, *pdest;
fixed16_t    s, t, snext, tnext, sstep, tstep;
float        sdivz, tdivz, zi, z, du, dv, spancountminus1;
float        sdivzstepu, tdivzstepu, zistepu;



	sstep = 0;
	tstep = 0;

	pbase = (byte *)st->cacheblock;
	sdivzstepu = st->d_sdivzstepu * 16;
	tdivzstepu = st->d_tdivzstepu * 16;
	zistepu = st->d_zistepu * 16;

	do {
    if (rows && ((pspan->v & 1) != rows - 1)) continue;
		pdest = (byte *)((byte *)d_viewbuffer + (r_screenwidth * pspan->v) + pspan->u);
		count = pspan->count >> 4;

		spancount = pspan->count % 16;


		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = st->d_sdivzorigin + dv*st->d_sdivzstepv + du*st->d_sdivzstepu;
		tdivz = st->d_tdivzorigin + dv*st->d_tdivzstepv + du*st->d_tdivzstepu;
		zi = st->d_ziorigin + dv*st->d_zistepv + du*st->d_zistepu;
		z = (float)0x10000 / zi;

		s = (int)(sdivz * z) + st->sadjust;
		if (s < 0) s = 0;
		else if (s > st->bbextents) s = st->bbextents;

		t = (int)(tdivz * z) + st->tadjust;
		if (t < 0) t = 0;
		else if (t > st->bbextentt) t = st->bbextentt;

		while (count-- > 0)
		{
			sdivz += sdivzstepu;
			tdivz += tdivzstepu;
			zi += zistepu;
			z = (float)0x10000 / zi;

			snext = (int)(sdivz * z) + st->sadjust;
			if (snext < 16) snext = 16;
			else if (snext > st->bbextents) snext = st->bbextents;

			tnext = (int)(tdivz * z) + st->tadjust;
			if (tnext < 16) tnext = 16;
			else if (tnext > st->bbextentt) tnext = st->bbextentt;

			sstep = (snext - s) >> 4;
			tstep = (tnext - t) >> 4;
			pdest += 16;

			WRITEPDEST(-16);
			WRITEPDEST(-15);
			WRITEPDEST(-14);
			WRITEPDEST(-13);
			WRITEPDEST(-12);
			WRITEPDEST(-11);
			WRITEPDEST(-10);
			WRITEPDEST(-9);
			WRITEPDEST(-8);
			WRITEPDEST(-7);
			WRITEPDEST(-6);
			WRITEPDEST(-5);
			WRITEPDEST(-4);
			WRITEPDEST(-3);
			WRITEPDEST(-2);
			WRITEPDEST(-1);

			s = snext;
			t = tnext;
		}
		if (spancount > 0)
		{
			spancountminus1 = (float)(spancount - 1);
			sdivz += st->d_sdivzstepu * spancountminus1;
			tdivz += st->d_tdivzstepu * spancountminus1;
			zi += st->d_zistepu * spancountminus1;
			z = (float)0x10000 / zi;

			snext = (int)(sdivz * z) + st->sadjust;
			if (snext < 16) snext = 16;
			else if (snext > st->bbextents) snext = st->bbextents;

			tnext = (int)(tdivz * z) + st->tadjust;
			if (tnext < 16) tnext = 16;
			else if (tnext > st->bbextentt) tnext = st->bbextentt;

			if (spancount > 1)
			{
				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}

			pdest += spancount;

			switch (spancount)
			{
			case 16:
				WRITEPDEST(-16);
			case 15:
				WRITEPDEST(-15);
			case 14:
				WRITEPDEST(-14);
			case 13:
				WRITEPDEST(-13);
			case 12:
				WRITEPDEST(-12);
			case 11:
				WRITEPDEST(-11);
			case 10:
				WRITEPDEST(-10);
			case  9:
				WRITEPDEST(-9);
			case  8:
				WRITEPDEST(-8);
			case  7:
				WRITEPDEST(-7);
			case  6:
				WRITEPDEST(-6);
			case  5:
				WRITEPDEST(-5);
			case  4:
				WRITEPDEST(-4);
			case  3:
				WRITEPDEST(-3);
			case  2:
				WRITEPDEST(-2);
			case  1:
				WRITEPDEST(-1);
				break;
			}
		}
	} while ((pspan = pspan->pnext) != NULL);
}
void IRAM_ATTR D_WorldZ(espan_t *pspan, const world_span_state_t *st, int rows) {
  int count, quadcount, doublecount, izistep;
  int izi;
  short *pdest;
  unsigned ltemp;
  float zi;
  float du, dv;


  const int zwidth = d_zwidth;
  short *const pzbuffer = d_pzbuffer;
  const float zistepu = st->d_zistepu, zistepv = st->d_zistepv, ziorigin = st->d_ziorigin;

  izistep = (int)(zistepu * 0x8000 * 0x10000);

  do {
    if (rows && ((pspan->v & 1) != rows - 1)) continue;
    pdest = pzbuffer + (zwidth * pspan->v) + pspan->u;
    count = pspan->count;

    du = (float)pspan->u;
    dv = (float)pspan->v;
    zi = ziorigin + dv * zistepv + du * zistepu;
    izi = (int)(zi * 0x8000 * 0x10000);


    if ((size_t)pdest & 0x02) {
      *pdest++ = (short)(izi >> 16);
      izi += izistep;
      count--;
    }


    if ((quadcount = count >> 3) > 0) {
      do {
        ltemp  = (unsigned)izi >> 16;
        izi += izistep;
        ltemp |= (unsigned)izi & 0xFFFF0000u;
        izi += izistep;
        ((unsigned *)pdest)[0] = ltemp;

        ltemp  = (unsigned)izi >> 16;
        izi += izistep;
        ltemp |= (unsigned)izi & 0xFFFF0000u;
        izi += izistep;
        ((unsigned *)pdest)[1] = ltemp;

        ltemp  = (unsigned)izi >> 16;
        izi += izistep;
        ltemp |= (unsigned)izi & 0xFFFF0000u;
        izi += izistep;
        ((unsigned *)pdest)[2] = ltemp;

        ltemp  = (unsigned)izi >> 16;
        izi += izistep;
        ltemp |= (unsigned)izi & 0xFFFF0000u;
        izi += izistep;
        ((unsigned *)pdest)[3] = ltemp;

        pdest += 8;
      } while (--quadcount > 0);
    }

    count &= 7;


    if ((doublecount = count >> 1) > 0) {
      do {
        ltemp  = (unsigned)izi >> 16;
        izi += izistep;
        ltemp |= (unsigned)izi & 0xFFFF0000u;
        izi += izistep;
        *(unsigned *)pdest = ltemp;
        pdest += 2;
      } while (--doublecount > 0);
    }

    if (count & 1)
      *pdest = (short)(izi >> 16);

  } while ((pspan = pspan->pnext) != NULL);
}
void D_WorldTurb(espan_t *pspan, const world_span_state_t *st, int rows) {
  unsigned char *r_turb_pbase, *r_turb_pdest;
  fixed16_t r_turb_s, r_turb_t, r_turb_sstep, r_turb_tstep;
  int *r_turb_turb; int r_turb_spancount;
  int count;
  fixed16_t snext, tnext;
  float sdivz, tdivz, zi, z, du, dv, spancountminus1;
  float sdivz16stepu, tdivz16stepu, zi16stepu;

  r_turb_turb = sintable + ((int)(r_newrefdef.time * SPEED) & (CYCLE - 1));

  r_turb_sstep = 0;
  r_turb_tstep = 0;

  r_turb_pbase = (unsigned char *)st->cacheblock;

  sdivz16stepu = st->d_sdivzstepu * 16;
  tdivz16stepu = st->d_tdivzstepu * 16;
  zi16stepu = st->d_zistepu * 16;

  do {
    if (rows && ((pspan->v & 1) != rows - 1)) continue;
    r_turb_pdest = (unsigned char *)((byte *)d_viewbuffer +
                                     (r_screenwidth * pspan->v) + pspan->u);

    count = pspan->count;


    du = (float)pspan->u;
    dv = (float)pspan->v;

    sdivz = st->d_sdivzorigin + dv * st->d_sdivzstepv + du * st->d_sdivzstepu;
    tdivz = st->d_tdivzorigin + dv * st->d_tdivzstepv + du * st->d_tdivzstepu;
    zi = st->d_ziorigin + dv * st->d_zistepv + du * st->d_zistepu;
    z = (float)0x10000 / zi;

    r_turb_s = (int)(sdivz * z) + st->sadjust;
    if (r_turb_s > st->bbextents)
      r_turb_s = st->bbextents;
    else if (r_turb_s < 0)
      r_turb_s = 0;

    r_turb_t = (int)(tdivz * z) + st->tadjust;
    if (r_turb_t > st->bbextentt)
      r_turb_t = st->bbextentt;
    else if (r_turb_t < 0)
      r_turb_t = 0;

    do {

      if (count >= 16)
        r_turb_spancount = 16;
      else
        r_turb_spancount = count;

      count -= r_turb_spancount;

      if (count) {


        sdivz += sdivz16stepu;
        tdivz += tdivz16stepu;
        zi += zi16stepu;
        z = (float)0x10000 / zi;

        snext = (int)(sdivz * z) + st->sadjust;
        if (snext > st->bbextents)
          snext = st->bbextents;
        else if (snext < 16)
          snext = 16;



        tnext = (int)(tdivz * z) + st->tadjust;
        if (tnext > st->bbextentt)
          tnext = st->bbextentt;
        else if (tnext < 16)
          tnext = 16;

        r_turb_sstep = (snext - r_turb_s) >> 4;
        r_turb_tstep = (tnext - r_turb_t) >> 4;
      } else {




        spancountminus1 = (float)(r_turb_spancount - 1);
        sdivz += st->d_sdivzstepu * spancountminus1;
        tdivz += st->d_tdivzstepu * spancountminus1;
        zi += st->d_zistepu * spancountminus1;
        z = (float)0x10000 / zi;
        snext = (int)(sdivz * z) + st->sadjust;
        if (snext > st->bbextents)
          snext = st->bbextents;
        else if (snext < 16)
          snext = 16;



        tnext = (int)(tdivz * z) + st->tadjust;
        if (tnext > st->bbextentt)
          tnext = st->bbextentt;
        else if (tnext < 16)
          tnext = 16;

        if (r_turb_spancount > 1) {
          r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
          r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);
        }
      }

      r_turb_s = r_turb_s & ((CYCLE << 16) - 1);
      r_turb_t = r_turb_t & ((CYCLE << 16) - 1);

      {
  int sturb, tturb;

  do {
    sturb =
        ((r_turb_s + r_turb_turb[(r_turb_t >> 16) & (CYCLE - 1)]) >> 16) & 63;
    tturb =
        ((r_turb_t + r_turb_turb[(r_turb_s >> 16) & (CYCLE - 1)]) >> 16) & 63;
    *r_turb_pdest++ = *(r_turb_pbase + (tturb << 6) + sturb);
    r_turb_s += r_turb_sstep;
    r_turb_t += r_turb_tstep;
  } while (--r_turb_spancount > 0);}

      r_turb_s = snext;
      r_turb_t = tnext;

    } while (count > 0);

  } while ((pspan = pspan->pnext) != NULL);
}
void D_WorldFlow(espan_t *pspan, const world_span_state_t *st, int rows) {
  unsigned char *r_turb_pbase, *r_turb_pdest;
  fixed16_t r_turb_s, r_turb_t, r_turb_sstep, r_turb_tstep;
  int *r_turb_turb; int r_turb_spancount;
  int count;
  fixed16_t snext, tnext;
  float sdivz, tdivz, zi, z, du, dv, spancountminus1;
  float sdivz16stepu, tdivz16stepu, zi16stepu;


  r_turb_turb = blanktable;

  r_turb_sstep = 0;
  r_turb_tstep = 0;

  r_turb_pbase = (unsigned char *)st->cacheblock;

  sdivz16stepu = st->d_sdivzstepu * 16;
  tdivz16stepu = st->d_tdivzstepu * 16;
  zi16stepu = st->d_zistepu * 16;

  do {
    if (rows && ((pspan->v & 1) != rows - 1)) continue;
    r_turb_pdest = (unsigned char *)((byte *)d_viewbuffer +
                                     (r_screenwidth * pspan->v) + pspan->u);

    count = pspan->count;


    du = (float)pspan->u;
    dv = (float)pspan->v;

    sdivz = st->d_sdivzorigin + dv * st->d_sdivzstepv + du * st->d_sdivzstepu;
    tdivz = st->d_tdivzorigin + dv * st->d_tdivzstepv + du * st->d_tdivzstepu;
    zi = st->d_ziorigin + dv * st->d_zistepv + du * st->d_zistepu;
    z = (float)0x10000 / zi;

    r_turb_s = (int)(sdivz * z) + st->sadjust;
    if (r_turb_s > st->bbextents)
      r_turb_s = st->bbextents;
    else if (r_turb_s < 0)
      r_turb_s = 0;

    r_turb_t = (int)(tdivz * z) + st->tadjust;
    if (r_turb_t > st->bbextentt)
      r_turb_t = st->bbextentt;
    else if (r_turb_t < 0)
      r_turb_t = 0;

    do {

      if (count >= 16)
        r_turb_spancount = 16;
      else
        r_turb_spancount = count;

      count -= r_turb_spancount;

      if (count) {


        sdivz += sdivz16stepu;
        tdivz += tdivz16stepu;
        zi += zi16stepu;
        z = (float)0x10000 / zi;

        snext = (int)(sdivz * z) + st->sadjust;
        if (snext > st->bbextents)
          snext = st->bbextents;
        else if (snext < 16)
          snext = 16;



        tnext = (int)(tdivz * z) + st->tadjust;
        if (tnext > st->bbextentt)
          tnext = st->bbextentt;
        else if (tnext < 16)
          tnext = 16;

        r_turb_sstep = (snext - r_turb_s) >> 4;
        r_turb_tstep = (tnext - r_turb_t) >> 4;
      } else {




        spancountminus1 = (float)(r_turb_spancount - 1);
        sdivz += st->d_sdivzstepu * spancountminus1;
        tdivz += st->d_tdivzstepu * spancountminus1;
        zi += st->d_zistepu * spancountminus1;
        z = (float)0x10000 / zi;
        snext = (int)(sdivz * z) + st->sadjust;
        if (snext > st->bbextents)
          snext = st->bbextents;
        else if (snext < 16)
          snext = 16;



        tnext = (int)(tdivz * z) + st->tadjust;
        if (tnext > st->bbextentt)
          tnext = st->bbextentt;
        else if (tnext < 16)
          tnext = 16;

        if (r_turb_spancount > 1) {
          r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
          r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);
        }
      }

      r_turb_s = r_turb_s & ((CYCLE << 16) - 1);
      r_turb_t = r_turb_t & ((CYCLE << 16) - 1);

      {
  int sturb, tturb;

  do {
    sturb =
        ((r_turb_s + r_turb_turb[(r_turb_t >> 16) & (CYCLE - 1)]) >> 16) & 63;
    tturb =
        ((r_turb_t + r_turb_turb[(r_turb_s >> 16) & (CYCLE - 1)]) >> 16) & 63;
    *r_turb_pdest++ = *(r_turb_pbase + (tturb << 6) + sturb);
    r_turb_s += r_turb_sstep;
    r_turb_t += r_turb_tstep;
  } while (--r_turb_spancount > 0);}

      r_turb_s = snext;
      r_turb_t = tnext;

    } while (count > 0);

  } while ((pspan = pspan->pnext) != NULL);
}
