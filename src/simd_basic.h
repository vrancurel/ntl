/*
 * Copyright 2017-2018 Scality
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QUAD_SIMD_BASIC_H__
#define __QUAD_SIMD_BASIC_H__

#include <x86intrin.h>

namespace quadiron {
namespace simd {

template <typename T>
inline VecType CARD(T q)
{
    return (q == F3) ? F3_u32 : F4_u32;
}

template <typename T>
inline VecType CARD_M_1(T q)
{
    return (q == F3) ? F3m1_u32 : F4m1_u32;
}

/* ================= Basic Operations for u32 ================= */

/**
 * Modular addition for packed unsigned 32-bit integers
 *
 * @param x input register
 * @param y input register
 * @param q modulo
 * @return (x + y) mod q
 */
inline VecType ADD_MOD(VecType x, VecType y, uint32_t q)
{
    VecType res = ADD32(x, y);
    return MINU32(res, SUB32(res, CARD(q)));
}

/**
 * Modular subtraction for packed unsigned 32-bit integers
 *
 * @param x input register
 * @param y input register
 * @param q modulo
 * @return (x - y) mod q
 */
inline VecType SUB_MOD(VecType x, VecType y, uint32_t q)
{
    VecType res = SUB32(x, y);
    return MINU32(res, ADD32(res, CARD(q)));
}

/**
 * Modular negation for packed unsigned 32-bit integers
 *
 * @param x input register
 * @param q modulo
 * @return (-x) mod q
 */
inline VecType NEG_MOD(VecType x, uint32_t q)
{
    VecType res = SUB32(CARD(q), x);
    return MINU32(res, SUB32(res, CARD(q)));
}

/**
 * Modular multiplication for packed unsigned 32-bit integers
 *
 * @note We assume that at least `x` or `y` is less than `q-1` so it's
 * not necessary to verify overflow on multiplying elements
 *
 * @param x input register
 * @param y input register
 * @param q modulo
 * @return (x * y) mod q
 */
inline VecType MUL_MOD(VecType x, VecType y, uint32_t q)
{
    VecType res = MUL32(x, y);
    VecType lo =
        (q == F3) ? BLEND8(ZERO, res, MASK8_LO) : BLEND16(ZERO, res, 0x55);
    VecType hi = (q == F3) ? BLEND8(ZERO, SHIFTR(res, 1), MASK8_LO)
                           : BLEND16(ZERO, SHIFTR(res, 2), 0x55);
    return SUB_MOD(lo, hi, q);
}

/**
 * Modular general multiplication for packed unsigned 32-bit integers
 *
 * @note It's necessary to verify overflow on multiplying elements
 *
 * @param x input register
 * @param y input register
 * @param q modulo
 * @return (x * y) mod q
 */
inline VecType MULFULL_MOD(VecType x, VecType y, uint32_t q)
{
    VecType res = MUL32(x, y);

    // filter elements of both of a & b = card-1
    VecType cmp = AND(CMPEQ32(x, CARD_M_1(q)), CMPEQ32(y, CARD_M_1(q)));
    res = (q == F3) ? XOR(res, AND(F4_u32, cmp)) : ADD32(res, AND(ONE32, cmp));

    VecType lo =
        (q == F3) ? BLEND8(ZERO, res, MASK8_LO) : BLEND16(ZERO, res, 0x55);
    VecType hi = (q == F3) ? BLEND8(ZERO, SHIFTR(res, 1), MASK8_LO)
                           : BLEND16(ZERO, SHIFTR(res, 2), 0x55);
    return SUB_MOD(lo, hi, q);
}

/**
 * Update property for a given register for packed unsigned 32-bit integers
 *
 * @param props properties bound to fragments
 * @param threshold register storing max value in its elements
 * @param mask a specific mask
 * @param symb input register
 * @param offset offset in the data fragments
 * @param max a dummy variable
 */
inline void ADD_PROPS(
    Properties& props,
    VecType threshold,
    VecType mask,
    VecType symb,
    off_t offset,
    uint32_t max)
{
    const VecType b = CMPEQ32(threshold, symb);
    const VecType c = AND(mask, b);
    auto d = MVMSK8(c);
    const unsigned element_size = sizeof(uint32_t);
    while (d > 0) {
        unsigned byte_idx = __builtin_ctz(d);
        off_t _offset = offset + byte_idx / element_size;
        props.add(_offset, OOR_MARK);
        d ^= 1 << byte_idx;
    }
}

