/*
 * B-TRON HMI Standard Library: hmi.h
 * Component specification and API definitions based on TRON HMI Standard.
 *
 * Designed for MISRA-C compliance and deterministic memory usage.
 */

#ifndef _BTRON_HMI_H_
#define _BTRON_HMI_H_

#include <btron/types.h>
#include <btron/error.h>
#include <btron/dp.h>
#include <btron/event.h>
#include <btron/wnd.h>
#include <btron/troncode.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Control Types ─────────────────────────────────────────────── */
typedef enum {
    HMI_TYPE_NONE = 0,
    HMI_TYPE_PUSH_SWITCH,       /* Momentary Push Switch (モーメンタリ) */
    HMI_TYPE_TOGGLE_SWITCH,     /* Alternate / Push-Lock Switch (オルタネート) */
    HMI_TYPE_STANDARD_TRIAD,    /* Cancel / Default / Start Switch Group */
    HMI_TYPE_UPDOWN_SELECTOR,   /* Up/Down Step Selector (アップダウンセレクタ) */
    HMI_TYPE_RADIO_SELECTOR,    /* Exclusive Radio Selector Matrix */
    HMI_TYPE_ROTARY_SELECTOR,   /* Multi-position Rotary Switch */
    HMI_TYPE_SLIDER_VOLUME,     /* Linear Slide Volume (スライドボリューム) */
    HMI_TYPE_DIAL_VOLUME,       /* Rotary Knob Volume (ロータリボリューム) */
    HMI_TYPE_BAR_METER,         /* Level Bar Meter (レベルメータ) */
    HMI_TYPE_STATUS_LED,        /* 3-Color Status Indicator (3色インジケータ) */
    HMI_TYPE_DIGITAL_DISPLAY,   /* 7-Segment / Numeric Display */
    HMI_TYPE_UNIVERSAL_PAD      /* TRON Universal Controller (万能コントローラ) */
} HMI_CTRL_TYPE;

/* ── Trigger Modes (Chapter 9: Touch Edge vs Release Edge) ──────── */
typedef enum {
    HMI_TRIGGER_TOUCH_EDGE = 0, /* Activates immediately on press down */
    HMI_TRIGGER_RELEASE_EDGE = 1 /* Activates on button release */
} HMI_TRIGGER_MODE;

/* ── 3-Color Indicator Colors (Chapter 9) ──────────────────────── */
typedef enum {
    HMI_LED_OFF = 0,
    HMI_LED_GREEN,              /* Normal Operation / Safe (緑) */
    HMI_LED_YELLOW,             /* Caution / Standby / Warning (黄/橙) */
    HMI_LED_RED                 /* Stop / Error / Emergency (赤) */
} HMI_LED_COLOR;

/* ── Universal Controller Keys (Chapter 7) ─────────────────────── */
typedef enum {
    HMI_KEY_NONE = 0,
    HMI_KEY_PREV_ITEM,          /* [▲/前] Select Previous Item */
    HMI_KEY_NEXT_ITEM,          /* [▼/次] Select Next Item */
    HMI_KEY_DEC_VALUE,          /* [◄/-] Decrease Value */
    HMI_KEY_INC_VALUE,          /* [►/+] Increase Value */
    HMI_KEY_EXECUTE,            /* [O] Execute / Start / OK */
    HMI_KEY_CANCEL,             /* [X] Cancel / Reset */
    HMI_KEY_COMMAND             /* [命名] Menu / Command */
} HMI_UNIVERSAL_KEY;

/* ── Control State Flags ───────────────────────────────────────── */
#define HMI_STATE_ACTIVE    (1 << 0)
#define HMI_STATE_FOCUSED   (1 << 1)  /* Current Item (カレント項目) */
#define HMI_STATE_PRESSED   (1 << 2)
#define HMI_STATE_DISABLED  (1 << 3)
#define HMI_STATE_CHECKED   (1 << 4)
#define HMI_STATE_ENABLEWARE (1 << 5) /* Tactile dot (ボッチ) enabled */

/* Forward Declarations */
struct HMI_CTRL;
struct HMI_PANEL;

typedef void (*HMI_CALLBACK)(struct HMI_CTRL *ctrl, struct HMI_PANEL *panel, void *user_data);

