#pragma once
#include "../kernelstd/types.h"

#define PIT_FREQUENCY 1193182
#define TARGET_HZ     100
#define DIVISOR       (PIT_FREQUENCY / TARGET_HZ)

void pit_init();
uint64_t pit_get_ticks();