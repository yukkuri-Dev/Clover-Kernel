#pragma once
#include "../kernelstd/types.h"
#include "task.h"

void scheduler_init();
void scheduler_add_task(void (*entry)(), uint64_t stack_size);
void schedule();