/* ── Generic HMI Control Descriptor ────────────────────────────── */
typedef struct HMI_CTRL {
    ID               id;
    HMI_CTRL_TYPE    type;
    RECT             bounds;
    UW               flags;
    HMI_TRIGGER_MODE trigger;
    char             label[64];
    char             unit[16];

    /* Values & Bounds */
    int              val;
    int              min_val;
    int              max_val;
    int              step;

    /* Options / Items for Selectors */
    int              num_options;
    const char       *options[16];

    /* Dynamic Visuals */
    HMI_LED_COLOR    led_color;
    COLOR            accent_col;
    int              peak_val;

    /* Event Callback */
    HMI_CALLBACK     on_change;
    void             *user_data;
} HMI_CTRL;

/* ── Modeless HMI Panel Container (Max 32 controls per panel) ──── */
#define HMI_PANEL_MAX_CTRLS 32

typedef struct HMI_PANEL {
    ID          id;
    char        title[64];
    RECT        bounds;
    COLOR       bg_color;
    HMI_CTRL    controls[HMI_PANEL_MAX_CTRLS];
    int         num_controls;
    int         focused_index;  /* Current Item (カレント項目) */
    BOOL        show_remote;    /* Whether Universal Remote is visible */
    void        (*on_paint_custom)(struct HMI_PANEL *panel, GDEV *dev);
} HMI_PANEL;

/* ── Core Lifecycle & Event APIs ───────────────────────────────── */
ER   hmi_init_panel(HMI_PANEL *panel, const char *title, H x, H y, H w, H h, COLOR bg_col);
ER   hmi_draw_panel(HMI_PANEL *panel, GDEV *dev);
BOOL hmi_dispatch_event(HMI_PANEL *panel, const EVT *evt);
ER   hmi_set_focus(HMI_PANEL *panel, int ctrl_index);
ER   hmi_focus_next(HMI_PANEL *panel);
ER   hmi_focus_prev(HMI_PANEL *panel);

/* ── Universal Controller Remote API ───────────────────────────── */
ER   hmi_handle_universal_key(HMI_PANEL *panel, HMI_UNIVERSAL_KEY key);
void hmi_draw_universal_remote(GDEV *dev, H x, H y, H w, H h, HMI_UNIVERSAL_KEY pressed_key);
BOOL hmi_remote_hit_test(H rx, H ry, H x, H y, HMI_UNIVERSAL_KEY *out_key);

/* ── Specific Control Factory APIs ─────────────────────────────── */
HMI_CTRL* hmi_add_push_switch(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, HMI_CALLBACK cb);
HMI_CTRL* hmi_add_toggle_switch(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, BOOL init_state, HMI_CALLBACK cb);
HMI_CTRL* hmi_add_standard_triad(HMI_PANEL *p, ID id, H x, H y, H w, H h, HMI_CALLBACK on_cancel, HMI_CALLBACK on_default, HMI_CALLBACK on_start);
HMI_CTRL* hmi_add_updown_selector(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int min_v, int max_v, int init_v, const char *unit, HMI_CALLBACK cb);
HMI_CTRL* hmi_add_radio_selector(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int num_opts, const char **opts, int init_idx, HMI_CALLBACK cb);
HMI_CTRL* hmi_add_slider_volume(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int min_v, int max_v, int init_v, HMI_CALLBACK cb);
HMI_CTRL* hmi_add_dial_volume(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int min_v, int max_v, int init_v, HMI_CALLBACK cb);
HMI_CTRL* hmi_add_bar_meter(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int min_v, int max_v);
HMI_CTRL* hmi_add_status_led(HMI_PANEL *p, ID id, const char *label, H x, H y, HMI_LED_COLOR init_color);
HMI_CTRL* hmi_add_digital_display(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, const char *init_text);
HMI_CTRL* hmi_add_rotary_selector(HMI_PANEL *p, ID id, const char *label, H x, H y, H w, H h, int num_positions, int init_pos, HMI_CALLBACK cb);
HMI_CTRL* hmi_add_universal_pad(HMI_PANEL *p, ID id, H x, H y, H w, H h, HMI_CALLBACK cb);

/* ── Control Rendering Helpers ─────────────────────────────────── */
void hmi_draw_push_switch(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_toggle_switch(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_standard_triad(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_updown_selector(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_radio_selector(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_slider_volume(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_dial_volume(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_bar_meter(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_status_led(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_digital_display(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_rotary_selector(GDEV *dev, const HMI_CTRL *ctrl);
void hmi_draw_universal_pad(GDEV *dev, const HMI_CTRL *ctrl);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_HMI_H_ */
