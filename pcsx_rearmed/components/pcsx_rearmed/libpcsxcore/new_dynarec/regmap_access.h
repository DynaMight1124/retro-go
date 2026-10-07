/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2009-2011 Ari64
 * Shared compiler code extracted unchanged from new_dynarec.c.
 */
// get reg suitable for writing
static signed char get_reg_w(const signed char regmap[], signed char r)
{
  return r == 0 ? -1 : get_reg(regmap, r);
}

// get reg as mask bit (1 << hr)
static u_int get_regm(const signed char regmap[], signed char r)
{
  return (1u << (get_reg(regmap, r) & 31)) & ~(1u << 31);
}

static signed char get_reg_temp(const signed char regmap[])
{
  int hr;
  for (hr = 0; hr < HOST_REGS; hr++) {
    if (hr == EXCLUDE_REG)
      continue;
    if (regmap[hr] == (signed char)-1)
      return hr;
  }
  return -1;
}
