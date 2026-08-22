/*
 * B-TRON System Event Queue: event.c
 */

#include <btron/event.h>
#include <SDL.h>
#include <stdlib.h>
#include <string.h>

#define EVENT_QUEUE_SIZE 64

static EVT g_queue[EVENT_QUEUE_SIZE];
static int g_q_head = 0;
static int g_q_tail = 0;
static int g_q_count = 0;

ER init_evt_sys(void) {
    g_q_head = 0;
    g_q_tail = 0;
    g_q_count = 0;
    return E_OK;
}

ER snd_evt(const EVT *p_evt) {
    if (!p_evt) return E_PAR;
    if (g_q_count >= EVENT_QUEUE_SIZE) return E_BUSY;

    g_queue[g_q_tail] = *p_evt;
    g_q_tail = (g_q_tail + 1) % EVENT_QUEUE_SIZE;
    g_q_count++;
    return E_OK;
}

ER get_evt(EVT *p_evt, W timeout_ms) {
    if (!p_evt) return E_PAR;
    (void)timeout_ms;

    SDL_Event sdlev;
    while (SDL_PollEvent(&sdlev)) {
        EVT ev;
        memset(&ev, 0, sizeof(EVT));

        switch (sdlev.type) {
            case SDL_QUIT:
                ev.type = EV_WND_CLOSE;
                snd_evt(&ev);
                break;
            case SDL_MOUSEBUTTONDOWN:
                ev.type = EV_BUT_DOWN;
                ev.pos.x = sdlev.button.x;
                ev.pos.y = sdlev.button.y;
                ev.button = sdlev.button.button;
                snd_evt(&ev);
                break;
            case SDL_MOUSEBUTTONUP:
                ev.type = EV_BUT_UP;
                ev.pos.x = sdlev.button.x;
                ev.pos.y = sdlev.button.y;
                ev.button = sdlev.button.button;
                snd_evt(&ev);
                break;
            case SDL_MOUSEMOTION:
                ev.type = EV_MOUSE_MOVE;
                ev.pos.x = sdlev.motion.x;
                ev.pos.y = sdlev.motion.y;
                snd_evt(&ev);
                break;
            case SDL_KEYDOWN:
                ev.type = EV_KEY_DOWN;
                ev.key = sdlev.key.keysym.sym;
                snd_evt(&ev);
                break;
            default:
                break;
        }
    }

    if (g_q_count > 0) {
        *p_evt = g_queue[g_q_head];
        g_q_head = (g_q_head + 1) % EVENT_QUEUE_SIZE;
        g_q_count--;
        return E_OK;
    }

    p_evt->type = EV_NONE;
    return E_TMOUT;
}
