/*
    VeMUlator - A Dreamcast Visual Memory Unit emulator for libretro
    Copyright (C) 2018  Mahmoud Jaoune

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _STATE_H_
#define _STATE_H_

#include <string.h>

#include "common.h"

/* One pass over the machine's state, in whichever direction is being asked
   for. Every subsystem has a serialize() that names its fields once, and that
   one list is used to size a state, to write it and to read it back, so a
   field cannot be saved and then not restored.
 *
 * Sizes are pinned rather than left to the compiler: a state written by one
 * build should be readable by another. Doubles are the one exception, and are
 * taken as-is on the assumption of IEEE 754. */
class VE_STATE
{
public:
   enum Mode { COUNT, SAVE, LOAD };

   VE_STATE(Mode _mode, byte *_buffer, size_t _size)
   {
      mode     = _mode;
      buffer   = _buffer;
      size     = _size;
      pos      = 0;
      bad      = false;
   }

   /* True while everything asked for has fitted and nothing has rejected the
      state. A load of one that is the wrong size, or was not written by this
      build, stops rather than reading on. */
   bool ok() const       { return !bad; }
   size_t used() const   { return pos; }

   /* For a caller that has looked at what it read and does not like it. */
   void fail()           { bad = true; }

   void raw(void *p, size_t n)
   {
      if(mode == COUNT)
      {
         pos += n;
         return;
      }

      if(bad || n > size - pos)
      {
         bad = true;
         return;
      }

      if(mode == SAVE)
         memcpy(buffer + pos, p, n);
      else
         memcpy(p, buffer + pos, n);

      pos += n;
   }

   void u8(byte &x)      { raw(&x, 1); }

   void b(bool &x)
   {
      byte v = x ? 1 : 0;
      raw(&v, 1);
      if(mode == LOAD) x = (v != 0);
   }

   void i32(int &x)
   {
      byte v[4];
      pack32(v, (unsigned)x);
      raw(v, 4);
      if(mode == LOAD) x = (int)unpack32(v);
   }

   void u32(unsigned &x)
   {
      byte v[4];
      pack32(v, x);
      raw(v, 4);
      if(mode == LOAD) x = unpack32(v);
   }

   void i64(int64_t &x)
   {
      byte v[8];
      unsigned lo = (unsigned)((uint64_t)x & 0xFFFFFFFFu);
      unsigned hi = (unsigned)(((uint64_t)x >> 16) >> 16);
      pack32(v, lo);
      pack32(v + 4, hi);
      raw(v, 8);
      if(mode == LOAD)
         x = (int64_t)(((uint64_t)unpack32(v + 4) << 16 << 16)
                    | unpack32(v));
   }

   void f64(double &x)   { raw(&x, sizeof(double)); }

private:
   static void pack32(byte *v, unsigned x)
   {
      v[0] = (byte)(x & 0xFF);
      v[1] = (byte)((x >> 8) & 0xFF);
      v[2] = (byte)((x >> 16) & 0xFF);
      v[3] = (byte)((x >> 24) & 0xFF);
   }

   static unsigned unpack32(const byte *v)
   {
      return (unsigned)v[0]
           | ((unsigned)v[1] << 8)
           | ((unsigned)v[2] << 16)
           | ((unsigned)v[3] << 24);
   }

   Mode   mode;
   byte  *buffer;
   size_t size;
   size_t pos;
   bool   bad;
};

#endif // _STATE_H_
