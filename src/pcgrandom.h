#ifndef PCG_RANDOM_H
#define PCG_RANDOM_H

#include <stdint.h>

void seed_rng(void);
uint32_t pcg_get_random(void);
uint32_t pcg_ranged_random(uint32_t range);

#endif