/* ================= Basic Operations for u16 ================= */

/**
 * Modular addition for packed unsigned 16-bit integers
 *
 * @param x input register
 * @param y input register
 * @param q modulo
 * @return (x + y) mod q
 */
inline VecType ADD_MOD(VecType x, VecType y, uint16_t q)
{
    VecType res = ADD16(x, y);
    return MINU16(res, SUB16(res, F3_u16));
}

/**
 * Modular subtraction for packed unsigned 16-bit integers
 *
 * @param x input register
 * @param y input register
 * @param q modulo
 * @return (x - y) mod q
 */
inline VecType SUB_MOD(VecType x, VecType y, uint16_t q)
{
    VecType res = SUB16(x, y);
    return MINU16(res, SUB16(ADD16(x, F3_u16), y));
}

/**
 * Modular negation for packed unsigned 16-bit integers
 *
 * @param x input register
 * @param q modulo
 * @return (-x) mod q
 */
inline VecType NEG_MOD(VecType x, uint16_t q)
{
    VecType res = SUB16(F3_u16, x);
    return MINU16(res, SUB16(res, F3_u16));
}

/**
 * Modular multiplication for packed unsigned 16-bit integers
 *
 * @note We assume that at least `x` or `y` is less than `q-1` so it's
 * not necessary to verify overflow on multiplying elements
 *
 * @param x input register
 * @param y input register
 * @param q modulo
 * @return (x * y) mod q
 */
inline VecType MUL_MOD(VecType x, VecType y, uint16_t q)
{
    VecType res = MUL16(x, y);
    VecType lo = BLEND8(ZERO, res, MASK8_LO);
    VecType hi = BLEND8(ZERO, SHIFTR(res, 1), MASK8_LO);
    return SUB_MOD(lo, hi, q);
}

/**
 * Modular general multiplication for packed unsigned 16-bit integers
 *
 * @note It's necessary to verify overflow on multiplying elements
 *
 * @param x input register
 * @param y input register
 * @param q modulo
 * @return (x * y) mod q
 */
inline VecType MULFULL_MOD(VecType x, VecType y, uint16_t q)
{
    VecType res = MUL16(x, y);

    // filter elements of both of a & b = card-1
    VecType cmp = AND(CMPEQ16(x, F3m1_u16), CMPEQ16(y, F3m1_u16));
    res = ADD16(res, AND(ONE16, cmp));

    VecType lo = BLEND8(ZERO, res, MASK8_LO);
    VecType hi = BLEND8(ZERO, SHIFTR(res, 1), MASK8_LO);
    return SUB_MOD(lo, hi, q);
}

/**
 * Update property for a given register for packed unsigned 32-bit integers
 *
 * @param props properties bound to fragments
 * @param threshold register storing max value in its elements
 * @param mask a specific mask
 * @param symb input register
 * @param offset offset in the data fragments
 * @param max a dummy variable
 */
inline void ADD_PROPS(
    Properties& props,
    VecType threshold,
    VecType mask,
    VecType symb,
    off_t offset,
    uint16_t max)
{
    const VecType b = CMPEQ16(threshold, symb);
    const VecType c = AND(mask, b);
    auto d = MVMSK8(c);
    const unsigned element_size = sizeof(uint16_t);
    while (d > 0) {
        unsigned byte_idx = __builtin_ctz(d);
        off_t _offset = offset + byte_idx / element_size;
        props.add(_offset, OOR_MARK);
        d ^= 1 << byte_idx;
    }
}

/* ==================== Operations for RingModN =================== */
/** Perform a multiplication of a coefficient `a` to each element of `src` and
 *  add result to correspondent element of `dest`
 *
 * @note: 1 < `a` < card - 1
 */
