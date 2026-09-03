/*
 * B-System (BTRON 3.20) Music & Audio Synthesis (src/apps/music.c)
 * Real-Time Audio Engine & MIDI Tracker
 */

#include <btron/wnd.h>
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif

typedef struct {
    int bpm;
    int tracks;
    int sample_rate;
} MusicEngineState;
