#pragma once
#include "../kernelstd/types.h"
#include "task.h"

void scheduler_init();
void scheduler_add_task(const char* name, void (*entry)(), uint64_t stack_size);
void schedule();
int scheduler_get_task_count();
task_t* scheduler_get_task(int idx);