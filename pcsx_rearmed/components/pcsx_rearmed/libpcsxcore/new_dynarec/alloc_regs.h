/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2009-2011 Ari64
 * Shared compiler code extracted unchanged from new_dynarec.c.
 */
static void alloc_set(struct regstat *cur, int reg, int hr)
{
  cur->regmap[hr] = reg;
  cur->dirty &= ~(1u << hr);
  cur->isconst &= ~(1u << hr);
  cur->noevict |= 1u << hr;
}

static void evict_alloc_reg(struct compile_state *st, struct regstat *cur,
  int i, int reg, int preferred_hr)
{
  u_char hsn[MAXREG+1];
  int j, r, hr;
  memset(hsn, 10, sizeof(hsn));
  lsn(st, hsn, i);
  //printf("hsn(%x): %d %d %d %d %d %d %d\n",start+i*4,hsn[cur->regmap[0]&63],hsn[cur->regmap[1]&63],hsn[cur->regmap[2]&63],hsn[cur->regmap[3]&63],hsn[cur->regmap[5]&63],hsn[cur->regmap[6]&63],hsn[cur->regmap[7]&63]);
  if(i>0) {
    // Don't evict the cycle count at entry points, otherwise the entry
    // stub will have to write it.
    if(dops[i].bt&&hsn[CCREG]>2) hsn[CCREG]=2;
    if (i>1 && hsn[CCREG] > 2 && dops[i-2].is_jump) hsn[CCREG]=2;
    for(j=10;j>=3;j--)
    {
      // Alloc preferred register if available
      if (!((cur->noevict >> preferred_hr) & 1)
          && hsn[cur->regmap[preferred_hr]] == j)
      {
        alloc_set(cur, reg, preferred_hr);
        return;
      }
      for(r=1;r<=MAXREG;r++)
      {
        if(hsn[r]==j&&r!=dops[i-1].rs1&&r!=dops[i-1].rs2&&r!=dops[i-1].rt1&&r!=dops[i-1].rt2) {
          for(hr=0;hr<HOST_REGS;hr++) {
            if (hr == EXCLUDE_REG || ((cur->noevict >> hr) & 1))
              continue;
            if(hr!=HOST_CCREG||j<hsn[CCREG]) {
              if(cur->regmap[hr]==r) {
                alloc_set(cur, reg, hr);
                return;
              }
            }
          }
        }
      }
    }
  }
  for(j=10;j>=0;j--)
  {
    for(r=1;r<=MAXREG;r++)
    {
      if(hsn[r]==j) {
        for(hr=0;hr<HOST_REGS;hr++) {
          if (hr == EXCLUDE_REG || ((cur->noevict >> hr) & 1))
            continue;
          if(cur->regmap[hr]==r) {
            alloc_set(cur, reg, hr);
            return;
          }
        }
      }
    }
  }
  SysPrintf("This shouldn't happen (evict_alloc_reg)\n");
  abort();
}

// Note: registers are allocated clean (unmodified state)
// if you intend to modify the register, you must call dirty_reg().
static void alloc_reg(struct compile_state *st, struct regstat *cur,
  int i, signed char reg)
{
  int r,hr;
  int preferred_reg = PREFERRED_REG_FIRST
    + reg % (PREFERRED_REG_LAST - PREFERRED_REG_FIRST + 1);
  if (reg == CCREG) preferred_reg = HOST_CCREG;
  if (reg == PTEMP || reg == FTEMP) preferred_reg = 12;
  assert(PREFERRED_REG_FIRST != EXCLUDE_REG && EXCLUDE_REG != HOST_REGS);
  assert(reg >= 0);

  // Don't allocate unused registers
  if((cur->u>>reg)&1) return;

  // see if it's already allocated
  if ((hr = get_reg(cur->regmap, reg)) >= 0) {
    cur->noevict |= 1u << hr;
    return;
  }

  // Keep the same mapping if the register was already allocated in a loop
  preferred_reg = loop_reg(st,i,reg,preferred_reg);

  // Try to allocate the preferred register
  if (cur->regmap[preferred_reg] == -1) {
    alloc_set(cur, reg, preferred_reg);
    return;
  }
  r=cur->regmap[preferred_reg];
  assert(r < 64);
  if((cur->u>>r)&1) {
    alloc_set(cur, reg, preferred_reg);
    return;
  }

  // Clear any unneeded registers
  // We try to keep the mapping consistent, if possible, because it
  // makes branches easier (especially loops).  So we try to allocate
  // first (see above) before removing old mappings.  If this is not
  // possible then go ahead and clear out the registers that are no
  // longer needed.
  for(hr=0;hr<HOST_REGS;hr++)
  {
    r=cur->regmap[hr];
    if(r>=0) {
      assert(r < 64);
      if((cur->u>>r)&1) {cur->regmap[hr]=-1;break;}
    }
  }

  // Try to allocate any available register, but prefer
  // registers that have not been used recently.
  if (i > 0) {
    for (hr = PREFERRED_REG_FIRST; ; ) {
      if (cur->regmap[hr] < 0) {
        int oldreg = regs[i-1].regmap[hr];
        if (oldreg < 0 || (oldreg != dops[i-1].rs1 && oldreg != dops[i-1].rs2
             && oldreg != dops[i-1].rt1 && oldreg != dops[i-1].rt2))
        {
          alloc_set(cur, reg, hr);
          return;
        }
      }
      hr++;
      if (hr == EXCLUDE_REG)
        hr++;
      if (hr == HOST_REGS)
        hr = 0;
      if (hr == PREFERRED_REG_FIRST)
        break;
    }
  }

  // Try to allocate any available register
  for (hr = PREFERRED_REG_FIRST; ; ) {
    if (cur->regmap[hr] < 0) {
      alloc_set(cur, reg, hr);
      return;
    }
    hr++;
    if (hr == EXCLUDE_REG)
      hr++;
    if (hr == HOST_REGS)
      hr = 0;
    if (hr == PREFERRED_REG_FIRST)
      break;
  }

  // Ok, now we have to evict someone
  // Pick a register we hopefully won't need soon
  evict_alloc_reg(st, cur, i, reg, preferred_reg);
}

// Allocate a temporary register.  This is done without regard to
// dirty status or whether the register we request is on the unneeded list
// Note: This will only allocate one register, even if called multiple times
static void alloc_reg_temp(struct compile_state *st, struct regstat *cur,
  int i, signed char reg)
{
  int r,hr;

  // see if it's already allocated
  for (hr = 0; hr < HOST_REGS; hr++)
  {
    if (hr != EXCLUDE_REG && cur->regmap[hr] == reg) {
      cur->noevict |= 1u << hr;
      return;
    }
  }

  // Try to allocate any available register
  for(hr=HOST_REGS-1;hr>=0;hr--) {
    if(hr!=EXCLUDE_REG&&cur->regmap[hr]==-1) {
      alloc_set(cur, reg, hr);
      return;
    }
  }

  // Find an unneeded register
  for(hr=HOST_REGS-1;hr>=0;hr--)
  {
    r=cur->regmap[hr];
    if(r>=0) {
      assert(r < 64);
      if((cur->u>>r)&1) {
        if(i==0||((st->unneeded_reg[i-1]>>r)&1)) {
          alloc_set(cur, reg, hr);
          return;
        }
      }
    }
  }

  // Ok, now we have to evict someone
  // Pick a register we hopefully won't need soon
  evict_alloc_reg(st, cur, i, reg, 0);
}
