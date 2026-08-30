/*
 * B-TRON HMI Canonical Showcase: audio_player.c
 * Classic SONY-Style Hi-Fi Stereo Audio Deck & Player (TC-K777ES / TRON HMI).
 *
 * Implements SUI/GUI components from TRON HMI Standard Handbook:
 * - Chapter 7: Input source matrix & Universal Controller remote control
 * - Chapter 11: Tape transport buttons alignment & Tone rotary dials
 * - Chapter 5: Dual L/R Peak-Hold VU Bar Meter & Digital Track counter
 */

#include <btron/apps.h>
#include <btron/hmi.h>
#include <btron/wnd.h>
#include <btron/dp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define AUDIO_WND_W 560
#define AUDIO_WND_H 370

/* Audio Deck State */
typedef enum {
    TAPE_STOP = 0,
    TAPE_PLAY,
    TAPE_REV_PLAY,
    TAPE_FF,
    TAPE_REW,
    TAPE_PAUSE
} TAPE_STATE;

typedef struct {
    HMI_PANEL  panel;
    TAPE_STATE state;
    int        source_idx;     /* 0: CD, 1: DAT, 2: TAPE 1, 3: TAPE 2, 4: TUNER, 5: LINE */
    int        track_num;
    int        seconds_elapsed;
    int        vu_left;
    int        vu_right;
    int        vu_peak_l;
    int        vu_peak_r;
    int        tape_reel_angle;
    BOOL       power_on;
    BOOL       dolby_nr;
} AudioDeck;

static AudioDeck g_deck;
static WND *g_audio_wnd = NULL;

/* Sources */
static const char *k_sources[] = { "CD", "DAT", "Tape 1", "Tape 2", "Tuner", "Line" };

/* Callbacks */
static void on_power_toggle(HMI_CTRL *ctrl, HMI_PANEL *p, void *data) {
    (void)p; (void)data;
    g_deck.power_on = (ctrl->val != 0);
    if (!g_deck.power_on) {
        g_deck.state = TAPE_STOP;
        g_deck.vu_left = 0;
        g_deck.vu_right = 0;
    }
}

static void on_source_changed(HMI_CTRL *ctrl, HMI_PANEL *p, void *data) {
    (void)p; (void)data;
    g_deck.source_idx = ctrl->val;
}

static void on_transport_click(HMI_CTRL *ctrl, HMI_PANEL *p, void *data) {
    (void)p; (void)data;
    if (!g_deck.power_on) return;

    if (strcmp(ctrl->label, "▶") == 0) {
        g_deck.state = TAPE_PLAY;
    } else if (strcmp(ctrl->label, "◀") == 0) {
        g_deck.state = TAPE_REV_PLAY;
    } else if (strcmp(ctrl->label, "▶▶") == 0) {
        g_deck.state = TAPE_FF;
    } else if (strcmp(ctrl->label, "◀◀") == 0) {
        g_deck.state = TAPE_REW;
    } else if (strcmp(ctrl->label, "■") == 0) {
        g_deck.state = TAPE_STOP;
        g_deck.vu_left = 0;
        g_deck.vu_right = 0;
    } else if (strcmp(ctrl->label, "❚❚") == 0) {
        g_deck.state = (g_deck.state == TAPE_PAUSE) ? TAPE_PLAY : TAPE_PAUSE;
    }
}

static void on_remote_toggle(HMI_CTRL *ctrl, HMI_PANEL *p, void *data) {
    (void)ctrl; (void)data;
    p->show_remote = !p->show_remote;
}

