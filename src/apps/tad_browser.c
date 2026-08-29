/*
 * B-TRON Application: Native TAD Document Browser (tad_browser.c)
 * Cleanroom implementation conforming to BTRON3 SPEC 3.20 & NASA JPL Scope.
 */

#include <btron/tad_browser.h>
#include <btron/troncode.h>
#include <btron/event.h>
#include <btron/error.h>

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#define memset  tkl_memset
#define memcpy  tkl_memcpy
#define strlen  tkl_strlen
#define strncpy tkl_strncpy
#define snprintf snprintf
#endif

/* Global Browser Instance for standalone window */
static TAD_BROWSER g_active_browser;

void tad_browser_init(TAD_BROWSER *tb) {
    if (!tb) return;
    memset(tb, 0, sizeof(TAD_BROWSER));
    strncpy(tb->doc_title, "BTRON TAD Document", sizeof(tb->doc_title) - 1);
    tb->hovered_link_idx = -1;
    tb->active_link_idx = -1;
    tb->page_height = 400;
    tb->doc_width = 640;
}

static void parse_text_tad_lines(TAD_BROWSER *tb, const char *text, UW len) {
    TAD_STYLE cur_style;
    memset(&cur_style, 0, sizeof(cur_style));
    cur_style.font_id = 0;       /* Mincho */
    cur_style.font_size = 12;
    cur_style.weight = 400;
    cur_style.color = COLOR_BLACK;
    cur_style.line_pitch = 20;
    cur_style.indent = 10;

    UW pos = 0;
    while (pos < len && tb->span_count < TAD_MAX_SPANS) {
        /* Read line */
        char line[256];
        int l_idx = 0;
        while (pos < len && text[pos] != '\n' && text[pos] != '\r' && l_idx < (int)sizeof(line) - 1) {
            line[l_idx++] = text[pos++];
        }
        line[l_idx] = '\0';
        while (pos < len && (text[pos] == '\n' || text[pos] == '\r')) pos++;

        if (l_idx == 0) {
            /* Empty line spacing */
            TAD_SPAN *span = &tb->spans[tb->span_count++];
            memset(span, 0, sizeof(TAD_SPAN));
            span->style = cur_style;
            span->style.line_pitch = 10;
            span->text[0] = '\0';
            continue;
        }

        /* Check Fusen / Section Markers */
        TAD_SPAN *span = &tb->spans[tb->span_count++];
        memset(span, 0, sizeof(TAD_SPAN));

        if (strncmp(line, "■ ", 4) == 0 || strncmp(line, "【", 3) == 0) {
            span->style = cur_style;
            span->style.font_id = 1;     /* Gothic */
            span->style.font_size = 16;
            span->style.weight = 700;
            span->style.color = COLOR_NAVY;
            span->style.line_pitch = 28;
            span->style.indent = 10;
            strncpy(span->text, line, sizeof(span->text) - 1);
        } else if (strncmp(line, "▶ ", 4) == 0 || strncmp(line, "第", 3) == 0) {
            span->style = cur_style;
            span->style.font_id = 1;
            span->style.font_size = 14;
            span->style.weight = 700;
            span->style.color = COLOR_BLUE;
            span->style.line_pitch = 24;
            span->style.indent = 15;
            strncpy(span->text, line, sizeof(span->text) - 1);
        } else if (strncmp(line, "[仮身]", 8) == 0) {
            /* Virtual Object Hyper-Link */
            span->style = cur_style;
            span->style.font_id = 1;
            span->style.font_size = 12;
            span->style.weight = 700;
            span->style.color = COLOR_BLUE;
            span->style.line_pitch = 22;
            span->style.indent = 25;
            span->style.is_vobj = TRUE;
            span->is_link = TRUE;

            /* Parse ID and label */
            span->style.target_robj = 100;
            const char *id_ptr = strstr(line, "#");
            if (id_ptr) span->style.target_robj = (ID)atoi(id_ptr + 1);
            strncpy(span->style.vobj_label, line, sizeof(span->style.vobj_label) - 1);
            strncpy(span->text, line, sizeof(span->text) - 1);
        } else if (strncmp(line, "───", 6) == 0 || strncmp(line, "━━━", 6) == 0 || strncmp(line, "===", 3) == 0) {
            span->style = cur_style;
            span->style.is_hr = TRUE;
            span->style.line_pitch = 12;
            span->text[0] = '\0';
        } else if (line[0] == '|' || strncmp(line, "```", 3) == 0 || strncmp(line, "  ", 2) == 0) {
            span->style = cur_style;
            span->style.font_id = 2;     /* Monospace */
            span->style.font_size = 10;
            span->style.color = COLOR_DKGRAY;
            span->style.line_pitch = 18;
            span->style.indent = 20;
            strncpy(span->text, line, sizeof(span->text) - 1);
        } else {
            span->style = cur_style;
            strncpy(span->text, line, sizeof(span->text) - 1);
        }
    }
}

