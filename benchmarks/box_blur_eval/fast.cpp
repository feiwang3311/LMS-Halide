/*Copyright (c) 2012-2018 MIT CSAIL, Google Inc., and other contributors

Developed by:

  The Halide team
  http://halide-lang.org

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <cmath>
#include <cstdint>
#include <cstdio>
#ifdef __SSE2__
#include <emmintrin.h>
#elif __ARM_NEON
#include <arm_neon.h>
#endif

#include "../benchmark.h"
#include "HalideBuffer.h"

using namespace Halide::Runtime;

Buffer<uint16_t> blur_fast(Buffer<uint16_t> in, Buffer<uint16_t> out) {
#ifdef __SSE2__
        __m128i one_third = _mm_set1_epi16(21846);
//#pragma omp parallel for
        for (int yTile = 0; yTile < out.height(); yTile += 32) {
            __m128i tmp[(128/8) * (32 + 2)];
            for (int xTile = 0; xTile < out.width(); xTile += 128) {
                __m128i *tmpPtr = tmp;
                for (int y = 0; y < 32+2; y++) {
                    const uint16_t *inPtr = &(in(xTile, yTile+y));
                    for (int x = 0; x < 128; x += 8) {
                        __m128i a = _mm_load_si128((const __m128i*)(inPtr));
                        __m128i b = _mm_loadu_si128((const __m128i*)(inPtr+1));
                        __m128i c = _mm_loadu_si128((const __m128i*)(inPtr+2));
                        __m128i sum = _mm_add_epi16(_mm_add_epi16(a, b), c);
                        __m128i avg = _mm_mulhi_epi16(sum, one_third);
                        _mm_store_si128(tmpPtr++, avg);
                        inPtr+=8;
                    }
                }
                tmpPtr = tmp;
                for (int y = 0; y < 32; y++) {
                    __m128i *outPtr = (__m128i *)(&(out(xTile, yTile+y)));
                    for (int x = 0; x < 128; x += 8) {
                        __m128i a = _mm_load_si128(tmpPtr+(2*128)/8);
                        __m128i b = _mm_load_si128(tmpPtr+128/8);
                        __m128i c = _mm_load_si128(tmpPtr++);
                        __m128i sum = _mm_add_epi16(_mm_add_epi16(a, b), c);
                        __m128i avg = _mm_mulhi_epi16(sum, one_third);
                        _mm_store_si128(outPtr++, avg);
                    }
                }
            }
        }
#elif __ARM_NEON
        uint16x4_t one_third = vdup_n_u16(21846);
#pragma omp parallel for
        for (int yTile = 0; yTile < out.height(); yTile += 32) {
            uint16x8_t tmp[(128/8) * (32 + 2)];
            for (int xTile = 0; xTile < out.width(); xTile += 128) {
                uint16_t *tmpPtr = (uint16_t*)tmp;
                for (int y = 0; y < 32+2; y++) {
                    const uint16_t *inPtr = &(in(xTile, yTile+y));
                    for (int x = 0; x < 128; x += 8) {
                        uint16x8_t a = vld1q_u16(inPtr);
                        uint16x8_t b = vld1q_u16(inPtr+1);
                        uint16x8_t c = vld1q_u16(inPtr+2);
                        uint16x8_t sum = vaddq_u16(vaddq_u16(a, b), c);
                        uint16x4_t sumlo = vget_low_u16(sum);
                        uint16x4_t sumhi = vget_high_u16(sum);
                        uint16x4_t avglo = vshrn_n_u32(vmull_u16(sumlo, one_third), 16);
                        uint16x4_t avghi = vshrn_n_u32(vmull_u16(sumhi, one_third), 16);
                        uint16x8_t avg = vcombine_u16(avglo, avghi);
                        vst1q_u16(tmpPtr, avg);
                        tmpPtr+=8;
                        inPtr+=8;
                    }
                }
                tmpPtr = (uint16_t*)tmp;
                for (int y = 0; y < 32; y++) {
                    uint16_t *outPtr = &(out(xTile, yTile+y));
                    for (int x = 0; x < 128; x += 8) {
                        uint16x8_t a = vld1q_u16(tmpPtr+(2*128));
                        uint16x8_t b = vld1q_u16(tmpPtr+128);
                        uint16x8_t c = vld1q_u16(tmpPtr);
                        uint16x8_t sum = vaddq_u16(vaddq_u16(a, b), c);
                        uint16x4_t sumlo = vget_low_u16(sum);
                        uint16x4_t sumhi = vget_high_u16(sum);
                        uint16x4_t avglo = vshrn_n_u32(vmull_u16(sumlo, one_third), 16);
                        uint16x4_t avghi = vshrn_n_u32(vmull_u16(sumhi, one_third), 16);
                        uint16x8_t avg = vcombine_u16(avglo, avghi);
                        vst1q_u16(outPtr, avg);
                        tmpPtr+=8;
                        outPtr+=8;
                    }
                }
            }
        }
#else
        // No intrinsics enabled, do a naive thing.
        for (int y = 0; y < out.height(); y++) {
            for (int x = 0; x < out.width(); x++) {
                int tmp[3] = {
                    (in(x, y) + in(x+1, y) + in(x+2, y))/3,
                    (in(x, y+1) + in(x+1, y+1) + in(x+2, y+1))/3,
                    (in(x, y+2) + in(x+1, y+2) + in(x+2, y+2))/3,
                };
                out(x, y) = (tmp[0] + tmp[1] + tmp[2])/3;
            }
        }
#endif
    return out;
}

void test_fast(void) {
  blur_fast(in, out);
}
