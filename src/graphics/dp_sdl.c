/*
 * B-TRON Display Primitives SDL2 Host Integration: dp_sdl.c
 */

#include <btron/dp.h>
#include <SDL.h>
#include <stdio.h>

static SDL_Window   *g_sdl_window = NULL;
static SDL_Renderer *g_sdl_renderer = NULL;
static SDL_Texture  *g_sdl_texture = NULL;

BOOL init_sdl_backend(H width, H height, const char *title) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return FALSE;
    }

    g_sdl_window = SDL_CreateWindow(
        title ? title : "B-TRON Retro OS Desktop Environment",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!g_sdl_window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return FALSE;
    }

    g_sdl_renderer = SDL_CreateRenderer(
        g_sdl_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!g_sdl_renderer) {
        g_sdl_renderer = SDL_CreateRenderer(g_sdl_window, -1, 0);
    }

    g_sdl_texture = SDL_CreateTexture(
        g_sdl_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height
    );

    return TRUE;
}

void flush_gdev_to_sdl(GDEV *dev) {
    if (!dev || !g_sdl_renderer || !g_sdl_texture) return;

    SDL_UpdateTexture(g_sdl_texture, NULL, dev->pixels, dev->width * sizeof(COLOR));
    SDL_RenderClear(g_sdl_renderer);
    SDL_RenderCopy(g_sdl_renderer, g_sdl_texture, NULL, NULL);
    SDL_RenderPresent(g_sdl_renderer);
}

void shutdown_sdl_backend(void) {
    if (g_sdl_texture) SDL_DestroyTexture(g_sdl_texture);
    if (g_sdl_renderer) SDL_DestroyRenderer(g_sdl_renderer);
    if (g_sdl_window) SDL_DestroyWindow(g_sdl_window);
    SDL_Quit();
}
