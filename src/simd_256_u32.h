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

#ifndef __QUAD_SIMD_256_U32_H__
#define __QUAD_SIMD_256_U32_H__

#include <x86intrin.h>

namespace quadiron {
namespace simd {

/* ==================== Essential Operations =================== */

/** Perform a%card where a is a addition of two numbers whose elements are
 *  symbols of GF(card) */
inline m256i mod_after_add(m256i a, aint32 card)
{
    const m256i _card = _mm256_set1_epi32(card);
    const m256i _card_minus_1 = _mm256_set1_epi32(card - 1);

    m256i cmp = _mm256_cmpgt_epi32(a, _card_minus_1);
    m256i b = _mm256_sub_epi32(a, _mm256_and_si256(_card, cmp));

    return b;
}

/** Perform addition of two numbers a, b whose elements are of GF(card) */
inline m256i add(m256i a, m256i b, aint32 card)
{
    m256i _a = _mm256_load_si256(&a);
    m256i _b = _mm256_load_si256(&b);
    m256i c = _mm256_add_epi32(_a, _b);

    // Modulo
    return mod_after_add(c, card);
}

/** Perform subtraction of a by b where a, b whose elements are symbols of
 *  GF(card)
 * sub(a, b) = a - b if a >= b, or
 *             card + a - b, otherwise
 */
inline m256i sub(m256i a, m256i b, aint32 card)
{
    const m256i _card = _mm256_set1_epi32(card);

    m256i _a = _mm256_load_si256(&a);
    m256i _b = _mm256_load_si256(&b);

    m256i cmp = _mm256_cmpgt_epi32(_b, _a);
    m256i _a1 = _mm256_add_epi32(_a, _mm256_and_si256(_card, cmp));

    return _mm256_sub_epi32(_a1, _b);
}

/** Negate `a`
 * @return 0 if (a == 0), else card - a
 */
inline m256i neg(m256i a, aint32 card = F4)
{
    const m256i _card = _mm256_set1_epi32(card);
    m256i _a = _mm256_load_si256(&a);
    m256i _b = _mm256_setzero_si256();

    m256i cmp = _mm256_cmpgt_epi32(_a, _b);

    return _mm256_sub_epi32(_mm256_and_si256(cmp, _card), _a);
}

/** Perform a%card where a is a multiplication of two numbers whose elements are
 *  symbols of GF(F4)
 *
 *  We find v in a = u * card + v
 *  a is expressed also as: a = hi * (card-1) + lo
 *  where hi and lo is 16-bit for F4 (or 8-bit for F3) high and low parts of a
 *  hence, v = (lo - hi) % F4
 *      v = lo - hi, if lo >= hi
 *          or
 *          F4 + lo - hi, otherwise
 */
inline m256i mod_after_multiply_f4(m256i a)
{
    const m256i mask = _mm256_set1_epi32(F4 - 2);

    m256i lo = _mm256_and_si256(a, mask);

    m256i a_shift = _mm256_srli_si256(a, 2);
    m256i hi = _mm256_and_si256(a_shift, mask);

    m256i cmp = _mm256_cmpgt_epi32(hi, lo);
    m256i _lo = _mm256_add_epi32(lo, _mm256_and_si256(F4_m256i, cmp));

    return _mm256_sub_epi32(_lo, hi);
}

inline m256i mod_after_multiply_f3(m256i a)
{
    const m256i mask = _mm256_set1_epi32(F3 - 2);

    m256i lo = _mm256_and_si256(a, mask);

    m256i a_shift = _mm256_srli_si256(a, 1);
    m256i hi = _mm256_and_si256(a_shift, mask);

    m256i cmp = _mm256_cmpgt_epi32(hi, lo);
    m256i _lo = _mm256_add_epi32(lo, _mm256_and_si256(F3_m256i, cmp));

    return _mm256_sub_epi32(_lo, hi);
}

inline m256i mul_f4(m256i a, m256i b)
{
    m256i _a = _mm256_load_si256(&a);
    m256i _b = _mm256_load_si256(&b);

    m256i c = _mm256_mullo_epi32(_a, _b);

    // filter elements of both of a & b = card-1
    m256i cmp = _mm256_and_si256(
        _mm256_cmpeq_epi32(_a, F4minus1_m256i),
        _mm256_cmpeq_epi32(_b, F4minus1_m256i));

    const m256i one = _mm256_set1_epi32(1);
    c = _mm256_add_epi32(c, _mm256_and_si256(one, cmp));

    // Modulo
    return mod_after_multiply_f4(c);
}

inline m256i mul_f4_simple(m256i a, m256i b)
{
    m256i _a = _mm256_load_si256(&a);
    m256i _b = _mm256_load_si256(&b);

    m256i c = _mm256_mullo_epi32(_a, _b);

    // Modulo
    return mod_after_multiply_f4(c);
}

inline m256i mul_f3(m256i a, m256i b)
{
    m256i _a = _mm256_load_si256(&a);
    m256i _b = _mm256_load_si256(&b);

    m256i c = _mm256_mullo_epi32(_a, _b);

    // filter elements of both of a & b = card-1
    m256i cmp = _mm256_and_si256(
        _mm256_cmpeq_epi32(_a, F3minus1_m256i),
        _mm256_cmpeq_epi32(_b, F3minus1_m256i));

    c = _mm256_xor_si256(c, _mm256_and_si256(F4_m256i, cmp));

    // Modulo
    return mod_after_multiply_f3(c);
}

inline m256i mul_f3_simple(m256i a, m256i b)
{
    m256i _a = _mm256_load_si256(&a);
    m256i _b = _mm256_load_si256(&b);

    m256i c = _mm256_mullo_epi32(_a, _b);

    // Modulo
    return mod_after_multiply_f3(c);
}

/** Perform multiplication of two numbers a, b whose elements are of GF(card)
 *  where `card` is a prime Fermat number, i.e. card = Fx with x < 5
 *  Currently, it supports only for F3 and F4
 */
inline m256i mul(m256i a, m256i b, aint32 card)
{
    assert(card == F4 || card == F3);
    if (card == F4)
        return mul_f4(a, b);
    return mul_f3(a, b);
}

inline m256i mul_simple(m256i a, m256i b, aint32 card)
{
    assert(card == F4 || card == F3);
    if (card == F4)
        return mul_f4_simple(a, b);
    return mul_f3_simple(a, b);
}

// Following functions are used for AVX2 w/ both of u16 & u32
#define EITHER(x, a, b) (((x)) ? (a) : (b))
#define CARD(q) (EITHER(q == F3, F3_m256i, F4_m256i))
#define CARD_M_1(q) (EITHER(q == F3, F3minus1_m256i, F4minus1_m256i))
#define CARD_M_2(q) (EITHER(q == F3, F3minus2_m256i, F4minus2_m256i))
#define ZERO (_mm256_setzero_si256())
#define ONE (_mm256_set1_epi32(1))

inline m256i LOAD(m256i* address)
{
    return _mm256_load_si256(address);
}
inline void STORE(m256i* address, m256i reg)
{
    _mm256_store_si256(address, reg);
}

inline m256i AND(m256i x, m256i y)
{
    return _mm256_and_si256(x, y);
}
inline m256i XOR(m256i x, m256i y)
{
    return _mm256_xor_si256(x, y);
}
inline m256i SHIFTR_1(m256i x)
{
    return _mm256_srli_si256(x, 1);
}
inline m256i SHIFTR_2(m256i x)
{
    return _mm256_srli_si256(x, 2);
}
inline uint32_t MVMSK8(m256i x)
{
    return _mm256_movemask_epi8(x);
}
inline uint32_t TESTZ(m256i x, m256i y)
{
    return _mm256_testz_si256(x, y);
}

// Following functions are used for AVX2 w/ u32 only
inline m256i SET1(uint32_t val)
{
    return _mm256_set1_epi32(val);
}
inline m256i ADD32(m256i x, m256i y)
{
    return _mm256_add_epi32(x, y);
}
inline m256i SUB32(m256i x, m256i y)
{
    return _mm256_sub_epi32(x, y);
}
inline m256i MUL32(m256i x, m256i y)
{
    return _mm256_mullo_epi32(x, y);
}

inline m256i CMPEQ32(m256i x, m256i y)
{
    return _mm256_cmpeq_epi32(x, y);
}
inline m256i CMPGT32(m256i x, m256i y)
{
    return _mm256_cmpgt_epi32(x, y);
}
inline m256i MINU32(m256i x, m256i y) {
    return _mm256_min_epu32(x, y);
}
#define BLEND16(x, y, imm8) (_mm256_blend_epi16(x, y, imm8))

// z = x + y mod q
// Input are loaded to registers
// Output is register
inline m256i ADD_MOD(m256i x, m256i y, uint32_t q)
{
    m256i res = ADD32(x, y);
    return MINU32(res, SUB32(res, CARD(q)));
}

// z = x - y mod q => z = q + x - y mod q
// Input are loaded to registers
// Output is register
inline m256i SUB_MOD(m256i x, m256i y, uint32_t q)
{
    m256i res = SUB32(x, y);
    return MINU32(res, ADD32(res, CARD(q)));
}

// y = 0 - x mod q => y = q - x mod q
// Input are loaded to registers
// Output is register
inline m256i NEG_MOD(m256i x, uint32_t q)
{
    m256i res = SUB32(CARD(q), x);
    return MINU32(res, SUB32(res, CARD(q)));
}

// z = x * y mod q
// Input are loaded to registers
// Output is register
// Note: we assume that at least `x` or `y` is less than `q-1` so it's
// not necessary to verify overflow on multiplying elements
inline m256i MUL_MOD(m256i x, m256i y, uint32_t q)
{
    m256i res = MUL32(x, y);
    m256i lo = BLEND16(ZERO, res, 0x5555);
    m256i hi = BLEND16(ZERO, SHIFTR_2(res), 0x5555);
    return SUB_MOD(lo, hi, q);
}

// z = x * y mod q
// Input are loaded to registers
// Output is register
inline m256i MULFULL_MOD(m256i x, m256i y, uint32_t q)
{
    m256i res = MUL32(x, y);

    // filter elements of both of a & b = card-1
    m256i cmp = AND(CMPEQ32(x, CARD_M_1(q)), CMPEQ32(y, CARD_M_1(q)));
    res = (q == F3) ? XOR(res, AND(F4_m256i, cmp)) : ADD32(res, AND(ONE, cmp));

    m256i lo = BLEND16(ZERO, res, 0x5555);
    m256i hi = SHIFTR_2(BLEND16(ZERO, res, 0xAAAA));
    return SUB_MOD(lo, hi, q);
}

// r == 1
inline void BUTTERFLY_1(m256i* x, m256i* y, uint32_t q)
{
    m256i add = ADD_MOD(*x, *y, q);
    *y = SUB_MOD(*x, *y, q);
    *x = add;
}

// r == q - 1
inline void BUTTERFLY_2(m256i* x, m256i* y, uint32_t q)
{
    m256i add = ADD_MOD(*x, *y, q);
    *x = SUB_MOD(*x, *y, q);
    *y = add;
}

// 1 < r < q - 1
inline void BUTTERFLY_3(m256i c, m256i* x, m256i* y, uint32_t q)
{
    m256i z = MUL_MOD(c, *y, q);
    *y = SUB_MOD(*x, z, q);
    *x = ADD_MOD(*x, z, q);
}

// x' = x + y mod q
// y' = x - y mode q
// Input and output are memory addresses
inline void BUTTERFLY_ADD(m256i* x, m256i* y, uint32_t q)
{
    m256i _x = LOAD(x);
    m256i _y = LOAD(y);
    STORE(x, ADD_MOD(_x, _y, q));
    STORE(y, SUB_MOD(_x, _y, q));
}

// x' = x - y mod q
// y' = x + y mode q
// Input and output are memory addresses
inline void BUTTERFLY_SUB(m256i* x, m256i* y, uint32_t q)
{
    m256i _x = LOAD(x);
    m256i _y = LOAD(y);
    STORE(y, ADD_MOD(_x, _y, q));
    STORE(x, SUB_MOD(_x, _y, q));
}

// x = x + z * y mod q
// y = x - z * y mod q
// Input and output are memory addresses
inline void BUTTERFLY_MULADD(m256i* z, m256i* x, m256i* y, uint32_t q)
{
    m256i _z = LOAD(z);
    m256i _y = LOAD(y);
    _y = MUL_MOD(_z, _y, q);
    m256i _x = LOAD(x);
    STORE(x, ADD_MOD(_x, _y, q));
    STORE(y, SUB_MOD(_x, _y, q));
}

// x = x + y mod q
// y = z * (x - y) mod q
// Input and output are memory addresses
inline void BUTTERFLY_ADDMUL(m256i* x, m256i* y, m256i* z, uint32_t q)
{
    m256i _x = LOAD(x);
    m256i _y = LOAD(y);
    m256i _sub = SUB_MOD(_x, _y, q);
    m256i _z = LOAD(z);
    STORE(x, ADD_MOD(_x, _y, q));
    STORE(y, MUL_MOD(_z, _sub, q));
}

inline void butterfly_step_1(
    uint32_t* __restrict a,
    uint32_t* __restrict b,
    uint32_t card,
    size_t len)
{
    m256i* inputA = reinterpret_cast<m256i*>(a);
    m256i* inputB = reinterpret_cast<m256i*>(b);

    // #pragma omp parallel for
    for (size_t j = 0; j < len; ++j) {
        BUTTERFLY_ADD(inputA + j, inputB + j, card);
    }
}

inline void butterfly_step_2(
    uint32_t* __restrict a,
    uint32_t* __restrict b,
    uint32_t card,
    size_t len)
{
    m256i* inputA = reinterpret_cast<m256i*>(a);
    m256i* inputB = reinterpret_cast<m256i*>(b);

    // #pragma omp parallel for
    for (size_t j = 0; j < len; ++j) {
        BUTTERFLY_SUB(inputA + j, inputB + j, card);
    }
}

inline void butterfly_step_3(
    uint32_t coef,
    uint32_t* __restrict a,
    uint32_t* __restrict b,
    uint32_t card,
    size_t len)
{
    m256i* _a = reinterpret_cast<m256i*>(a);
    m256i* _b = reinterpret_cast<m256i*>(b);
    m256i _coef = SET1(coef);

    // #pragma omp parallel for
    for (size_t j = 0; j < len; ++j) {
        BUTTERFLY_MULADD(&_coef, _a + j, _b + j, card);
    }
}

// for each pair (P, Q) = (buf[i], buf[i + m]):
// P = P + Q
// Q = P - Q
inline void butterfly_ct_1(
    vec::Buffers<uint32_t>& buf,
    unsigned start,
    unsigned m,
    unsigned step,
    size_t len,
    uint32_t card)
{
    for (int i = start; i < buf.get_n(); i += step) {
        uint32_t* a = buf.get(i);
        uint32_t* b = buf.get(i + m);
        // perform butterfly operation for Cooley-Tukey FFT algorithm
        butterfly_step_1(a, b, card, len);
    }
}

// for each pair (P, Q) = (buf[i], buf[i + m]):
// P = P - Q
// Q = P + Q
inline void butterfly_ct_2(
    vec::Buffers<uint32_t>& buf,
    unsigned start,
    unsigned m,
    unsigned step,
    size_t len,
    uint32_t card)
{
    for (int i = start; i < buf.get_n(); i += step) {
        uint32_t* a = buf.get(i);
        uint32_t* b = buf.get(i + m);
        // perform butterfly operation for Cooley-Tukey FFT algorithm
        butterfly_step_2(a, b, card, len);
    }
}

// for each pair (P, Q) = (buf[i], buf[i + m]):
// P = P + c * Q
// Q = P - c * Q
inline void butterfly_ct_3(
    uint32_t coef,
    vec::Buffers<uint32_t>& buf,
    unsigned start,
    unsigned m,
    unsigned step,
    size_t len,
    uint32_t card)
{
    for (int i = start; i < buf.get_n(); i += step) {
        uint32_t* a = buf.get(i);
        uint32_t* b = buf.get(i + m);
        // perform butterfly operation for Cooley-Tukey FFT algorithm
        butterfly_step_3(coef, a, b, card, len);
    }
}

/**
 * Vectorized butterly CT on two-layers at a time
 *
 * For each quadruple
 * (P, Q, R, S) = (buf[i], buf[i + m], buf[i + 2 * m], buf[i + 3 * m])
 * First layer: butterfly on (P, Q) and (R, S) for step = 2 * m
 *      coef r1 = W[start * n / (2 * m)]
 *      P = P + r1 * Q
 *      Q = P - r1 * Q
 *      R = R + r1 * S
 *      S = R - r1 * S
 * Second layer: butterfly on (P, R) and (Q, S) for step = 4 * m
 *      coef r2 = W[start * n / (4 * m)]
 *      coef r3 = W[(start + m) * n / (4 * m)]
 *      P = P + r2 * R
 *      R = P - r2 * R
 *      Q = Q + r3 * S
 *      S = Q - r3 * S
 *
 * @param buf - working buffers
 * @param r1 - coefficient for the 1st layer
 * @param r2 - 1st coefficient for the 2nd layer
 * @param r3 - 2nd coefficient for the 2nd layer
 * @param start - index of buffer among `m` ones
 * @param m - current group size
 * @param len - number of vectors per buffer
 * @param card - modulo cardinal
 */
inline void butterfly_ct_two_layers_step(
    vec::Buffers<uint32_t>& buf,
    unsigned r1,
    unsigned r2,
    unsigned r3,
    unsigned start,
    unsigned m,
    size_t len,
    uint32_t card)
{
    const unsigned step = m << 2;
    m256i c1 = SET1(r1);
    m256i c2 = SET1(r2);
    m256i c3 = SET1(r3);

#define BUTTERFLY_R1(c, x, y)                                                  \
    (EITHER(                                                                   \
        r1 == 1,                                                               \
        BUTTERFLY_1(x, y, card),                                               \
        EITHER(                                                                \
            r1 < card - 1,                                                     \
            BUTTERFLY_3(c, x, y, card),                                        \
            BUTTERFLY_2(x, y, card))));
#define BUTTERFLY_R2(c, x, y)                                                  \
    (EITHER(                                                                   \
        r2 == 1,                                                               \
        BUTTERFLY_1(x, y, card),                                               \
        EITHER(                                                                \
            r2 < card - 1,                                                     \
            BUTTERFLY_3(c, x, y, card),                                        \
            BUTTERFLY_2(x, y, card))));
#define BUTTERFLY_R3(c, x, y)                                                  \
    (EITHER(                                                                   \
        r3 == 1,                                                               \
        BUTTERFLY_1(x, y, card),                                               \
        EITHER(                                                                \
            r3 < card - 1,                                                     \
            BUTTERFLY_3(c, x, y, card),                                        \
            BUTTERFLY_2(x, y, card))));

    const size_t end = len - 1;
    const unsigned bufs_nb = buf.get_n();
    // #pragma omp parallel for
    // #pragma unroll
    for (unsigned i = start; i < bufs_nb; i += step) {
        m256i x1, y1, u1, v1;
        m256i x2, y2, u2, v2;
        m256i* __restrict p = reinterpret_cast<m256i*>(buf.get(i));
        m256i* __restrict q = reinterpret_cast<m256i*>(buf.get(i + m));
        m256i* __restrict r = reinterpret_cast<m256i*>(buf.get(i + 2 * m));
        m256i* __restrict s = reinterpret_cast<m256i*>(buf.get(i + 3 * m));

        // #pragma omp parallel for
        size_t j = 0;
        // #pragma unroll
        for (; j < end; j += 2) {
            // First layer (c1, x, y) & (c1, u, v)
            x1 = LOAD(p + j);
            y1 = LOAD(q + j);
            x2 = LOAD(p + j + 1);
            y2 = LOAD(q + j + 1);

            u1 = LOAD(r + j);
            v1 = LOAD(s + j);
            u2 = LOAD(r + j + 1);
            v2 = LOAD(s + j + 1);

            BUTTERFLY_R1(c1, &x1, &y1);
            BUTTERFLY_R1(c1, &x2, &y2);

            BUTTERFLY_R1(c1, &u1, &v1);
            BUTTERFLY_R1(c1, &u2, &v2);

            // Second layer (c2, x, u) & (c3, y, v)
            BUTTERFLY_R2(c2, &x1, &u1);
            BUTTERFLY_R2(c2, &x2, &u2);

            BUTTERFLY_R3(c3, &y1, &v1);
            BUTTERFLY_R3(c3, &y2, &v2);

            // Store back to memory
            STORE(p + j, x1);
            STORE(p + j + 1, x2);
            STORE(q + j, y1);
            STORE(q + j + 1, y2);

            STORE(r + j, u1);
            STORE(r + j + 1, u2);
            STORE(s + j, v1);
            STORE(s + j + 1, v2);
        }
        for (; j < len; ++j) {
            // First layer (c1, x, y) & (c1, u, v)
            x1 = LOAD(p + j);
            y1 = LOAD(q + j);
            u1 = LOAD(r + j);
            v1 = LOAD(s + j);

            BUTTERFLY_R1(c1, &x1, &y1);
            BUTTERFLY_R1(c1, &u1, &v1);
            // Second layer (c2, x, u) & (c3, y, v)
            BUTTERFLY_R2(c2, &x1, &u1);
            BUTTERFLY_R3(c3, &y1, &v1);
            // Store back to memory
            STORE(p + j, x1);
            STORE(q + j, y1);
            STORE(r + j, u1);
            STORE(s + j, v1);
        }
    }
}

// for each pair (P, Q) = (buf[i], buf[i + m]):
// P = Q + P
// Q = Q - P
inline void butterfly_gs_2(
    vec::Buffers<uint32_t>& buf,
    unsigned start,
    unsigned m,
    unsigned step,
    size_t len,
    uint32_t card)
{
    for (int i = start; i < buf.get_n(); i += step) {
        uint32_t* __restrict a = buf.get(i);
        uint32_t* __restrict b = buf.get(i + m);
        m256i* inputA = reinterpret_cast<m256i*>(a);
        m256i* inputB = reinterpret_cast<m256i*>(b);
        // perform butterfly operation for Cooley-Tukey FFT algorithm
        for (size_t j = 0; j < len; ++j) {
            BUTTERFLY_ADD(inputB + j, inputA + j, card);
        }
    }
}

// for each pair (P, Q) = (buf[i], buf[i + m]):
// P = P + Q
// Q = c * (P - Q)
inline void butterfly_gs_3(
    uint32_t coef,
    vec::Buffers<uint32_t>& buf,
    unsigned start,
    unsigned m,
    unsigned step,
    size_t len,
    uint32_t card)
{
    m256i _coef = SET1(coef);
    for (int i = start; i < buf.get_n(); i += step) {
        uint32_t* __restrict a = buf.get(i);
        uint32_t* __restrict b = buf.get(i + m);
        m256i* __restrict _a = reinterpret_cast<m256i*>(a);
        m256i* __restrict _b = reinterpret_cast<m256i*>(b);
        // perform butterfly operation for Cooley-Tukey FFT algorithm
        for (size_t j = 0; j < len; ++j) {
            BUTTERFLY_ADDMUL(_a + j, _b + j, &_coef, card);
        }
    }
}

inline void add_props(
    Properties& props,
    m256i threshold,
    m256i mask,
    m256i symb,
    off_t offset)
{
    const m256i b = CMPEQ32(threshold, symb);
    const m256i c = AND(mask, b);
    uint32_t d = MVMSK8(c);
    const unsigned element_size = sizeof(uint32_t);
    while (d > 0) {
        unsigned byte_idx = __builtin_ctz(d);
        off_t _offset = offset + byte_idx / element_size;
        props.add(_offset, OOR_MARK);
        d ^= 1 << byte_idx;
    }
}

inline void encode_post_process(
    vec::Buffers<uint32_t>& output,
    std::vector<Properties>& props,
    off_t offset,
    unsigned code_len,
    uint32_t threshold,
    size_t vecs_nb)
{
    const unsigned element_size = sizeof(uint32_t);
    const unsigned vec_size = ALIGN_SIZE / element_size;
    const uint32_t max = 1 << (element_size * 8 - 1);
    const m256i _threshold = SET1(threshold);
    const m256i mask_hi = SET1(max);

    // #pragma unroll
    for (unsigned frag_id = 0; frag_id < code_len; ++frag_id) {
        m256i* __restrict buf = reinterpret_cast<m256i*>(output.get(frag_id));

        size_t vec_id = 0;
        size_t end = vecs_nb - 3;
        // #pragma unroll
        for (; vec_id < end; vec_id += 4) {
            m256i a1 = LOAD(buf + vec_id);
            m256i a2 = LOAD(buf + vec_id + 1);
            m256i a3 = LOAD(buf + vec_id + 2);
            m256i a4 = LOAD(buf + vec_id + 3);

            uint32_t c1 = TESTZ(a1, _threshold);
            uint32_t c2 = TESTZ(a2, _threshold);
            uint32_t c3 = TESTZ(a3, _threshold);
            uint32_t c4 = TESTZ(a4, _threshold);

            if (c1 == 0) {
                const off_t curr_offset = offset + vec_id * vec_size;
                add_props(props[frag_id], _threshold, mask_hi, a1, curr_offset);
            }
            if (c2 == 0) {
                const off_t curr_offset = offset + (vec_id + 1) * vec_size;
                add_props(props[frag_id], _threshold, mask_hi, a2, curr_offset);
            }
            if (c3 == 0) {
                const off_t curr_offset = offset + (vec_id + 2) * vec_size;
                add_props(props[frag_id], _threshold, mask_hi, a3, curr_offset);
            }
            if (c4 == 0) {
                const off_t curr_offset = offset + (vec_id + 3) * vec_size;
                add_props(props[frag_id], _threshold, mask_hi, a4, curr_offset);
            }
        }
        for (; vec_id < vecs_nb; ++vec_id) {
            m256i a = LOAD(buf + vec_id);
            uint32_t c = TESTZ(a, _threshold);
            if (c == 0) {
                const off_t curr_offset = offset + vec_id * vec_size;
                add_props(props[frag_id], _threshold, mask_hi, a, curr_offset);
            }
        }
    }
}

/* ==================== Operations =================== */
/** Perform a multiplication of a coefficient `a` to each element of `src` and
 *  add result to correspondent element of `dest`
 *
 * @note: 1 < `a` < card - 1
 */
inline void mul_coef_to_buf(
    const uint32_t a,
    aint32* src,
    aint32* dest,
    size_t len,
    uint32_t card)
{
    const m256i coef = SET1(a);

    m256i* __restrict _src = reinterpret_cast<m256i*>(src);
    m256i* __restrict _dest = reinterpret_cast<m256i*>(dest);
    const unsigned ratio = sizeof(*_src) / sizeof(*src);
    const size_t _len = len / ratio;
    const size_t _last_len = len - _len * ratio;

    size_t i;
    for (i = 0; i < _len; i++) {
        // perform multiplication
        _dest[i] = MUL_MOD(coef, _src[i], card);
    }
    if (_last_len > 0) {
        uint64_t coef_64 = (uint64_t)a;
        for (i = _len * ratio; i < len; i++) {
            // perform multiplication
            dest[i] = (aint32)((coef_64 * src[i]) % card);
        }
    }
}

inline void
add_two_bufs(aint32* src, aint32* dest, size_t len, aint32 card)
{
    m256i* __restrict _src = reinterpret_cast<m256i*>(src);
    m256i* __restrict _dest = reinterpret_cast<m256i*>(dest);
    const unsigned ratio = sizeof(*_src) / sizeof(*src);
    const size_t _len = len / ratio;
    const size_t _last_len = len - _len * ratio;

    size_t i;
    for (i = 0; i < _len; i++) {
        // perform addition
        _dest[i] = ADD_MOD(_src[i], _dest[i], card);
    }
    if (_last_len > 0) {
        for (i = _len * ratio; i < len; i++) {
            // perform addition
            aint32 tmp = src[i] + dest[i];
            dest[i] = (tmp >= card) ? (tmp - card) : tmp;
        }
    }
}

inline void sub_two_bufs(
    aint32* bufa,
    aint32* bufb,
    aint32* res,
    size_t len,
    aint32 card = F4)
{
    m256i* __restrict _bufa = reinterpret_cast<m256i*>(bufa);
    m256i* __restrict _bufb = reinterpret_cast<m256i*>(bufb);
    m256i* __restrict _res = reinterpret_cast<m256i*>(res);
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

inline void
mul_two_bufs(aint32* src, aint32* dest, size_t len, aint32 card)
{
    m256i* __restrict _src = reinterpret_cast<m256i*>(src);
    m256i* __restrict _dest = reinterpret_cast<m256i*>(dest);
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
            dest[i] = uint32_t((uint64_t(src[i]) * dest[i]) % card);
        }
    }
}

/** Apply an element-wise negation to a buffer
 */
inline void neg(size_t len, aint32* buf, aint32 card = F4)
{
    m256i* _buf = reinterpret_cast<m256i*>(buf);
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

/* ==================== Operations for NF4 =================== */
typedef __m128i m128i;

/** Return aint128 integer from a _m128i register */
inline aint128 m256i_to_uint128(m256i v)
{
    aint128 hi, lo;
    _mm256_storeu2_m128i((m128i*)&hi, (m128i*)&lo, v);
    return lo; // NOLINT(clang-analyzer-core.uninitialized.UndefReturn)
}

inline __uint128_t add(__uint128_t a, __uint128_t b)
{
    m256i _a = _mm256_castsi128_si256((m128i)a);
    m256i _b = _mm256_castsi128_si256((m128i)b);
    m256i res = add(_a, _b, F4);
    return m256i_to_uint128(res);
}

inline __uint128_t sub(__uint128_t a, __uint128_t b)
{
    m256i _a = _mm256_castsi128_si256((m128i)a);
    m256i _b = _mm256_castsi128_si256((m128i)b);
    m256i res = sub(_a, _b, F4);
    return m256i_to_uint128(res);
}

inline __uint128_t mul(__uint128_t a, __uint128_t b)
{
    m256i _a = _mm256_castsi128_si256((m128i)a);
    m256i _b = _mm256_castsi128_si256((m128i)b);
    m256i res = mul(_a, _b, F4);
    return m256i_to_uint128(res);
}

/** Add buffer `y` to two halves of `x`. `x` is of length `n` */
inline void
add_buf_to_two_bufs(int n, aint128* _x, aint128* _y, aint32 card = F4)
{
    int i;
    m256i* x = reinterpret_cast<m256i*>(_x);
    m256i* y = reinterpret_cast<m256i*>(_y);

    const unsigned ratio = sizeof(*x) / sizeof(*_x);
    const int len_y = n / 2;
    const int len_y_256 = len_y / ratio;
    const int last_len_y = len_y - len_y_256 * ratio;

    aint128* x_half = _x + len_y;
    m256i* x_next = reinterpret_cast<m256i*>(x_half);

    // add y to the first half of `x`
    for (i = 0; i < len_y_256; i++) {
        x[i] = add(x[i], y[i], card);
    }

    // add y to the second half of `x`
    for (i = 0; i < len_y_256; i++) {
        x_next[i] = add(x_next[i], y[i], card);
    }

    if (last_len_y > 0) {
        // add last _y[] to x and x_next
        for (i = len_y_256 * ratio; i < len_y; i++) {
            m256i _x_p = _mm256_castsi128_si256((m128i)_x[i]);
            m256i _x_next_p = _mm256_castsi128_si256((m128i)x_half[i]);
            m256i _y_p = _mm256_castsi128_si256((m128i)_y[i]);

            _x_p = add(_x_p, _y_p, card);
            _x_next_p = add(_x_next_p, _y_p, card);
        }
    }
}

inline void hadamard_mul(int n, aint128* _x, aint128* _y)
{
    int i;
    m256i* x = reinterpret_cast<m256i*>(_x);
    m256i* y = reinterpret_cast<m256i*>(_y);

    const unsigned ratio = sizeof(*x) / sizeof(*_x);
    const int len_256 = n / ratio;
    const int last_len = n - len_256 * ratio;

    // multiply y to the first half of `x`
    for (i = 0; i < len_256; i++) {
        x[i] = mul(x[i], y[i], F4);
    }

    if (last_len > 0) {
        // add last _y[] to x
        for (i = len_256 * ratio; i < n; i++) {
            m256i _x_p = _mm256_castsi128_si256((m128i)_x[i]);
            m256i _y_p = _mm256_castsi128_si256((m128i)_y[i]);

            _x_p = mul(_x_p, _y_p, F4);
        }
    }
}

inline void hadamard_mul_doubled(int n, aint128* _x, aint128* _y)
{
    int i;
    m256i* x = reinterpret_cast<m256i*>(_x);
    m256i* y = reinterpret_cast<m256i*>(_y);

    const unsigned ratio = sizeof(*x) / sizeof(*_x);
    const int len_y = n / 2;
    const int len_y_256 = len_y / ratio;
    const int last_len_y = len_y - len_y_256 * ratio;

    aint128* x_half = _x + len_y;
    m256i* x_next = reinterpret_cast<m256i*>(x_half);

    // multiply y to the first half of `x`
    for (i = 0; i < len_y_256; i++) {
        x[i] = mul(x[i], y[i], F4);
    }

    // multiply y to the second half of `x`
    for (i = 0; i < len_y_256; i++) {
        x_next[i] = mul(x_next[i], y[i], F4);
    }

    if (last_len_y > 0) {
        // add last _y[] to x and x_next
        for (i = len_y_256 * ratio; i < len_y; i++) {
            m256i _x_p = _mm256_castsi128_si256((m128i)_x[i]);
            m256i _x_next_p = _mm256_castsi128_si256((m128i)x_half[i]);
            m256i _y_p = _mm256_castsi128_si256((m128i)_y[i]);

            _x_p = mul(_x_p, _y_p, F4);
            _x_next_p = mul(_x_next_p, _y_p, F4);
        }
    }
}

} // namespace simd
} // namespace quadiron

#endif
