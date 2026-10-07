/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2009-2011 Ari64
 * Shared compiler code extracted unchanged from new_dynarec.c.
 */
static signed char get_reg(const signed char regmap[], signed char r)
{
  int hr;
  for (hr = 0; hr < HOST_REGS; hr++) {
    if (hr == EXCLUDE_REG)
      continue;
    if (regmap[hr] == r)
      return hr;
  }
  return -1;
}