/* Custom Deck Paint (Cassette bay & metallic brushed finish) */
static void paint_sony_deck(HMI_PANEL *panel, GDEV *dev) {
    (void)panel;
    if (!dev) return;

    /* 1. Brushed Aluminum Chassis Bevel */
    RECT header_bar = { 10, 10, AUDIO_WND_W - 10, 36 };
    fill_rec(dev, &header_bar, COLOR_BLACK);
    drw_rec(dev, &header_bar);
    drw_tc_string(dev, 16, 16, "【 SONY 】STEREO CASSETTE DECK  TC-K777ES", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, AUDIO_WND_W - 180, 16, "TRON HMI STANDARD", COLOR_GREEN, 0x00000000);

    /* 2. Cassette Window Well */
    RECT cass_box = { 20, 50, 200, 175 };
    fill_rec(dev, &cass_box, COLOR_DKGRAY);
    drw_rec(dev, &cass_box);

    /* Cassette View Window */
    RECT cass_win = { 40, 75, 180, 150 };
    fill_rec(dev, &cass_win, COLOR_BLACK);
    drw_rec(dev, &cass_win);

    /* Left & Right Spindles */
    RECT sp1 = { 75 - 18, 112 - 18, 75 + 18, 112 + 18 };
    RECT sp2 = { 145 - 18, 112 - 18, 145 + 18, 112 + 18 };
    drw_ovl(dev, &sp1);
    drw_ovl(dev, &sp2);

    if (g_deck.state == TAPE_PLAY || g_deck.state == TAPE_FF) {
        g_deck.tape_reel_angle = (g_deck.tape_reel_angle + 15) % 360;
    } else if (g_deck.state == TAPE_REV_PLAY || g_deck.state == TAPE_REW) {
        g_deck.tape_reel_angle = (g_deck.tape_reel_angle - 15 + 360) % 360;
    }

    /* Spindle Teeth */
    drw_lin(dev, 75 - 12, 112, 75 + 12, 112);
    drw_lin(dev, 75, 112 - 12, 75, 112 + 12);
    drw_lin(dev, 145 - 12, 112, 145 + 12, 112);
    drw_lin(dev, 145, 112 - 12, 145, 112 + 12);

    /* Cassette Label */
    drw_tc_string(dev, 55, 80, "SONY HF-ES 90", COLOR_WHITE, 0x00000000);

    /* 3. VFD Fluorescent Display Area */
    RECT vfd_box = { 215, 50, 410, 125 };
    fill_rec(dev, &vfd_box, COLOR_BLACK);
    drw_rec(dev, &vfd_box);

    /* Track / Time Readout */
    char t_buf[32];
    if (g_deck.power_on) {
        int m = g_deck.seconds_elapsed / 60;
        int s = g_deck.seconds_elapsed % 60;
        snprintf(t_buf, sizeof(t_buf), "TR-0%d  %02d:%02d", g_deck.track_num, m, s);
        drw_tc_string(dev, 225, 58, t_buf, COLOR_CYAN, 0x00000000);
        drw_tc_string(dev, 340, 58, k_sources[g_deck.source_idx], COLOR_GREEN, 0x00000000);
    } else {
        drw_tc_string(dev, 225, 58, "--:--  POWER OFF", COLOR_DKGRAY, 0x00000000);
    }

    /* VU Meter labels */
    drw_tc_string(dev, 220, 84, "L", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, 220, 102, "R", COLOR_WHITE, 0x00000000);
    drw_tc_string(dev, 235, 74, "-20 -10 -5  0 +3 +5dB", COLOR_GRAY, 0x00000000);

    /* 4. Section Separator Lines */
    drw_lin(dev, 10, 245, AUDIO_WND_W - 10, 245);
    drw_lin(dev, 425, 45, 425, 240);

    /* Section Labels */
    drw_tc_string(dev, 440, 48, "入力選択 (Source)", COLOR_BLACK, 0x00000000);
    drw_tc_string(dev, 20, 252, "音質・音量調整 (Tone & Volume)", COLOR_BLACK, 0x00000000);

    /* 5. Draw Universal Controller Remote Overlay if active */
    if (g_deck.panel.show_remote) {
        hmi_draw_universal_remote(dev, AUDIO_WND_W - 145, 60, 125, 190, HMI_KEY_NONE);
    }
}

/* Event Handler for Audio Window */
static void audio_deck_event_handler(WND *wnd, const EVT *evt) {
    (void)wnd;
    if (!evt) return;

    /* Handle remote hit testing if remote overlay is open */
    if (g_deck.panel.show_remote && evt->type == EV_BUT_DOWN) {
        HMI_UNIVERSAL_KEY rkey = HMI_KEY_NONE;
        if (hmi_remote_hit_test(AUDIO_WND_W - 145, 60, evt->pos.x, evt->pos.y, &rkey)) {
            hmi_handle_universal_key(&g_deck.panel, rkey);
            inval_wnd(wnd);
            return;
        }
    }

    /* Dispatch to standard HMI controls */
    if (hmi_dispatch_event(&g_deck.panel, evt)) {
        inval_wnd(wnd);
    }
}

/* Paint Handler for Audio Window */
static void audio_deck_paint(WND *wnd, GDEV *dev) {
    (void)wnd;
    if (!dev) return;

    /* Simulate Audio Physics if Playing */
    if (g_deck.power_on && (g_deck.state == TAPE_PLAY || g_deck.state == TAPE_REV_PLAY)) {
        g_deck.seconds_elapsed++;
        g_deck.vu_left = 40 + (rand() % 55);
        g_deck.vu_right = 38 + (rand() % 58);
        if (g_deck.vu_left > g_deck.vu_peak_l) g_deck.vu_peak_l = g_deck.vu_left;
        else if (g_deck.vu_peak_l > 0) g_deck.vu_peak_l -= 2;

        if (g_deck.vu_right > g_deck.vu_peak_r) g_deck.vu_peak_r = g_deck.vu_right;
        else if (g_deck.vu_peak_r > 0) g_deck.vu_peak_r -= 2;

        /* Update VU meter controls */
        for (int i = 0; i < g_deck.panel.num_controls; i++) {
            HMI_CTRL *c = &g_deck.panel.controls[i];
            if (c->id == 100) { c->val = g_deck.vu_left; c->peak_val = g_deck.vu_peak_l; }
            if (c->id == 101) { c->val = g_deck.vu_right; c->peak_val = g_deck.vu_peak_r; }
        }
    }

    hmi_draw_panel(&g_deck.panel, dev);
}

