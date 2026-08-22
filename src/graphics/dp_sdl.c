/*
 * B-TRON Display Primitives SDL2 Host Integration: dp_sdl.c
 */

#include <btron/dp.h>
#include <SDL.h>
#include <stdio.h>

static SDL_Window   *g_sdl_window = NULL;
static SDL_Renderer *g_sdl_renderer = NULL;
static SDL_Texture  *g_sdl_texture = NULL;

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <SDL_syswm.h>
#define BOOL OBJC_BOOL_TEMP
#include <objc/message.h>
#include <objc/runtime.h>
#undef BOOL

static void macos_force_foreground_focus(void) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    ProcessSerialNumber psn;
    if (GetCurrentProcess(&psn) == 0) {
        TransformProcessType(&psn, kProcessTransformToForegroundApplication);
        SetFrontProcess(&psn);
    }
#pragma clang diagnostic pop

    id ns_app = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
    if (ns_app) {
        ((void (*)(id, SEL, long))objc_msgSend)(ns_app, sel_registerName("setActivationPolicy:"), 0);
        ((void (*)(id, SEL, int))objc_msgSend)(ns_app, sel_registerName("activateIgnoringOtherApps:"), 1);
    }

    id current_app = ((id (*)(id, SEL))objc_msgSend)((id)objc_getClass("NSRunningApplication"), sel_registerName("currentApplication"));
    if (current_app) {
        ((void (*)(id, SEL, unsigned long))objc_msgSend)(current_app, sel_registerName("activateWithOptions:"), 1UL << 1);
    }

    if (g_sdl_window) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(g_sdl_window, &wmInfo) && wmInfo.subsystem == SDL_SYSWM_COCOA) {
            id nswin = (id)wmInfo.info.cocoa.window;
            if (nswin) {
                ((void (*)(id, SEL, id))objc_msgSend)(nswin, sel_registerName("makeKeyAndOrderFront:"), NULL);
                id contentView = ((id (*)(id, SEL))objc_msgSend)(nswin, sel_registerName("contentView"));
                if (contentView) {
                    ((void (*)(id, SEL, id))objc_msgSend)(nswin, sel_registerName("makeFirstResponder:"), contentView);
                }
                printf("[TRACE-COCOA] Activated nswin=%p contentView=%p\n", (void*)nswin, (void*)contentView);
                fflush(stdout);
            }
        }
    }
}
#endif

void raise_sdl_window(void) {
    if (g_sdl_window) {
        SDL_RaiseWindow(g_sdl_window);
        SDL_SetWindowInputFocus(g_sdl_window);
        SDL_SetWindowGrab(g_sdl_window, SDL_TRUE);
        SDL_StartTextInput();
#ifdef __APPLE__
        macos_force_foreground_focus();
#endif
    }
}

BOOL init_sdl_backend(H width, H height, const char *title) {
#ifdef SDL_HINT_MAC_BACKGROUND_APP
    SDL_SetHint(SDL_HINT_MAC_BACKGROUND_APP, "0");
#endif
#ifdef SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif

#ifdef __APPLE__
    macos_force_foreground_focus();
#endif

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

    SDL_StartTextInput();
    raise_sdl_window();

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

static BOOL g_first_frame = TRUE;

void flush_gdev_to_sdl(GDEV *dev) {
    if (!dev || !g_sdl_renderer || !g_sdl_texture) return;

    if (g_first_frame) {
        g_first_frame = FALSE;
        raise_sdl_window();
    }

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
