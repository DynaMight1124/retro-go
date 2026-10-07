/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2016  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#ifndef __GTE_DIVIDER_H__
#define __GTE_DIVIDER_H__

u32 DIVIDE(u16 n, u16 d);

#if defined(CONFIG_IDF_TARGET_ESP32P4)
extern const u8 gte_divider_table[257];

static inline __attribute__((always_inline)) int gte_clz16_p4(u16 value)
{
	int shift = 0;

	/* The P4 baseline ISA has no scalar CLZ instruction, so __builtin_clz()
	 * calls the ROM libgcc helper. The divider has already excluded zero;
	 * normalize its 16-bit denominator directly in four short steps. */
	if (!(value & 0xff00u)) value <<= 8, shift += 8;
	if (!(value & 0xf000u)) value <<= 4, shift += 4;
	if (!(value & 0xc000u)) value <<= 2, shift += 2;
	if (!(value & 0x8000u)) shift++;
	return shift;
}

static inline __attribute__((always_inline)) u32 gte_divide_p4(u16 numerator,
		u16 denominator)
{
	if (numerator < denominator * 2) {
		int shift = gte_clz16_p4(denominator);
		int r1 = (denominator << shift) & 0x7fff;
		int r2 = gte_divider_table[(r1 + 0x40) >> 7] + 0x101;
		int r3 = ((0x80 - r2 * (r1 + 0x8000)) >> 8) & 0x1ffff;
		u32 reciprocal = (r2 * r3 + 0x80) >> 8;

		return ((u64)reciprocal * (numerator << shift) + 0x8000) >> 16;
	}

	return 0xffffffff;
}
#endif

#endif /* __GTE_DIVIDER_H__ */
