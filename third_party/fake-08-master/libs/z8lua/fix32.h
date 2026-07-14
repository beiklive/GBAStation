//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016–2024 Sam Hocevar <sam@hocevar.net>
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
    inline fix32(double d)
      : m_bits(int32_t(int64_t(d * 65536.0)))
    {}

    inline operator double() const
    {
        return double(m_bits) * (1.0 / 65536.0);
    }

    // Conversions up to int16_t are safe.
    inline fix32(int8_t x)  : m_bits(int32_t(x << 16)) {}
    inline fix32(uint8_t x) : m_bits(int32_t(x << 16)) {}
    inline fix32(int16_t x) : m_bits(int32_t(x << 16)) {}

    // Anything above int16_t is risky because of precision loss, but Lua
    // does too many implicit conversions from int that we can’t mark this
    // one as explicit.
    inline fix32(int32_t x)  : m_bits(int32_t(x << 16)) {}
    
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
    inline explicit operator bool() const { return bool(m_bits); }
    inline bool operator ==(fix32 x) const { return m_bits == x.m_bits; }
    inline bool operator !=(fix32 x) const { return m_bits != x.m_bits; }
    inline bool operator  <(fix32 x) const { return m_bits  < x.m_bits; }
    inline bool operator  >(fix32 x) const { return m_bits  > x.m_bits; }
    inline bool operator <=(fix32 x) const { return m_bits <= x.m_bits; }
    inline bool operator >=(fix32 x) const { return m_bits >= x.m_bits; }

    // Increments
    inline fix32& operator ++() { m_bits += 0x10000; return *this; }
    inline fix32& operator --() { m_bits -= 0x10000; return *this; }
    inline fix32 operator ++(int) { fix32 ret = *this; ++*this; return ret; }
    inline fix32 operator --(int) { fix32 ret = *this; --*this; return ret; }

    // Math operations
    inline fix32 const &operator +() const { return *this; }
    inline fix32 operator -() const { return frombits(-m_bits); }
    inline fix32 operator ~() const { return frombits(~m_bits); }

    inline fix32 operator +(fix32 x) const { return frombits(m_bits + x.m_bits); }
    inline fix32 operator -(fix32 x) const { return frombits(m_bits - x.m_bits); }
    inline fix32 operator &(fix32 x) const { return frombits(m_bits & x.m_bits); }
    inline fix32 operator |(fix32 x) const { return frombits(m_bits | x.m_bits); }
    inline fix32 operator ^(fix32 x) const { return frombits(m_bits ^ x.m_bits); }

    fix32 operator *(fix32 x) const
    {
        return frombits(int64_t(m_bits) * x.m_bits >> 16);
    }

    fix32 operator /(fix32 x) const
    {
        // This special case ensures 0x8000/0x1 = 0x8000, not 0x8000.0001
        if (x.m_bits == 0x1'0000)
            return *this;

        if (x.m_bits)
        {
            using std::abs;
            int64_t result = int64_t(m_bits) * 0x1'0000 / x.m_bits;
            if (abs(result) <= 0x7fff'ffffu)
                return frombits(int32_t(result));
        }

        // Return 0x8000.0001 (not 0x8000.0000) for -Inf, just like PICO-8
        return frombits((m_bits ^ x.m_bits) >= 0 ? 0x7fff'ffffu : 0x8000'0001u);
    }

    fix32 operator %(fix32 x) const
    {
        // PICO-8 always returns positive values
        x = abs(x);
        // PICO-8 0.2.5f changelog: x % 0 gives 0 (was x)
        int32_t result = x ? m_bits % x.m_bits : 0;
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

    // PICO-8 0.2.3 changelog: abs(0x8000) should be 0x7fff.ffff
    static inline fix32 abs(fix32 a) { return a.m_bits >= 0 ? a : a.m_bits << 1 == 0 ? ~a : -a; }

    static inline fix32 min(fix32 a, fix32 b) { return a < b ? a : b; }
    static inline fix32 max(fix32 a, fix32 b) { return a > b ? a : b; }

    static inline fix32 ceil(fix32 x) { return -floor(-x); }
    static inline fix32 modf(fix32 x) { return frombits(x.m_bits & 0x0000'ffff); }
    static inline fix32 floor(fix32 x) { return frombits(x.m_bits & 0xffff'0000); }

    static fix32 pow(fix32 x, fix32 y) 
    {
        if ((y.m_bits & 0xffff0000) == (uint32_t)y.m_bits) {
            return pow(x, (int)y);
        }
        return fix32(std::pow(double(x), double(y))); 
    }

    static fix32 pow(fix32 x, int y) 
    {
        fix32 res = 1;
        if (y > 0) {
            for(int i = 0; i < y; i++) {
                res *= x;
            }
        }
        else {
            for(int i = 0; i > y; i--){
                res /= x;
            }
        }
        return res;
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

#ifdef _3DS
    inline explicit fix32(size_t x) : m_bits(int32_t(x << 16)) {}

    inline fix32(int x)  : m_bits(int(x << 16)) {}

    inline explicit operator size_t() const { return m_bits >> 16; }
    
#endif

    static inline fix32 ldexp(fix32 x, int y)
    {
        return fix32(std::ldexp((double)x, y));
    }

private:
    int32_t m_bits;
};

}

