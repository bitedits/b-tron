/*
 * B-System (BTRON 3.20) Automated Headless Window Screenshot Capturer
 * src/tools/capture_screens.c
 *
 * Renders authentic C99 windows directly onto a headless GDEV framebuffer,
 * extracts pixel-perfect isolated window bounding boxes (including 3D bevels,
 * sliding tabs, fonts, and controls) with transparent background around the window,
 * and dumps raw ARGB frames for PNG conversion.
 *
 * File Naming Convention:
 * - Settings screens:    <Name>_Settings.png
 * - Application screens: <Name>_Application.png
 * - System overlays:     GlobalMenu.png
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <btron/types.h>
#include <btron/dp.h>
#include <btron/wnd.h>
#include <btron/settings.h>
#include <btron/language_settings.h>

/* Forward declarations for Settings applet window openers */
extern WND* open_control_panel_window(void);
extern WND* open_appearance_settings_window(void);
extern WND* open_desktop_settings_window(void);
extern WND* open_display_settings_window(void);
extern WND* open_input_settings_window(void);
extern WND* open_language_settings_window(void);
extern WND* open_media_settings_window(void);
extern WND* open_network_settings_window(void);
extern WND* open_security_settings_window(void);
extern WND* open_sound_settings_window(void);
extern WND* open_system_settings_window(void);
extern WND* open_terminal_settings_window(void);

/* Forward declarations for Applications window openers */
extern WND* open_vobj_manager_window(void);
extern WND* open_t_editor_window(void);
extern WND* open_tad_browser_window(const char *filepath, const char *title);
extern WND* open_audio_player_window(void);
extern WND* launch_beos_chat(void);
extern WND* open_about_window(void);
extern void global_menu_render_bar(GDEV *dev);

/* Helper to dump raw ARGB rectangle to file */
static void dump_window_rect(GDEV *dev, WND *wnd, const char *out_filename) {
    if (!dev || !wnd) return;

    /* Window total geometry including title tab & 3D borders */
    int x0 = wnd->bounds.left - 4;
    int y0 = wnd->bounds.top - 30;
    int x1 = wnd->bounds.right + 6;
    int y1 = wnd->bounds.bottom + 6;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > dev->width) x1 = dev->width;
    if (y1 > dev->height) y1 = dev->height;

    int crop_w = x1 - x0;
    int crop_h = y1 - y0;

    FILE *fp = fopen(out_filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open output file: %s\n", out_filename);
        return;
    }

    /* Write 8-byte header: width (4 bytes LE), height (4 bytes LE) */
    fwrite(&crop_w, sizeof(int), 1, fp);
    fwrite(&crop_h, sizeof(int), 1, fp);

    for (int y = y0; y < y1; y++) {
        COLOR *row = &dev->pixels[y * dev->width + x0];
        fwrite(row, sizeof(COLOR), crop_w, fp);
    }

    fclose(fp);
    printf("  [CAPTURED] %-30s -> %s (%dx%d px)\n", wnd->title, out_filename, crop_w, crop_h);
}

static void dump_raw_region(GDEV *dev, int x0, int y0, int crop_w, int crop_h, const char *name, const char *out_filename) {
    if (!dev) return;

    FILE *fp = fopen(out_filename, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open output file: %s\n", out_filename);
        return;
    }

    fwrite(&crop_w, sizeof(int), 1, fp);
    fwrite(&crop_h, sizeof(int), 1, fp);

    for (int y = y0; y < y0 + crop_h; y++) {
        COLOR *row = &dev->pixels[y * dev->width + x0];
        fwrite(row, sizeof(COLOR), crop_w, fp);
    }

    fclose(fp);
    printf("  [CAPTURED] %-30s -> %s (%dx%d px)\n", name, out_filename, crop_w, crop_h);
}

typedef struct {
    const char *name;
    WND* (*open_fn)(void);
} WINDOW_TARGET;

