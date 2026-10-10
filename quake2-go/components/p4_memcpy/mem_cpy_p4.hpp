#pragma once
/*
 * Copyright 2025, <https://github.com/BitsForPeople>
 *
 * Usage: mem_cpy::cpy_mem(&source, &dest, numberOfBytes);
 * Note the reversed order of source and dest compared to std::memcpy().
 * Can be changed by just editing the signature of cpy_mem() if desired
 */
#include <sdkconfig.h>
#include <cstdint>
#include <type_traits>

namespace mem_cpy {
    namespace {
        static constexpr bool BUILDING_FOR_P4 =
            #if CONFIG_IDF_TARGET_ESP32P4
                true;
            #else
                false;
            #endif

        static_assert(BUILDING_FOR_P4, "Only ESP32-P4 is supported.");

        template<typename T, typename P>
        static inline T& as(P* ptr) {
            return *((T*)(ptr));
        }

        template<typename T>
        static inline T* pplus(T* const ptr, int i) {
            return (T*)((uintptr_t)ptr + i);
        }

        template<typename T>
        static inline void incp(T*& ptr, int i) {
            ptr = (T*)((uintptr_t)ptr + i);
        }

        /**
         * @brief Checks if \p cond is \e known at compile-time to be \c true.
         *
         * @param cond
         * @return true if \p cond is known to be \c true at compile-time
         * @return false if \p const is not known to be \c true at compile-time
         */
        static inline constexpr bool known(bool cond) noexcept {
            return __builtin_constant_p(cond) && cond;
        }

        /**
         * @brief Checks if \p cond \e may be \c true at compile- or run-time,
         * i.e. if, at compile-time, \p cond is \e not known to be \c false .
         *
         * @param cond
         * @return true if \p cond \e may be \c true
         * @return false if \p cond is \e known to be \c false at compile-time
         */
        static inline constexpr bool maybe(bool cond) noexcept {
            return !known(!cond);
        }

        /**
         * @brief Returns \p x divided by \p D , i.e. <tt>(x/D)</tt>, but forces
         * gcc to use hardware division at runtime.
         *
         * @tparam D
         * @param x
         * @return <tt>x/D</tt>
         */
        template<uint32_t D>
        requires (D != 0)
        static constexpr uint32_t div(const uint32_t x) {
            if(std::is_constant_evaluated() || __builtin_constant_p(x/D)) {
                return x/D;
            } else {
                uint32_t d = D;
                asm ("":"+r"(d));
                const uint32_t r = x/d;
                [[assume(r == (x/D))]];
                return r;
            }
        }

        /**
         * @brief Returns the remainder of \p x divided by \p D , i.e. <tt>(x%D)</tt>,
         * but forces gcc to use hardware division at runtime.
         *
         * @tparam D
         * @param x
         * @return <tt>x%D</tt>
         */
        template<uint32_t D>
        requires (D != 0)
        static constexpr uint32_t rem(const uint32_t x) {
            if(std::is_constant_evaluated() || __builtin_constant_p(x%D)) {
                return x%D;
            } else {
                uint32_t d = D;
                asm ("":"+r"(d));
                const uint32_t r = x%d;
                [[assume(r == (x % D))]];
                [[assume(r < D)]];
                return r;
            }
        }

        [[gnu::always_inline]]
        static constexpr uint32_t min(const uint32_t a, const uint32_t b) {
            return (a<b) ? a : b;
        }

        template<typename T, typename S, typename D>
        static inline void cpyAs(const S*& src, D*& dst) {
            as<T>(dst) = as<const T>(src);
            incp(dst,sizeof(T));
            incp(src,sizeof(T));
        }

        /**
         * @brief Copies 0-15 bytes from \p src to \p dst and advances both pointers
         * to the first byte after those copied.
         *
         * @tparam S
         * @tparam D
         * @param[in,out] src
         * @param[in,out] dst
         * @param cnt number of bytes to copy; <tt>(cnt % 16)</tt> bytes will be copied.
         */
        template<typename S, typename D>
        static inline void cpyshrt(const S*& src, D*& dst, const uint32_t cnt) {
            if(cnt & 1) {
                cpyAs<uint8_t>(src,dst);
            }
            if(cnt & 2) {
                cpyAs<uint16_t>(src,dst);
            }
            if(cnt & 4) {
                cpyAs<uint32_t>(src,dst);
            }
            if(cnt & 8) {
                cpyAs<uint64_t>(src,dst);
            }
        }

    } // anon namespace



