#include "HalideBuffer.h"
#ifndef BENCHMARK_H
#define BENCHMARK_H

double benchmark(void (*f)(), int min_iterations,
                 int max_iterations, double accuracy);
extern int input_height;
extern int input_width;

extern Halide::Runtime::Buffer<uint16_t> in;
extern Halide::Runtime::Buffer<uint16_t> out;
extern Halide::Runtime::Buffer<uint8_t> in8;
extern Halide::Runtime::Buffer<uint8_t> out8;

void test_clean(void);
void test_fast(void);

#endif