static void clear_canvas_transparent(GDEV *dev) {
    if (!dev || !dev->pixels) return;
    memset((void*)dev->pixels, 0, dev->width * dev->height * sizeof(COLOR));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("==========================================================\n");
    printf(" B-System Automated Headless Window Screenshot Capturer\n");
    printf(" (_Settings and _Application Naming Standard)\n");
    printf("==========================================================\n");

    if (system("mkdir -p b-system/img/screens /tmp/btron_raw_screens") != 0) {
        /* Ignore error */
    }

    /* Create 1280x800 desktop canvas */
    GDEV *dev = opn_dev(1280, 800);
    if (!dev) {
        fprintf(stderr, "Error: Failed to allocate 1280x800 framebuffer\n");
        return 1;
    }
    init_wnd_mgr(dev);

    /* 1. Settings Applet Windows (_Settings suffix) */
    WINDOW_TARGET settings_targets[] = {
        { "Appearance_Settings", open_appearance_settings_window },
        { "Desktop_Settings", open_desktop_settings_window },
        { "Display_Settings", open_display_settings_window },
        { "Input_Settings", open_input_settings_window },
        { "Language_Settings", open_language_settings_window },
        { "Media_Settings", open_media_settings_window },
        { "Network_Settings", open_network_settings_window },
        { "Security_Settings", open_security_settings_window },
        { "Sound_Settings", open_sound_settings_window },
        { "System_Settings", open_system_settings_window },
        { "Terminal_Settings", open_terminal_settings_window },
        { "Preferences_Settings", open_control_panel_window },
        { NULL, NULL }
    };

    for (int i = 0; settings_targets[i].name != NULL; i++) {
        clear_canvas_transparent(dev);

        WND *w = settings_targets[i].open_fn();
        if (!w) {
            fprintf(stderr, "Error opening window: %s\n", settings_targets[i].name);
            continue;
        }

        redraw_all_windows();

        char raw_path[256];
        snprintf(raw_path, sizeof(raw_path), "/tmp/btron_raw_screens/%s.raw", settings_targets[i].name);
        dump_window_rect(dev, w, raw_path);

        cls_wnd(w);
    }

    /* 2. Core Application Windows (_Application suffix) */
    {
        /* Cabinet Application Window */
        clear_canvas_transparent(dev);
        WND *w_cab = open_vobj_manager_window();
        if (w_cab) {
            redraw_all_windows();
            dump_window_rect(dev, w_cab, "/tmp/btron_raw_screens/Cabinet_Application.raw");
            cls_wnd(w_cab);
        }

        /* T-Editor Application Window */
        clear_canvas_transparent(dev);
        WND *w_ted = open_t_editor_window();
        if (w_ted) {
            redraw_all_windows();
            dump_window_rect(dev, w_ted, "/tmp/btron_raw_screens/TEditor_Application.raw");
            cls_wnd(w_ted);
        }

        /* TAD Browser Application Window */
        clear_canvas_transparent(dev);
        WND *w_brw = open_tad_browser_window(NULL, "BTRON3 3.20 OS Specification");
        if (w_brw) {
            redraw_all_windows();
            dump_window_rect(dev, w_brw, "/tmp/btron_raw_screens/Browser_Application.raw");
            cls_wnd(w_brw);
        }

        /* Cassette Application Window */
        clear_canvas_transparent(dev);
        WND *w_cas = open_audio_player_window();
        if (w_cas) {
            redraw_all_windows();
            dump_window_rect(dev, w_cas, "/tmp/btron_raw_screens/Cassette_Application.raw");
            cls_wnd(w_cas);
        }

        /* Chat Application Window */
        clear_canvas_transparent(dev);
        WND *w_cht = launch_beos_chat();
        if (w_cht) {
            redraw_all_windows();
            dump_window_rect(dev, w_cht, "/tmp/btron_raw_screens/Chat_Application.raw");
            cls_wnd(w_cht);
        }

        /* About Application Dialog Window */
        clear_canvas_transparent(dev);
        WND *w_abt = open_about_window();
        if (w_abt) {
            redraw_all_windows();
            dump_window_rect(dev, w_abt, "/tmp/btron_raw_screens/About_Application.raw");
            cls_wnd(w_abt);
        }

        /* Global Menu Bar & Tracker Top Strip */
        clear_canvas_transparent(dev);
        global_menu_render_bar(dev);
        dump_raw_region(dev, 0, 0, 1280, 48, "Global System Menu Bar", "/tmp/btron_raw_screens/GlobalMenu.raw");
    }

    cls_dev(dev);
    printf("==========================================================\n");
    printf(" Successfully captured all real BTRON windows to raw dumps!\n");
    printf("==========================================================\n");
    return 0;
}
