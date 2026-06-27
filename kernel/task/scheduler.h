#pragma once
#include "../kernelstd/types.h"
#include "task.h"

void scheduler_init();
void scheduler_add_task(const char* name, void (*entry)(), uint64_t stack_size);
void scheduler_add_task_user(const char* name, void (*entry)(), uint64_t stack_size);
task_t* schedule_next(task_t** out_prev);
task_t* get_current_task();
void task_exit();
int scheduler_get_task_count();
task_t* scheduler_get_task(int idx);