    /**
     * @brief Copies \p cnt bytes from \p src to \p dst
     *
     * @param src
     * @param dst
     * @param cnt
     */
    inline void cpy_mem(const void* src, void* dst, uint32_t cnt) {

        static constexpr unsigned VEC_LEN = 16;

        if(cnt >= VEC_LEN) {
            // Only try to go SIMD if we have at least a chance to actually use it.
            {
                // Make it so that *dst is 16-byte aligned if it's not already:
                uint32_t off = ((uintptr_t)dst + 15) & ~0xf;
                if(off != (uintptr_t)dst) {
                    off = off - (uintptr_t)dst;
                    // off is now the number of bytes we'd need to copy to make *dst 16-byte-aligned
                    off = min(off,cnt);
                    cnt -= off;
                    cpyshrt(src,dst,off);
                }
            }

            if (cnt >= VEC_LEN) [[likely]] {
                asm (
                    "ESP.LD.128.USAR.IP q0, %[src], 16" "\n"
                    "ESP.VLD.128.IP q1, %[src], 16" "\n"
                    : [src] "+r" (src)
                    : "m" (*(const uint8_t(*)[2*VEC_LEN])src)
                );

                if(cnt >= 3*VEC_LEN) [[likely]] {
                    // Copy 48-byte chunks @ 2 instructions/vector
                    const uint32_t i = div<3*VEC_LEN>(cnt);
                    cnt = rem<3*VEC_LEN>(cnt);
                    asm (
                        "ESP.LP.SETUP 0, %[i], .Lend_%=" "\n"
                            "ESP.SRC.Q.LD.IP q2, %[src], 16, q0, q1" "\n"
                            "ESP.VST.128.IP q0, %[dst], 16" "\n"
                            "ESP.SRC.Q.LD.IP q0, %[src], 16, q1, q2" "\n"
                            "ESP.VST.128.IP q1, %[dst], 16" "\n"
                            "ESP.SRC.Q.LD.IP q1, %[src], 16, q2, q0" "\n"
                            "ESP.VST.128.IP q2, %[dst], 16" "\n"
                        ".Lend_%=:"
                        : [src] "+r" (src), [dst] "+r" (dst),
                          "=m" (*(uint8_t(*)[i*48])dst)
                        : [i] "r" (i),
                          "m" (*(const uint8_t(*)[i*48])src)
                    );
                }

                // assert( cnt < 3*VEC_LEN );

                // Copy remaining 0, 1, or 2 vectors @ 3 instructions/vector:

                // According to the P4 TRM, a hardware loop "should have a minimum of
                // six 32-bit instructions or twelve 16-bit instructions".
                // So no HW loop here.
                if(cnt >= VEC_LEN) [[likely]] {
                    asm (
                        "ESP.SRC.Q.QUP q2, q0, q1" "\n"
                        "ESP.VLD.128.IP q1, %[src], 16" "\n"
                        "ESP.VST.128.IP q2, %[dst], 16" "\n"
                        : [src] "+r" (src), [dst] "+r" (dst),
                          "=m" (*(uint8_t(*)[VEC_LEN])dst)
                        : "m" (*(const uint8_t(*)[VEC_LEN])src)
                    );
                    if(cnt >= 2*VEC_LEN) {
                        asm (
                            "ESP.SRC.Q.QUP q2, q0, q1" "\n"
                            "ESP.VLD.128.IP q1, %[src], 16" "\n"
                            "ESP.VST.128.IP q2, %[dst], 16" "\n"
                            : [src] "+r" (src), [dst] "+r" (dst),
                              "=m" (*(uint8_t(*)[VEC_LEN])dst)
                            : "m" (*(const uint8_t(*)[VEC_LEN])src)
                        );
                    }
                    cnt = cnt % VEC_LEN;
                }

                incp(src,-32);

                if(cnt & 8) [[likely]] {
                    // We still have data loaded from src up to at least (src+16) in (q1:q0).
                    // Let's use as much of it as we can.
                    asm (
                        "ESP.SRC.Q.QUP q2, q0, q1" "\n"
                        "ESP.VST.L.64.IP q2, %[dst], 8" "\n"
                        : [dst] "+r" (dst),
                          "=m" (*(uint8_t(*)[8])dst)
                        :
                    );
                    incp(src,8);
                    cnt -= 8;
                    // assert( cnt < 8 );
                }

            }
        }

        // assert( cnt < 16 );
        if(cnt) [[likely]] {
            cpyshrt(src,dst,cnt);
        }
    }
} // namespace mem_cpy