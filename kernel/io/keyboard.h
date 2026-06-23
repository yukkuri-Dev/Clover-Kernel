#pragma once
#include "../kernelstd/types.h"

void keyboard_init();
char keyboard_getchar();  // 1文字読む（ブロッキング）