static void destroy_audio_player(WND *wnd) {
    (void)wnd;
    g_audio_wnd = NULL;
}

/* Open Audio Deck Application Window */
WND* open_audio_player_window(void) {
    if (g_audio_wnd) {
        top_wnd(g_audio_wnd);
        return g_audio_wnd;
    }

    /* Initialize Deck State */
    memset(&g_deck, 0, sizeof(AudioDeck));
    g_deck.power_on = TRUE;
    g_deck.state = TAPE_STOP;
    g_deck.track_num = 3;
    g_deck.seconds_elapsed = 165; /* 02:45 */
    g_deck.source_idx = 0; /* CD */

    hmi_init_panel(&g_deck.panel, "SONY TC-K777ES", 0, 0, AUDIO_WND_W, AUDIO_WND_H, COLOR_LTGRAY);
    g_deck.panel.on_paint_custom = paint_sony_deck;

    /* 1. Power Switch */
    hmi_add_toggle_switch(&g_deck.panel, 1, "POWER", 20, 200, 70, 26, TRUE, on_power_toggle);

    /* 2. Remote Toggle Button */
    hmi_add_push_switch(&g_deck.panel, 2, "万能リモコン", 100, 200, 100, 26, on_remote_toggle);

    /* 3. VU Meters L and R */
    hmi_add_bar_meter(&g_deck.panel, 100, "VU_L", 235, 84, 160, 14, 0, 100);
    hmi_add_bar_meter(&g_deck.panel, 101, "VU_R", 235, 102, 160, 14, 0, 100);

    /* 4. Transport Bar (図11-11 standard layout) */
    H tx = 215, ty = 140, tw = 36, th = 32;
    hmi_add_push_switch(&g_deck.panel, 10, "◀◀", tx, ty, tw, th, on_transport_click);
    hmi_add_push_switch(&g_deck.panel, 11, "◀",  tx + 40, ty, tw, th, on_transport_click);
    hmi_add_push_switch(&g_deck.panel, 12, "▶",  tx + 80, ty, tw, th, on_transport_click);
    hmi_add_push_switch(&g_deck.panel, 13, "▶▶", tx + 120, ty, tw, th, on_transport_click);
    hmi_add_push_switch(&g_deck.panel, 14, "■",  tx + 160, ty, tw, th, on_transport_click);
    hmi_add_push_switch(&g_deck.panel, 15, "❚❚", tx + 195, ty, tw, th, on_transport_click);

    /* 5. Input Source Selector (図7-9) */
    hmi_add_radio_selector(&g_deck.panel, 20, "INPUT", 435, 68, 105, 160, 6, k_sources, 0, on_source_changed);

    /* 6. Tone & Volume Dials (Chapter 11) */
    hmi_add_dial_volume(&g_deck.panel, 30, "BASS", 20, 275, 75, 75, -5, 5, 0, NULL);
    hmi_add_dial_volume(&g_deck.panel, 31, "TREBLE", 105, 275, 75, 75, -5, 5, 0, NULL);
    hmi_add_dial_volume(&g_deck.panel, 32, "BALANCE", 190, 275, 75, 75, -5, 5, 0, NULL);
    hmi_add_dial_volume(&g_deck.panel, 33, "VOLUME", 280, 265, 95, 95, 0, 10, 6, NULL);

    /* 7. Standard Triad System Controls (Chapter 6 & 11) */
    hmi_add_standard_triad(&g_deck.panel, 40, 390, 290, 150, 30, NULL, NULL, NULL);

    /* Open Window */
    g_audio_wnd = opn_wnd("【SONY】TC-K777ES ステレオカセットデッキ", 160, 120, AUDIO_WND_W, AUDIO_WND_H,
                          WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_BORDER);
    if (!g_audio_wnd) return NULL;

    g_audio_wnd->paint = audio_deck_paint;
    g_audio_wnd->event_handler = audio_deck_event_handler;
    g_audio_wnd->destroy = destroy_audio_player;

    return g_audio_wnd;
}