ER tad_browser_load_buffer(TAD_BROWSER *tb, const void *buf, UW len, const char *doc_title) {
    if (!tb || !buf || len == 0) return E_PAR;
    tad_browser_init(tb);
    if (doc_title) strncpy(tb->doc_title, doc_title, sizeof(tb->doc_title) - 1);
    tb->total_bytes = len;

    const unsigned char *p = (const unsigned char*)buf;

    /* Check if binary TAD (starts with Record Type 1: 0x00 0x01) */
    if (len >= 6 && p[0] == 0x00 && p[1] == 0x01) {
        tb->is_binary_tad = TRUE;
        UW payload_size = (p[2] << 24) | (p[3] << 16) | (p[4] << 8) | p[5];
        UW offset = 6;
        UW max_off = (offset + payload_size < len) ? offset + payload_size : len;

        TAD_STYLE cur_style;
        memset(&cur_style, 0, sizeof(cur_style));
        cur_style.font_id = 0;
        cur_style.font_size = 12;
        cur_style.weight = 400;
        cur_style.color = COLOR_BLACK;
        cur_style.line_pitch = 20;
        cur_style.indent = 10;

        while (offset < max_off && tb->span_count < TAD_MAX_SPANS) {
            /* Check 16-bit Fusen Tag */
            if (offset + 6 <= max_off && p[offset] == 0xFF && (p[offset+1] >= 0xA0 && p[offset+1] <= 0xBF)) {
                UH seg_tag = (p[offset] << 8) | p[offset+1];
                UW seg_len = (p[offset+2] << 24) | (p[offset+3] << 16) | (p[offset+4] << 8) | p[offset+5];
                offset += 6;
                const unsigned char *seg_data = &p[offset];

                if (seg_tag == 0xFFA1 && seg_len >= 5) { /* TS_TRULER */
                    cur_style.line_pitch = (seg_data[1] << 8) | seg_data[2];
                    cur_style.indent = (seg_data[3] << 8) | seg_data[4];
                } else if (seg_tag == 0xFFA2 && seg_len >= 4) { /* TS_TFONT */
                    cur_style.font_id = (seg_data[1] << 8) | seg_data[2];
                } else if (seg_tag == 0xFFA3 && seg_len >= 9) { /* TS_TCHAR */
                    cur_style.font_size = (seg_data[1] << 8) | seg_data[2];
                    cur_style.weight = (seg_data[3] << 8) | seg_data[4];
                    cur_style.color = (COLOR)((seg_data[5] << 24) | (seg_data[6] << 16) | (seg_data[7] << 8) | seg_data[8]);
                    if (cur_style.color == 0) cur_style.color = COLOR_BLACK;
                } else if (seg_tag == 0xFFA8 && seg_len >= 8) { /* TS_VOBJ */
                    TAD_SPAN *span = &tb->spans[tb->span_count++];
                    memset(span, 0, sizeof(TAD_SPAN));
                    span->style = cur_style;
                    span->style.is_vobj = TRUE;
                    span->is_link = TRUE;
                    span->style.target_robj = (seg_data[1] << 24) | (seg_data[2] << 16) | (seg_data[3] << 8) | seg_data[4];
                    UH l_len = (seg_data[5] << 8) | seg_data[6];
                    if (l_len > 0 && l_len < 64 && offset + 7 + l_len <= max_off) {
                        memcpy(span->style.vobj_label, &seg_data[7], l_len);
                        span->style.vobj_label[l_len] = '\0';
                    }
                    snprintf(span->text, sizeof(span->text), "[仮身] #%d : %s", span->style.target_robj, span->style.vobj_label);
                } else if (seg_tag == 0xFFB0) { /* TS_FPRIM / HR */
                    TAD_SPAN *span = &tb->spans[tb->span_count++];
                    memset(span, 0, sizeof(TAD_SPAN));
                    span->style = cur_style;
                    span->style.is_hr = TRUE;
                    span->style.line_pitch = 12;
                }
                offset += seg_len;
            } else {
                /* Text segment */
                char line[256];
                int l_idx = 0;
                while (offset < max_off && p[offset] != '\n' && l_idx < (int)sizeof(line) - 1) {
                    /* Break if next token is a Fusen segment */
                    if (offset + 2 <= max_off && p[offset] == 0xFF && (p[offset+1] >= 0xA0 && p[offset+1] <= 0xBF)) break;
                    line[l_idx++] = p[offset++];
                }
                line[l_idx] = '\0';
                if (offset < max_off && p[offset] == '\n') offset++;

                if (l_idx > 0) {
                    TAD_SPAN *span = &tb->spans[tb->span_count++];
                    memset(span, 0, sizeof(TAD_SPAN));
                    span->style = cur_style;
                    strncpy(span->text, line, sizeof(span->text) - 1);
                }
            }
        }
    } else {
        /* Fallback text TAD parser */
        tb->is_binary_tad = FALSE;
        parse_text_tad_lines(tb, (const char*)buf, len);
    }

    tad_browser_layout(tb, 640);
    return E_OK;
}

