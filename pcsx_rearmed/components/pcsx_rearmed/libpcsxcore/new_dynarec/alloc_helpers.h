/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2009-2011 Ari64
 * Shared compiler code extracted unchanged from new_dynarec.c.
 */
static void dirty_reg(struct regstat *cur, signed char reg)
{
  int hr;
  if (!reg) return;
  hr = get_reg(cur->regmap, reg);
  if (hr >= 0)
    cur->dirty |= 1<<hr;
}

static void set_const(struct regstat *cur, signed char reg, uint32_t value)
{
  int hr;
  if (!reg) return;
  hr = get_reg(cur->regmap, reg);
  if (hr >= 0) {
    cur->isconst |= 1<<hr;
    current_constmap[hr] = value;
  }
}

static void clear_const(struct regstat *cur, signed char reg)
{
  int hr;
  if (!reg) return;
  hr = get_reg(cur->regmap, reg);
  if (hr >= 0)
    cur->isconst &= ~(1<<hr);
}

static int is_const(const struct regstat *cur, signed char reg)
{
  int hr;
  if (reg < 0) return 0;
  if (!reg) return 1;
  hr = get_reg(cur->regmap, reg);
  if (hr >= 0)
    return (cur->isconst>>hr)&1;
  return 0;
}

static uint32_t get_const(const struct regstat *cur, signed char reg)
{
  int hr;
  if (!reg) return 0;
  hr = get_reg(cur->regmap, reg);
  if (hr >= 0)
    return current_constmap[hr];

  SysPrintf("Unknown constant in r%d\n", reg);
  abort();
}

// Least soon needed registers
// Look at the next ten instructions and see which registers
// will be used.  Try not to reallocate these.
static void lsn(struct compile_state *st, u_char hsn[], int i)
{
  int j;
  int b=-1;
  for(j=0;j<9;j++)
  {
    if(i+j >= st->slen) {
      j = st->slen-i-1;
      break;
    }
    if (dops[i+j].is_ujump)
    {
      // Don't go past an unconditonal jump
      j++;
      break;
    }
  }
  for(;j>=0;j--)
  {
    if(dops[i+j].rs1) hsn[dops[i+j].rs1]=j;
    if(dops[i+j].rs2) hsn[dops[i+j].rs2]=j;
    if(dops[i+j].rt1) hsn[dops[i+j].rt1]=j;
    if(dops[i+j].rt2) hsn[dops[i+j].rt2]=j;
    if(dops[i+j].itype==STORE || dops[i+j].itype==STORELR) {
      // Stores can allocate zero
      hsn[dops[i+j].rs1]=j;
      hsn[dops[i+j].rs2]=j;
    }
    if (ram_offset && (dops[i+j].is_load || dops[i+j].is_store))
      hsn[ROREG] = j;
    // On some architectures stores need invc_ptr
    #if defined(HOST_IMM8)
    if (dops[i+j].is_store)
      hsn[INVCP] = j;
    #endif
    if(i+j>=0&&(dops[i+j].itype==UJUMP||dops[i+j].itype==CJUMP||dops[i+j].itype==SJUMP))
    {
      hsn[CCREG]=j;
      b=j;
    }
  }
  if(b>=0)
  {
    if(cinfo[i+b].ba >= st->start && cinfo[i+b].ba < (st->start + st->slen*4))
    {
      // Follow first branch
      int t=(cinfo[i+b].ba-st->start)>>2;
      j=7-b;
      if (t+j >= st->slen) j = st->slen-t-1;
      for(;j>=0;j--)
      {
        if(dops[t+j].rs1) if(hsn[dops[t+j].rs1]>j+b+2) hsn[dops[t+j].rs1]=j+b+2;
        if(dops[t+j].rs2) if(hsn[dops[t+j].rs2]>j+b+2) hsn[dops[t+j].rs2]=j+b+2;
        //if(dops[t+j].rt1) if(hsn[dops[t+j].rt1]>j+b+2) hsn[dops[t+j].rt1]=j+b+2;
        //if(dops[t+j].rt2) if(hsn[dops[t+j].rt2]>j+b+2) hsn[dops[t+j].rt2]=j+b+2;
      }
    }
    // TODO: preferred register based on backward branch
  }
  // Delay slot should preferably not overwrite branch conditions or cycle count
  if (i > 0 && dops[i-1].is_jump) {
    if(dops[i-1].rs1) if(hsn[dops[i-1].rs1]>1) hsn[dops[i-1].rs1]=1;
    if(dops[i-1].rs2) if(hsn[dops[i-1].rs2]>1) hsn[dops[i-1].rs2]=1;
    hsn[CCREG]=1;
    // ...or hash tables
    hsn[RHASH]=1;
    hsn[RHTBL]=1;
  }
  // Coprocessor load/store needs FTEMP, even if not declared
  if(dops[i].itype==C2LS) {
    hsn[FTEMP]=0;
  }
  // Load/store L/R also uses FTEMP as a temporary register
  if (dops[i].itype == LOADLR || dops[i].itype == STORELR) {
    hsn[FTEMP]=0;
  }
  // Don't remove the miniht registers
  if(dops[i].itype==UJUMP||dops[i].itype==RJUMP)
  {
    hsn[RHASH]=0;
    hsn[RHTBL]=0;
  }
}

