/*
 * B-System (BTRON 3.20) Desktop Calculator (src/apps/calculator.c)
 * Scientific & Programmer Math Engine
 */

#include <btron/wnd.h>
#include <btron/dp.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#endif

typedef struct {
    double current_value;
    double accumulator;
    char op;
    char tape[1024];
} CalcState;

void calc_init(CalcState *st) {
    if (!st) return;
    memset(st, 0, sizeof(CalcState));
}