ER tad_browser_load_file(TAD_BROWSER *tb, const char *filepath) {
    if (!tb || !filepath) return E_PAR;
    strncpy(tb->file_path, filepath, sizeof(tb->file_path) - 1);

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
    FILE *f = fopen(filepath, "rb");
    if (!f) return E_NOEXS;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) { fclose(f); return E_SYS; }
    char *buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return E_NOMEM; }

    size_t read_bytes = fread(buf, 1, sz, f);
    fclose(f);
    buf[read_bytes] = '\0';

    ER er = tad_browser_load_buffer(tb, buf, (UW)read_bytes, filepath);
    free(buf);
    return er;
#else
    return E_NOSPT;
#endif
}

void tad_browser_layout(TAD_BROWSER *tb, int view_width) {
    if (!tb) return;
    tb->doc_width = (view_width > 200) ? view_width : 640;

    int cur_y = 10;
    for (int i = 0; i < tb->span_count; i++) {
        TAD_SPAN *s = &tb->spans[i];
        int pitch = s->style.line_pitch;
        if (pitch <= 0) pitch = (s->style.font_size > 0) ? s->style.font_size + 6 : 20;

        int indent = s->style.indent;
        if (indent <= 0) indent = 10;

        int text_len = (int)strlen(s->text);
        int calc_w = text_len * ((s->style.font_size >= 16) ? 10 : 7);
        if (calc_w > tb->doc_width - indent - 30) calc_w = tb->doc_width - indent - 30;
        if (calc_w < 50 && s->style.is_vobj) calc_w = 200;

        s->bounds.left = indent;
        s->bounds.top = cur_y;
        s->bounds.right = indent + calc_w;
        s->bounds.bottom = cur_y + pitch;

        cur_y += pitch;
    }
    tb->doc_height = cur_y + 30;
}

void tad_browser_paint(TAD_BROWSER *tb, GDEV *dev, const RECT *client_rect) {
    if (!tb || !dev || !client_rect) return;

    /* Background Fill */
    RECT bg = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &bg, COLOR_WHITE);

    int view_h = client_rect->bottom - client_rect->top;
    tb->page_height = view_h;

    /* Render Visible Spans */
    for (int i = 0; i < tb->span_count; i++) {
        TAD_SPAN *s = &tb->spans[i];
        int vy = s->bounds.top - tb->scroll_y;

        /* Clipping check */
        if (vy + s->style.line_pitch < 0 || vy > view_h) continue;

        if (s->style.is_hr) {
            /* Vector horizontal separator */
            drw_lin(dev, s->bounds.left, vy + 6, dev->width - 30, vy + 6);
        } else if (s->style.is_vobj || s->is_link) {
            /* Virtual Object Hyper-Link Container */
            RECT link_bg = { s->bounds.left - 2, vy - 1, s->bounds.right + 4, vy + s->style.line_pitch - 2 };
            COLOR bg_col = (tb->hovered_link_idx == i) ? COLOR_LTGRAY : COLOR_WHITE;

            fill_rec(dev, &link_bg, bg_col);
            drw_rec(dev, &link_bg);

            drw_tc_string(dev, s->bounds.left + 2, vy, s->text, COLOR_BLUE, 0x00000000);
            if (tb->hovered_link_idx == i) {
                drw_lin(dev, s->bounds.left + 2, vy + s->style.line_pitch - 3, s->bounds.right + 2, vy + s->style.line_pitch - 3);
            }
        } else {
            /* Standard text line */
            if (s->text[0] != '\0') {
                COLOR text_col = (s->style.color != 0) ? s->style.color : COLOR_BLACK;
                drw_tc_string(dev, s->bounds.left, vy, s->text, text_col, 0x00000000);
            }
        }
    }

    /* Scrollbar Indicator (Right Margin) */
    if (tb->doc_height > view_h && view_h > 0) {
        int sb_x = dev->width - 12;
        RECT sb_track = { sb_x, 0, dev->width, view_h - 20 };
        fill_rec(dev, &sb_track, COLOR_LTGRAY);
        drw_rec(dev, &sb_track);

        int thumb_h = (view_h * view_h) / tb->doc_height;
        if (thumb_h < 15) thumb_h = 15;
        int thumb_y = (tb->scroll_y * (view_h - 20 - thumb_h)) / (tb->doc_height - view_h);

        RECT sb_thumb = { sb_x + 1, thumb_y, dev->width - 1, thumb_y + thumb_h };
        fill_rec(dev, &sb_thumb, COLOR_DKGRAY);
        drw_rec(dev, &sb_thumb);
    }

    /* Status Bar Footer */
    RECT status_r = { 0, dev->height - 20, dev->width, dev->height };
    fill_rec(dev, &status_r, COLOR_LTGRAY);
    drw_lin(dev, 0, dev->height - 20, dev->width, dev->height - 20);

    char status_msg[128];
    snprintf(status_msg, sizeof(status_msg), "TAD SPEC 3.20 | %d Spans | %u Bytes | Pos: %d/%d",
             tb->span_count, tb->total_bytes, tb->scroll_y, tb->doc_height);
    drw_tc_string(dev, 8, dev->height - 16, status_msg, COLOR_BLACK, 0x00000000);
}

