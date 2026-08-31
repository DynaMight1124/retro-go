//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2020 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#pragma once

#include <stdint.h>    // int32_t, int64_t, …
#include <cmath>       // std::abs
#include <algorithm>   // std::min
#include <type_traits> // std::enable_if

namespace z8
{

struct fix32
{

    inline fix32() = default;

    // Convert from/to double
    inline fix32(float d)
      : m_bits(int32_t(int64_t(d * 65536.0)))
    {}

    // Direct double ctor wins identity-rank over float + integer ctors so
    // functional casts like `(lua_Number)HUGE_VAL` and `(lua_Number)10.0`
    // resolve to a single best candidate instead of being ambiguous.
    // This does not collide with any existing ctor (only `fix32(float)`
    // was previously available for double input via standard conversion).
    inline fix32(double d)
      : m_bits(int32_t(int64_t(d * 65536.0)))
    {}

    // NOTE: deliberately NOT adding `inline fix32(int x)` because the
    // existing `fix32(int32_t x)` already handles the common case where
    // `int` is a typedef for `int32_t` (ESP32, x86-64, most 32-bit GCC
    // targets) — adding another ctor with the same effective parameter
    // type would be a redefinition. The `#ifdef __3DS__` guard on the
    // existing `fix32(int)` ctor exists for that platform specifically.
    // On platforms where `int` ≠ `int32_t` (rare for embedded GCC, but
    // possible under exotic toolchains), RAND_MAX casts in lmathlib.cpp
    // would still work via the narrowing integer ctors at conversion
    // rank; the existing pico8 cart code paths don't reach this case.

    inline operator float() const
    {
        return float(m_bits) * (1.0 / 65536.0);
    }

    // Conversions up to int16_t are safe.
    inline fix32(int8_t x)  : m_bits(int32_t(x << 16)) {}
    inline fix32(uint8_t x) : m_bits(int32_t(x << 16)) {}
    inline fix32(int16_t x) : m_bits(int32_t(x << 16)) {}
    inline fix32(int16_t x, uint16_t y) : m_bits(int32_t(x << 16) | int32_t(y)) {}

    // Anything above int16_t is risky because of precision loss, but Lua
    // does too many implicit conversions from int that we can’t mark this
    // one as explicit.
    inline fix32(int32_t x)  : m_bits(int32_t(x << 16)) {}
#ifdef __3DS__
    inline fix32(int x)  : m_bits(int32_t(x << 16)) {}
#endif

    inline explicit fix32(uint16_t x) : m_bits(int32_t(x << 16)) {}
    inline explicit fix32(uint32_t x) : m_bits(int32_t(x << 16)) {}
    inline explicit fix32(int64_t x)  : m_bits(int32_t(x << 16)) {}
    inline explicit fix32(uint64_t x) : m_bits(int32_t(x << 16)) {}

    // Support for long and unsigned long when it is a distinct
    // type from the standard int*_t types, e.g. on Windows.
    template<typename T,
             typename std::enable_if<(std::is_same<T, long>::value ||
                                      std::is_same<T, unsigned long>::value) &&
                                     !std::is_same<T, int32_t>::value &&
                                     !std::is_same<T, uint32_t>::value &&
                                     !std::is_same<T, int64_t>::value &&
                                     !std::is_same<T, uint64_t>::value>::type *...>
    inline explicit fix32(T x) : m_bits(int32_t(x << 16)) {}

    // Explicit casts are all allowed
    inline explicit operator int8_t()   const { return m_bits >> 16; }
    inline explicit operator uint8_t()  const { return m_bits >> 16; }
    inline explicit operator int16_t()  const { return m_bits >> 16; }
    inline explicit operator uint16_t() const { return m_bits >> 16; }
    inline explicit operator int32_t()  const { return m_bits >> 16; }
    inline explicit operator uint32_t() const { return m_bits >> 16; }
    inline explicit operator int64_t()  const { return m_bits >> 16; }
    inline explicit operator uint64_t() const { return m_bits >> 16; }

    // Additional casts for long and unsigned long on architectures where
    // these are not the same types as their cstdint equivalents.
    template<typename T,
             typename std::enable_if<(std::is_same<T, long>::value ||
                                      std::is_same<T, unsigned long>::value) &&
                                     !std::is_same<T, int32_t>::value &&
                                     !std::is_same<T, uint32_t>::value &&
                                     !std::is_same<T, int64_t>::value &&
                                     !std::is_same<T, uint64_t>::value>::type *...>
    inline explicit operator T() const { return T(m_bits >> 16); }

    // Directly initialise bits
    static inline fix32 frombits(int32_t x)
    {
        fix32 ret; ret.m_bits = x; return ret;
    }

    inline int32_t bits() const { return m_bits; }

