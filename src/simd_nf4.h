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

#ifndef __QUAD_SIMD_NF4_H__
#define __QUAD_SIMD_NF4_H__

#include <x86intrin.h>

namespace quad {
namespace simd {

#ifdef QUADIRON_USE_AVX2
typedef __m128i m128i;

/** Return aint128 integer from a _m128i register */
static inline aint128 m128i_to_uint128(m128i v)
{
    aint128 i;
    _mm_store_si128((m128i*)&i, v);

    return i; // NOLINT(clang-analyzer-core.uninitialized.UndefReturn)
}
#endif // #ifdef QUADIRON_USE_AVX2

inline aint128 expand16(aint16* arr, int n)
{
    // since n <= 4
    uint16_t _arr[4] __attribute__((aligned(ALIGN_SIZE))) = {0, 0, 0, 0};
    std::copy_n(arr, n, _arr);

    m128i b = _mm_set_epi64(
        _mm_setzero_si64(), _mm_set_pi16(_arr[3], _arr[2], _arr[1], _arr[0]));

    return m128i_to_uint128(b);
}

inline aint128 expand32(aint32* arr, int n)
{
    // since n <= 4
    uint32_t _arr[4] __attribute__((aligned(ALIGN_SIZE))) = {0, 0, 0, 0};
    std::copy_n(arr, n, _arr);

    m128i b = _mm_set_epi32(_arr[3], _arr[2], _arr[1], _arr[0]);

    return m128i_to_uint128(b);
}

inline GroupedValues<__uint128_t> unpack(aint128 a, int n)
{
    aint32 flag = 0;
    uint32_t ai[4] __attribute__((aligned(ALIGN_SIZE)));
    uint32_t bi[4] __attribute__((aligned(ALIGN_SIZE))) = {0, 0, 0, 0};
    aint128 values;
    int i;

    m128i _a = _mm_loadu_si128((m128i*)&a);
    ai[0] = _mm_extract_epi32(_a, 0);
    ai[1] = _mm_extract_epi32(_a, 1);
    ai[2] = _mm_extract_epi32(_a, 2);
    ai[3] = _mm_extract_epi32(_a, 3);
    for (i = 0; i < n; i++) {
        if (ai[i] == 65536)
            flag |= (1 << i);
        else
            bi[i] = (aint16)ai[i];
    }
    m128i val = _mm_set_epi64(
        _mm_setzero_si64(), _mm_set_pi16(bi[3], bi[2], bi[1], bi[0]));
    _mm_store_si128((m128i*)&values, val);

    GroupedValues<__uint128_t> b = {values, flag};

    return b;
}

inline aint128 pack(aint128 a)
{
    m128i _a = _mm_loadu_si128((m128i*)&a);
    m128i b = _mm_set_epi32(
        _mm_extract_epi16(_a, 3),
        _mm_extract_epi16(_a, 2),
        _mm_extract_epi16(_a, 1),
        _mm_extract_epi16(_a, 0));

    return m128i_to_uint128(b);
}

inline aint128 pack(aint128 a, aint32 flag)
{
    aint32 b0, b1, b2, b3;
    m128i _a = _mm_loadu_si128((m128i*)&a);

    if (flag & 1)
        b0 = 65536;
    else
        b0 = _mm_extract_epi16(_a, 0);
    flag >>= 1;
    if (flag & 1)
        b1 = 65536;
    else
        b1 = _mm_extract_epi16(_a, 1);
    flag >>= 1;
    if (flag & 1)
        b2 = 65536;
    else
        b2 = _mm_extract_epi16(_a, 2);
    flag >>= 1;
    if (flag & 1)
        b3 = 65536;
    else
        b3 = _mm_extract_epi16(_a, 3);

    m128i b = _mm_set_epi32(b3, b2, b1, b0);

    return m128i_to_uint128(b);
}

} // namespace simd
} // namespace quad

#endif