BOOL tad_browser_handle_mouse(TAD_BROWSER *tb, int mouse_x, int mouse_y, BOOL is_click, ID *out_clicked_robj, char *out_clicked_path) {
    if (!tb) return FALSE;
    int doc_y = mouse_y + tb->scroll_y;
    tb->hovered_link_idx = -1;

    for (int i = 0; i < tb->span_count; i++) {
        TAD_SPAN *s = &tb->spans[i];
        if (!s->is_link) continue;

        if (mouse_x >= s->bounds.left && mouse_x <= s->bounds.right &&
            doc_y >= s->bounds.top && doc_y <= s->bounds.bottom) {
            tb->hovered_link_idx = i;
            if (is_click) {
                tb->active_link_idx = i;
                if (out_clicked_robj) *out_clicked_robj = s->style.target_robj;
                if (out_clicked_path) strncpy(out_clicked_path, s->style.vobj_path, 127);
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

void tad_browser_scroll(TAD_BROWSER *tb, int delta_y) {
    if (!tb) return;
    tb->scroll_y += delta_y;
    int max_scroll = tb->doc_height - tb->page_height;
    if (max_scroll < 0) max_scroll = 0;
    if (tb->scroll_y < 0) tb->scroll_y = 0;
    if (tb->scroll_y > max_scroll) tb->scroll_y = max_scroll;
}

static void handle_tad_browser_event(WND *wnd, const EVT *evt) {
    if (!wnd || !evt) return;

    H rel_x = evt->pos.x - (wnd->bounds.left + 4);
    H rel_y = evt->pos.y - (wnd->bounds.top + 26);

    if (evt->type == EV_BUT_DOWN) {
        ID clicked_robj = 0;
        char clicked_path[128] = "";
        if (tad_browser_handle_mouse(&g_active_browser, rel_x, rel_y, TRUE, &clicked_robj, clicked_path)) {
            if (clicked_path[0] != '\0') {
                /* If path ends with .html, map to corresponding .tad if available */
                char target_file[128];
                strncpy(target_file, clicked_path, 127);
                char *dot = strstr(target_file, ".html");
                if (dot) {
                    strcpy(dot, ".tad");
                }
                open_tad_browser_window(target_file, NULL);
            }
        }
        return;
    }

    if (evt->type == EV_MOUSE_MOVE) {
        tad_browser_handle_mouse(&g_active_browser, rel_x, rel_y, FALSE, NULL, NULL);
        return;
    }

    if (evt->type == EV_KEY_DOWN) {
        UW key = evt->key;
        if (key == BTRON_KEY_UP || key == 'k') {
            tad_browser_scroll(&g_active_browser, -24);
        } else if (key == BTRON_KEY_DOWN || key == 'j') {
            tad_browser_scroll(&g_active_browser, 24);
        } else if (key == BTRON_KEY_PAGE_UP) {
            tad_browser_scroll(&g_active_browser, -g_active_browser.page_height + 40);
        } else if (key == BTRON_KEY_PAGE_DOWN || key == ' ') {
            tad_browser_scroll(&g_active_browser, g_active_browser.page_height - 40);
        }
    }
}

static void paint_browser_wnd(WND *wnd, GDEV *dev) {
    if (!wnd || !dev) return;
    RECT cr = { 0, 0, dev->width, dev->height };
    tad_browser_paint(&g_active_browser, dev, &cr);
}

WND* open_tad_browser_window(const char *filepath, const char *title) {
    tad_browser_init(&g_active_browser);
    if (filepath) {
        tad_browser_load_file(&g_active_browser, filepath);
    }
    const char *w_title = title ? title : (filepath ? filepath : "BTRON TAD Browser");
    WND *wnd = opn_wnd(w_title, 60, 40, 680, 480,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_RESIZE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->paint = paint_browser_wnd;
        wnd->event_handler = handle_tad_browser_event;
    }
    return wnd;
}