    // Comparisons
    inline explicit operator bool()    const { return bool(m_bits); }
    inline bool operator ==(fix32 x)   const { return m_bits == x.m_bits; }
    inline bool operator !=(fix32 x)   const { return m_bits != x.m_bits; }
    inline bool operator  <(int16_t x) const { return m_bits  < (int32_t(x << 16)); }
    inline bool operator  <(fix32 x)   const { return m_bits  < x.m_bits; }
    inline bool operator  >(fix32 x)   const { return m_bits  > x.m_bits; }
    inline bool operator  >(int16_t x) const { return m_bits  > (int32_t(x << 16)); }
    inline bool operator <=(fix32 x)   const { return m_bits <= x.m_bits; }
    inline bool operator >=(fix32 x)   const { return m_bits >= x.m_bits; }

    // Increments
    inline fix32& operator ++() { m_bits += 0x10000; return *this; }
    inline fix32& operator --() { m_bits -= 0x10000; return *this; }
    inline fix32 operator ++(int) { fix32 ret = *this; ++*this; return ret; }
    inline fix32 operator --(int) { fix32 ret = *this; --*this; return ret; }

    // Math operations
    inline fix32 const &operator +() const { return *this; }
    inline fix32 operator -() const { return frombits(-m_bits); }
    inline fix32 operator ~() const { return frombits(~m_bits); }

    inline fix32 operator +(fix32 x) const   { return frombits(m_bits + x.m_bits); }
    inline fix32 operator -(fix32 x) const   { return frombits(m_bits - x.m_bits); }
//    inline fix32 operator -(int16_t x) const { return *this - fix32(x); }
    inline fix32 operator &(fix32 x) const   { return frombits(m_bits & x.m_bits); }
    inline fix32 operator |(fix32 x) const   { return frombits(m_bits | x.m_bits); }
    inline fix32 operator ^(fix32 x) const   { return frombits(m_bits ^ x.m_bits); }

    inline fix32 operator *(int32_t x) const
    {
        return frombits(int64_t(m_bits) * x);
    }

    inline fix32 operator *(fix32 x) const
    {
        return frombits(int64_t(m_bits) * x.m_bits >> 16);
    }

    inline fix32 operator /(fix32 x) const
    {
        // This special case ensures 0x8000/0x1 = 0x8000, not 0x8000.0001
        if (x.m_bits == 0x10000)
            return *this;

        if (x.m_bits)
        {
            using std::abs;
            int64_t result = int64_t(m_bits) * 0x10000 / x.m_bits;
            if (abs(result) <= 0x7fffffffu)
                return frombits(int32_t(result));
        }

        // Return 0x8000.0001 (not 0x8000.0000) for -Inf, just like PICO-8
        return frombits((m_bits ^ x.m_bits) >= 0 ? 0x7fffffffu : 0x80000001u);
    }

    inline fix32 operator %(fix32 x) const
    {
        // PICO-8 always returns positive values
        x = abs(x);
        int32_t result = x ? m_bits % x.m_bits : m_bits;
        return frombits(result >= 0 ? result : result + x.m_bits);
    }

    inline fix32 operator <<(int y) const
    {
        // If y is negative, use lshr() instead.
        return y < 0 ? lshr(*this, -y) : frombits(y >= 32 ? 0 : bits() << y);
    }

    inline fix32 operator >>(int y) const
    {
        using std::min;
        // If y is negative, use << instead.
        return y < 0 ? *this << -y : frombits(bits() >> min(y, 31));
    }

    inline fix32& operator +=(fix32 x) { return *this = *this + x; }
    inline fix32& operator -=(fix32 x) { return *this = *this - x; }
    inline fix32& operator &=(fix32 x) { return *this = *this & x; }
    inline fix32& operator |=(fix32 x) { return *this = *this | x; }
    inline fix32& operator ^=(fix32 x) { return *this = *this ^ x; }
    inline fix32& operator *=(fix32 x) { return *this = *this * x; }
    inline fix32& operator /=(fix32 x) { return *this = *this / x; }
    inline fix32& operator %=(fix32 x) { return *this = *this % x; }

    // Free functions
    static inline fix32 abs(fix32 a) { return a.m_bits > 0 ? a : -a; }
    static inline fix32 fabs(fix32 a) { return a.m_bits >= 0 ? a : -a; }
    static inline fix32 min(fix32 a, fix32 b) { return a < b ? a : b; }
    static inline fix32 max(fix32 a, fix32 b) { return a > b ? a : b; }

    static inline fix32 ceil(fix32 x) { return -floor(-x); }
    static inline fix32 decimals(fix32 x) { return frombits(x.m_bits & 0x0000ffff); }
    static inline fix32 floor(fix32 x) { return frombits(x.m_bits & 0xffff0000); }

    static fix32 pow(fix32 x, fix32 y) { return fix32(std::pow(float(x), float(y))); }

    static void to_string(fix32 x, char* buf) {
        uint32_t dec = (uint16_t)(x.bits()); // keep only last 16 bits
        for(uint8_t i = 0; i<16; i++) {
            dec *= 10;
            buf[i] = '0' + (dec >> 16); // keep the integer part only
            dec &= 0xffff;
            if(dec==0) {
                buf[i+1] = 0;
                break;
            }
        }
    }

    static inline fix32 fast_shl(fix32 x, uint8_t y)
    {
        // assumes: 0 < y < 31
        return frombits(x.bits() << y);
    }

