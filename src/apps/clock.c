/*
 * B-System (BTRON 3.20) Desktop Clock & Timer (src/apps/clock.c)
 * Analog & Digital Chronometer
 */

#include <btron/wnd.h>
#include <btron/dp.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <time.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif

typedef struct {
    int hours;
    int minutes;
    int seconds;
    BOOL show_analog;
} ClockState;