template <typename T>
inline void mul_coef_to_buf(const T a, T* src, T* dest, size_t len, T card)
{
    const VecType coef = SET1(a);

    VecType* __restrict _src = reinterpret_cast<VecType*>(src);
    VecType* __restrict _dest = reinterpret_cast<VecType*>(dest);
    const unsigned ratio = sizeof(*_src) / sizeof(*src);
    const size_t _len = len / ratio;
    const size_t _last_len = len - _len * ratio;

    size_t i = 0;
    size_t end = (_len > 3) ? _len - 3 : 0;
    for (; i < end; i += 4) {
        _dest[i] = MUL_MOD(coef, _src[i], card);
        _dest[i + 1] = MUL_MOD(coef, _src[i + 1], card);
        _dest[i + 2] = MUL_MOD(coef, _src[i + 2], card);
        _dest[i + 3] = MUL_MOD(coef, _src[i + 3], card);
    }
    for (; i < _len; ++i) {
        _dest[i] = MUL_MOD(coef, _src[i], card);
    }

    if (_last_len > 0) {
        DoubleSizeVal<T> coef_double = DoubleSizeVal<T>(a);
        for (size_t i = _len * ratio; i < len; i++) {
            dest[i] = (T)((coef_double * src[i]) % card);
        }
    }
}

template <typename T>
inline void add_two_bufs(T* src, T* dest, size_t len, T card)
{
    VecType* __restrict _src = reinterpret_cast<VecType*>(src);
    VecType* __restrict _dest = reinterpret_cast<VecType*>(dest);
    const unsigned ratio = sizeof(*_src) / sizeof(*src);
    const size_t _len = len / ratio;
    const size_t _last_len = len - _len * ratio;

    size_t i;
    for (i = 0; i < _len; i++) {
        _dest[i] = ADD_MOD(_src[i], _dest[i], card);
    }
    if (_last_len > 0) {
        for (i = _len * ratio; i < len; i++) {
            T tmp = src[i] + dest[i];
            dest[i] = (tmp >= card) ? (tmp - card) : tmp;
        }
    }
}

template <typename T>
inline void sub_two_bufs(T* bufa, T* bufb, T* res, size_t len, T card)
{
    VecType* __restrict _bufa = reinterpret_cast<VecType*>(bufa);
    VecType* __restrict _bufb = reinterpret_cast<VecType*>(bufb);
    VecType* __restrict _res = reinterpret_cast<VecType*>(res);
    const unsigned ratio = sizeof(*_bufa) / sizeof(*bufa);
    const size_t _len = len / ratio;
    const size_t _last_len = len - _len * ratio;

    size_t i;
    for (i = 0; i < _len; i++) {
        // perform subtraction
        _res[i] = SUB_MOD(_bufa[i], _bufb[i], card);
    }
    if (_last_len > 0) {
        for (i = _len * ratio; i < len; i++) {
            // perform subtraction
            if (bufa[i] >= bufb[i])
                res[i] = bufa[i] - bufb[i];
            else
                res[i] = card - (bufb[i] - bufa[i]);
        }
    }
}

template <typename T>
inline void mul_two_bufs(T* src, T* dest, size_t len, T card)
{
    VecType* __restrict _src = reinterpret_cast<VecType*>(src);
    VecType* __restrict _dest = reinterpret_cast<VecType*>(dest);
    const unsigned ratio = sizeof(*_src) / sizeof(*src);
    const size_t _len = len / ratio;
    const size_t _last_len = len - _len * ratio;

    size_t i;
    for (i = 0; i < _len; i++) {
        // perform multiplicaton
        _dest[i] = MULFULL_MOD(_src[i], _dest[i], card);
    }
    if (_last_len > 0) {
        for (i = _len * ratio; i < len; i++) {
            // perform multiplicaton
            dest[i] = T((DoubleSizeVal<T>(src[i]) * dest[i]) % card);
        }
    }
}

/** Apply an element-wise negation to a buffer
 */
template <typename T>
inline void neg(size_t len, T* buf, T card)
{
    VecType* _buf = reinterpret_cast<VecType*>(buf);
    unsigned ratio = sizeof(*_buf) / sizeof(*buf);
    size_t _len = len / ratio;
    size_t _last_len = len - _len * ratio;

    size_t i;
    for (i = 0; i < _len; i++) {
        _buf[i] = NEG_MOD(_buf[i], card);
    }
    if (_last_len > 0) {
        for (i = _len * ratio; i < len; i++) {
            if (buf[i])
                buf[i] = card - buf[i];
        }
    }
}

} // namespace simd
} // namespace quadiron

#endif
