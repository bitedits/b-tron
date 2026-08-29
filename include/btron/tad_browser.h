/*
 * B-TRON Specification Header: btron/tad_browser.h
 * Stream-oriented Native Binary & Symbolic TAD Document Browser.
 * Cleanroom implementation conforming to BTRON3 SPEC 3.20 & NASA JPL Scope.
 */

#ifndef _BTRON_TAD_BROWSER_H_
#define _BTRON_TAD_BROWSER_H_

#include <btron/types.h>
#include <btron/wnd.h>
#include <btron/dp.h>
#include <btron/vobj.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAD_MAX_SPANS   1024
#define TAD_MAX_LINKS   128
#define TAD_MAX_TITLE   128
#define TAD_MAX_PATH    256
#define TAD_MAX_HISTORY 32

/* Text Span Style Attributes */
typedef struct {
    UH font_id;     /* 0=Mincho, 1=Gothic, 2=Monospace */
    UH font_size;   /* 10, 12, 14, 16, 22 pt */
    UH weight;      /* 400=Regular, 700=Bold */
    COLOR color;    /* RGB Text Color */
    UH indent;      /* Left margin indent in pixels */
    UH line_pitch;  /* Line height in pixels */
    BOOL is_hr;     /* Horizontal vector separator */
    BOOL is_vobj;   /* Virtual Object Link */
    ID target_robj; /* Target Real Object ID if VOBJ */
    char vobj_label[64];
    char vobj_path[128];
} TAD_STYLE;

/* Computed Layout Span */
typedef struct {
    RECT bounds;            /* Visual bounding box (x, y, w, h) in document space */
    TAD_STYLE style;        /* Fusen styling */
    char text[256];         /* Text segment buffer */
    BOOL is_link;           /* Clickable VOBJ link */
} TAD_SPAN;

/* Browser Navigation History Item */
typedef struct {
    char path[TAD_MAX_PATH];
    int scroll_y;
} TAD_HISTORY_ENTRY;

/* TAD Browser State */
typedef struct {
    char doc_title[TAD_MAX_TITLE];
    char file_path[TAD_MAX_PATH];
    TAD_SPAN spans[TAD_MAX_SPANS];
    int span_count;

    int doc_height;         /* Total document pixel height */
    int doc_width;          /* Document pixel width */
    int scroll_y;           /* Viewport vertical scroll offset */
    int page_height;        /* Viewport visible height */

    /* Interactive Hyper-Data Links */
    int hovered_link_idx;   /* Currently mouse-hovered link span index (-1 if none) */
    int active_link_idx;    /* Clicked link span index */

    /* Navigation History Stack */
    TAD_HISTORY_ENTRY history[TAD_MAX_HISTORY];
    int history_count;
    int history_idx;

    BOOL is_binary_tad;     /* TRUE if parsed from binary TAD stream */
    UW total_bytes;         /* Document byte size */
} TAD_BROWSER;

/* Lifecycle & Window APIs */
void tad_browser_init(TAD_BROWSER *tb);
ER tad_browser_load_file(TAD_BROWSER *tb, const char *filepath);
ER tad_browser_load_buffer(TAD_BROWSER *tb, const void *buf, UW len, const char *doc_title);
void tad_browser_layout(TAD_BROWSER *tb, int view_width);
void tad_browser_paint(TAD_BROWSER *tb, GDEV *dev, const RECT *client_rect);
BOOL tad_browser_handle_mouse(TAD_BROWSER *tb, int mouse_x, int mouse_y, BOOL is_click, ID *out_clicked_robj, char *out_clicked_path);
void tad_browser_scroll(TAD_BROWSER *tb, int delta_y);

/* In-place Navigation & Path Resolution APIs */
void tad_browser_resolve_path(const char *current_path, const char *target, char *out_resolved, size_t max_len);
ER tad_browser_navigate(TAD_BROWSER *tb, const char *target_path);
BOOL tad_browser_can_go_back(const TAD_BROWSER *tb);
BOOL tad_browser_can_go_forward(const TAD_BROWSER *tb);
void tad_browser_go_back(TAD_BROWSER *tb);
void tad_browser_go_forward(TAD_BROWSER *tb);
void tad_browser_go_home(TAD_BROWSER *tb);
void tad_browser_reload(TAD_BROWSER *tb);

/* Open Standard Application Windows */
WND* open_tad_browser_window(const char *filepath, const char *title);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_TAD_BROWSER_H_ */
