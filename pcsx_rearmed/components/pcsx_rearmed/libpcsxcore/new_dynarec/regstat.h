/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2009-2011 Ari64
 * Shared compiler code extracted unchanged from new_dynarec.c.
 */
struct regstat
{
  signed char regmap_entry[HOST_REGS];
  signed char regmap[HOST_REGS];
  u_int wasdirty;
  u_int dirty;
  u_int wasconst;                // before; for example 'lw r2, (r2)' wasconst is true
  u_int isconst;                 //  ... but isconst is false when r2 is known (hr)
  u_int loadedconst;             // host regs that have constants loaded
  u_int noevict;                 // can't evict this hr (alloced by current op)
  //u_int waswritten;              // MIPS regs that were used as store base before
  uint64_t u;
};
