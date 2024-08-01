#ifndef COAL_HELPER_H
#define COAL_HELPER_H

#include <stdint.h>

extern void cm_spiral_loop(unsigned int width, unsigned int height,
                           void (*func)(unsigned int x, unsigned int y));
extern uint32_t cm_trailing_zeros(uint64_t n);
extern uint32_t cm_trailing_ones(uint64_t n);
extern uint32_t cm_pow2(uint32_t n);
#endif //COAL_HELPER_H