// We only want to allocate registers if we're going to use them again soon
static int needed_again(struct compile_state *st, int r, int i)
{
  int j;
  int b=-1;
  int rn=10;

  if (i > 0 && dops[i-1].is_ujump)
  {
    if(cinfo[i-1].ba < st->start || cinfo[i-1].ba > st->start + st->slen*4-4)
      return 0; // Don't need any registers if exiting the block
  }
  for(j=0;j<9;j++)
  {
    if(i+j >= st->slen) {
      j = st->slen-i-1;
      break;
    }
    if (dops[i+j].is_ujump)
    {
      // Don't go past an unconditonal jump
      j++;
      break;
    }
    if (dops[i+j].is_exception)
    {
      break;
    }
  }
  for(;j>=1;j--)
  {
    if(dops[i+j].rs1==r) rn=j;
    if(dops[i+j].rs2==r) rn=j;
    if((st->unneeded_reg[i+j]>>r)&1) rn=10;
    if(i+j>=0&&(dops[i+j].itype==UJUMP||dops[i+j].itype==CJUMP||dops[i+j].itype==SJUMP))
    {
      b=j;
    }
  }
  if(rn<10) return 1;
  (void)b;
  return 0;
}

// Try to match register allocations at the end of a loop with those
// at the beginning
static int loop_reg(struct compile_state *st, int i, int r, int hr)
{
  int j,k;
  for(j=0;j<9;j++)
  {
    if(i+j >= st->slen) {
      j = st->slen-i-1;
      break;
    }
    if (dops[i+j].is_ujump)
    {
      // Don't go past an unconditonal jump
      j++;
      break;
    }
  }
  k=0;
  if(i>0){
    if(dops[i-1].itype==UJUMP||dops[i-1].itype==CJUMP||dops[i-1].itype==SJUMP)
      k--;
  }
  for(;k<j;k++)
  {
    assert(r < 64);
    if((st->unneeded_reg[i+k]>>r)&1) return hr;
    if(i+k>=0&&(dops[i+k].itype==UJUMP||dops[i+k].itype==CJUMP||dops[i+k].itype==SJUMP))
    {
      if(cinfo[i+k].ba >= st->start && cinfo[i+k].ba < (st->start+i*4))
      {
        int t=(cinfo[i+k].ba - st->start)>>2;
        int reg=get_reg(regs[t].regmap_entry,r);
        if(reg>=0) return reg;
        //reg=get_reg(regs[t+1].regmap_entry,r);
        //if(reg>=0) return reg;
      }
    }
  }
  return hr;
}