    static inline fix32 fast_shr(fix32 x, uint8_t y)
    {
        // assumes: 0 < y < 31
        return frombits(x.bits() >> y);
    }

    static inline fix32 lshr(fix32 x, int y)
    {
        // If y is negative, use << instead.
        return y < 0 ? x << -y : frombits(y >= 32 ? 0 : uint32_t(x.bits()) >> y);
    }

    static inline fix32 rotl(fix32 x, int y)
    {
        y &= 0x1f;
        return frombits((x.bits() << y) | (uint32_t(x.bits()) >> (32 - y)));
    }

    static inline fix32 rotr(fix32 x, int y)
    {
        y &= 0x1f;
        return frombits((uint32_t(x.bits()) >> y) | (x.bits() << (32 - y)));
    }
    // Trigonometric / exponential / power functions. fix32 has no native
    // implementation for these (sin/cos are custom, the rest aren't), so we
    // delegate to <cmath> float versions and round-trip through fix32.
    // PICO-8's `math.tan/asin/...` are uncommon in carts so the cost of
    // bouncing through float is acceptable; the heavy hitters (floor, ceil,
    // abs, mod, the +/-/*// arithmetic) stay native fixed-point.

    static inline fix32 sinh(fix32 x) { return fix32(float(std::sinh(float(x)))); }
    static inline fix32 cosh(fix32 x) { return fix32(float(std::cosh(float(x)))); }
    static inline fix32 tan(fix32 x)  { return fix32(float(std::tan(float(x)))); }
    static inline fix32 tanh(fix32 x) { return fix32(float(std::tanh(float(x)))); }
    static inline fix32 asin(fix32 x) { return fix32(float(std::asin(float(x)))); }
    static inline fix32 acos(fix32 x) { return fix32(float(std::acos(float(x)))); }
    static inline fix32 atan(fix32 x) { return fix32(float(std::atan(float(x)))); }
    static inline fix32 atan2(fix32 x, fix32 y)
                                   { return fix32(float(std::atan2(float(x), float(y)))); }
    static inline fix32 sqrt(fix32 x) { return fix32(float(std::sqrt(float(x)))); }
    static inline fix32 log(fix32 x)  { return fix32(float(std::log(float(x)))); }
    static inline fix32 log10(fix32 x){ return fix32(float(std::log10(float(x)))); }
    static inline fix32 exp(fix32 x)  { return fix32(float(std::exp(float(x)))); }
    static inline fix32 fmod(fix32 x, fix32 y)
                                   { return fix32(float(std::fmod(float(x), float(y)))); }

    // modf writes the integer part into *ip. The Lua stdlib takes `lua_Number*`
    // for both fp and ip, so we accept that and re-wrap.
    static inline fix32 modf(fix32 x, fix32 *ip)
    {
        float i;
        float f = std::modf(float(x), &i);
        *ip = fix32(i);
        return fix32(f);
    }

    // frexp / ldexp: PICO-8's `math.frexp/n.ldexp` deals with whole-number
    // exponents (probably mostly n=0) so the float round-trip is fine.
    static inline fix32 frexp(fix32 x, int *ep)
    {
        return fix32(float(std::frexp(float(x), ep)));
    }
    static inline fix32 ldexp(fix32 x, int ep)
    {
        return fix32(float(std::ldexp(float(x), ep)));
    }

    static inline fix32 sin(fix32 angle) {
        // https://www.nullhardware.com/blog/fixed-point-sine-and-cosine-for-embedded-systems/
        int16_t i = -1 * (angle << 15);
        /* Convert (signed) input to a value between 0 and 8192. (8192 is pi/2, which is the region of the curve fit). */
        /* ------------------------------------------------------------------- */
        i <<= 1;
        uint8_t c = i<0; //set carry for output pos/neg

        if(i == (i|0x4000)) // flip input value to corresponding value in range [0..8192)
            i = (1<<15) - i;
        i = (i & 0x7FFF) >> 1;
        /* ------------------------------------------------------------------- */

        /* The following section implements the formula:
           = y * 2^-n * ( A1 - 2^(q-p)* y * 2^-n * y * 2^-n * [B1 - 2^-r * y * 2^-n * C1 * y]) * 2^(a-q)
           Where the constants are defined as follows:
           */
        enum {A1=3370945099UL, B1=2746362156UL, C1=292421UL};
        enum {n=13, p=32, q=31, r=3, a=12};

        uint32_t y = (C1*((uint32_t)i))>>n;
        y = B1 - (((uint32_t)i*y)>>r);
        y = (uint32_t)i * (y>>n);
        y = (uint32_t)i * (y>>n);
        y = A1 - (y>>(p-q));
        y = (uint32_t)i * (y>>n);
        y = (y+(1UL<<(q-a-1)))>>(q-a); // Rounding

        // returns -4k == -1; +4k == +1
        // 4k = 2^12
        // but fix32 is on 16bit
        y <<= 4;

        return frombits((c ? -y : y));
    }

    static inline fix32 cos(fix32 angle) {
        return sin(angle+z8::fix32(0.75f));
    }

private:
    int32_t m_bits;
};




}

