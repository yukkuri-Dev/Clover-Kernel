// kernel/string.h
#pragma once
#include "types.h"

int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, uint64_t n);
int strlen(const char* s);