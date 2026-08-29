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
    tb->history_count = 0;
    tb->history_idx = -1;
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

            /* Parse ID, label and path */
            span->style.target_robj = 100;
            const char *id_ptr = strstr(line, "#");
            if (id_ptr) span->style.target_robj = (ID)atoi(id_ptr + 1);

            const char *path_ptr = strstr(line, "-> ");
            if (path_ptr) {
                strncpy(span->style.vobj_path, path_ptr + 3, sizeof(span->style.vobj_path) - 1);
            }
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
    tb->span_count = 0;
    tb->hovered_link_idx = -1;
    tb->active_link_idx = -1;
    if (doc_title) strncpy(tb->doc_title, doc_title, sizeof(tb->doc_title) - 1);
    tb->total_bytes = len;

    const unsigned char *p = (const unsigned char*)buf;

    /* Check if binary TAD (starts with Record Type 1: 0x00 0x01) */
    if (len >= 6 && p[0] == 0x00 && p[1] == 0x01) {
        tb->is_binary_tad = TRUE;
        UW offset = 6; /* Skip record header and length */

        TAD_STYLE cur_style;
        memset(&cur_style, 0, sizeof(cur_style));
        cur_style.font_id = 0;
        cur_style.font_size = 12;
        cur_style.weight = 400;
        cur_style.color = COLOR_BLACK;
        cur_style.line_pitch = 20;
        cur_style.indent = 10;

        while (offset + 6 <= len && tb->span_count < TAD_MAX_SPANS) {
            if (p[offset] == 0xFF) {
                UB tag_sub = p[offset + 1];
                UW seg_len = (p[offset + 2] << 24) | (p[offset + 3] << 16) | (p[offset + 4] << 8) | p[offset + 5];
                offset += 6;

                if (offset + seg_len > len) break;

                switch (tag_sub) {
                    case 0xA0: /* TS_TPAGE */
                        break;
                    case 0xA1: /* TS_TRULER */
                        if (seg_len >= 2) {
                            cur_style.line_pitch = p[offset];
                            cur_style.indent = p[offset + 1];
                        }
                        break;
                    case 0xA2: /* TS_TFONT */
                        if (seg_len >= 1) cur_style.font_id = p[offset];
                        break;
                    case 0xA3: /* TS_TCHAR */
                        if (seg_len >= 6) {
                            cur_style.font_size = p[offset];
                            cur_style.weight = (p[offset + 1] << 8) | p[offset + 2];
                            cur_style.color = (p[offset + 3] << 16) | (p[offset + 4] << 8) | p[offset + 5] | 0xFF000000;
                        }
                        break;
                    case 0xA8: /* TS_VOBJ */
                        if (seg_len >= 5) {
                            TAD_SPAN *span = &tb->spans[tb->span_count++];
                            memset(span, 0, sizeof(TAD_SPAN));
                            span->style = cur_style;
                            span->style.is_vobj = TRUE;
                            span->is_link = TRUE;

                            UW data_off = offset;
                            if (seg_len >= 7 && p[offset] == 0x00) {
                                data_off = offset + 1;
                            }

                            span->style.target_robj = (p[data_off] << 24) | (p[data_off + 1] << 16) | (p[data_off + 2] << 8) | p[data_off + 3];

                            UW label_len = (p[data_off + 4] << 8) | p[data_off + 5];
                            UW next_off = data_off + 6;
                            if (label_len == 0 && data_off + 5 < len && p[data_off + 5] > 0) {
                                label_len = p[data_off + 5];
                                next_off = data_off + 6;
                            }

                            if (label_len > 0 && next_off + label_len <= len) {
                                int copy_len = (label_len < 63) ? (int)label_len : 63;
                                memcpy(span->style.vobj_label, &p[next_off], copy_len);
                                span->style.vobj_label[copy_len] = '\0';
                                next_off += label_len;
                            }

                            if (next_off + 2 <= len) {
                                UW path_len = (p[next_off] << 8) | p[next_off + 1];
                                if (path_len > 0 && next_off + 2 + path_len <= len) {
                                    int copy_len = (path_len < 127) ? (int)path_len : 127;
                                    memcpy(span->style.vobj_path, &p[next_off + 2], copy_len);
                                    span->style.vobj_path[copy_len] = '\0';
                                }
                            }

                            span->style.line_pitch = 22;
                            span->style.indent = 20;
                            snprintf(span->text, sizeof(span->text), "[仮身] %s", span->style.vobj_label[0] ? span->style.vobj_label : "Link");
                        }
                        break;
                    case 0xB0: /* TS_FPRIM */
                        {
                            TAD_SPAN *span = &tb->spans[tb->span_count++];
                            memset(span, 0, sizeof(TAD_SPAN));
                            span->style = cur_style;
                            span->style.is_hr = TRUE;
                            span->style.line_pitch = 12;
                            span->text[0] = '\0';
                        }
                        break;
                    default:
                        break;
                }
                offset += seg_len;
            } else {
                /* Text segment */
                UW txt_start = offset;
                while (offset < len && p[offset] != 0xFF) offset++;

                UW max_off = offset;
                offset = txt_start;
                char line[256];
                int l_idx = 0;
                while (offset < max_off && p[offset] != '\n' && l_idx < (int)sizeof(line) - 1) {
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

    int cur_y = 36; /* Start below the 32px navigation toolbar */
    for (int i = 0; i < tb->span_count; i++) {
        TAD_SPAN *s = &tb->spans[i];
        int pitch = s->style.line_pitch;
        if (pitch <= 0) pitch = (s->style.font_size > 0) ? s->style.font_size + 6 : 20;

        int indent = s->style.indent;
        if (indent <= 0) indent = 10;

        int text_len = (int)strlen(s->text);
        int calc_w = text_len * ((s->style.font_size >= 16) ? 10 : 7);
        if (calc_w > tb->doc_width - indent - 30) calc_w = tb->doc_width - indent - 30;
        if (calc_w < 50 && s->style.is_vobj) calc_w = 260;

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
    tb->page_height = view_h - 60; /* Account for toolbar & status bar */

    /* ── Fixed Navigation Toolbar (Top: 0..30px) ───────────────────────────── */
    RECT tb_bar = { 0, 0, dev->width, 30 };
    fill_rec(dev, &tb_bar, COLOR_LTGRAY);
    drw_lin(dev, 0, 30, dev->width, 30);

    /* [◄ Назад] Button */
    RECT btn_back = { 6, 4, 70, 26 };
    fill_rec(dev, &btn_back, tad_browser_can_go_back(tb) ? COLOR_WHITE : COLOR_LTGRAY);
    drw_rec(dev, &btn_back);
    drw_tc_string(dev, 10, 8, "[◄ Назад]", tad_browser_can_go_back(tb) ? COLOR_BLACK : COLOR_GRAY, 0x00000000);

    /* [► Вперед] Button */
    RECT btn_fwd = { 76, 4, 144, 26 };
    fill_rec(dev, &btn_fwd, tad_browser_can_go_forward(tb) ? COLOR_WHITE : COLOR_LTGRAY);
    drw_rec(dev, &btn_fwd);
    drw_tc_string(dev, 80, 8, "[► Вперед]", tad_browser_can_go_forward(tb) ? COLOR_BLACK : COLOR_GRAY, 0x00000000);

    /* [⌂ Дім] Button */
    RECT btn_home = { 150, 4, 210, 26 };
    fill_rec(dev, &btn_home, COLOR_WHITE);
    drw_rec(dev, &btn_home);
    drw_tc_string(dev, 154, 8, "[⌂ Дім]", COLOR_NAVY, 0x00000000);

    /* [↻ Оновити] Button */
    RECT btn_reload = { 216, 4, 286, 26 };
    fill_rec(dev, &btn_reload, COLOR_WHITE);
    drw_rec(dev, &btn_reload);
    drw_tc_string(dev, 220, 8, "[↻ Оновити]", COLOR_BLACK, 0x00000000);

    /* Location Bar / Current Filepath */
    RECT loc_box = { 292, 4, dev->width - 16, 26 };
    fill_rec(dev, &loc_box, COLOR_WHITE);
    drw_rec(dev, &loc_box);
    char loc_text[128];
    snprintf(loc_text, sizeof(loc_text), "📄 %s", tb->file_path[0] ? tb->file_path : "TAD Document");
    drw_tc_string(dev, 298, 8, loc_text, COLOR_DKGRAY, 0x00000000);

    /* ── Render Visible Document Spans ─────────────────────────────────────── */
    for (int i = 0; i < tb->span_count; i++) {
        TAD_SPAN *s = &tb->spans[i];
        int vy = s->bounds.top - tb->scroll_y;

        /* Clipping check (between top toolbar at 32px and bottom status at height-22px) */
        if (vy + s->style.line_pitch < 32 || vy > dev->height - 24) continue;

        if (s->style.is_hr) {
            /* Vector horizontal separator */
            drw_lin(dev, s->bounds.left, vy + 6, dev->width - 30, vy + 6);
        } else if (s->style.is_vobj || s->is_link) {
            /* Virtual Object Hyper-Link Container */
            RECT link_bg = { s->bounds.left - 2, vy - 1, s->bounds.right + 4, vy + s->style.line_pitch - 2 };
            COLOR bg_col = (tb->hovered_link_idx == i) ? COLOR_LTGRAY : COLOR_WHITE;

            fill_rec(dev, &link_bg, bg_col);
            drw_rec(dev, &link_bg);

            drw_tc_string(dev, s->bounds.left + 4, vy, s->text, COLOR_BLUE, 0x00000000);
            if (tb->hovered_link_idx == i) {
                drw_lin(dev, s->bounds.left + 4, vy + s->style.line_pitch - 3, s->bounds.right + 2, vy + s->style.line_pitch - 3);
            }
        } else {
            /* Standard text line */
            if (s->text[0] != '\0') {
                COLOR text_col = (s->style.color != 0) ? s->style.color : COLOR_BLACK;
                drw_tc_string(dev, s->bounds.left, vy, s->text, text_col, 0x00000000);
            }
        }
    }

    /* ── Scrollbar Indicator (Right Margin) ────────────────────────────────── */
    if (tb->doc_height > dev->height - 54 && dev->height > 60) {
        int sb_x = dev->width - 12;
        int track_top = 32;
        int track_h = dev->height - 54;
        RECT sb_track = { sb_x, track_top, dev->width - 2, track_top + track_h };
        fill_rec(dev, &sb_track, COLOR_LTGRAY);
        drw_rec(dev, &sb_track);

        int max_scroll = tb->doc_height - tb->page_height;
        if (max_scroll < 1) max_scroll = 1;
        int thumb_h = (tb->page_height * track_h) / tb->doc_height;
        if (thumb_h < 15) thumb_h = 15;
        int thumb_y = track_top + (tb->scroll_y * (track_h - thumb_h)) / max_scroll;

        RECT sb_thumb = { sb_x + 1, thumb_y, dev->width - 3, thumb_y + thumb_h };
        fill_rec(dev, &sb_thumb, COLOR_DKGRAY);
        drw_rec(dev, &sb_thumb);
    }

    /* ── Status Bar Footer (Bottom: height-22 .. height) ───────────────────── */
    RECT status_r = { 0, dev->height - 22, dev->width, dev->height };
    fill_rec(dev, &status_r, COLOR_LTGRAY);
    drw_lin(dev, 0, dev->height - 22, dev->width, dev->height - 22);

    if (tb->hovered_link_idx >= 0 && tb->hovered_link_idx < tb->span_count) {
        TAD_SPAN *s = &tb->spans[tb->hovered_link_idx];
        char hover_msg[256];
        snprintf(hover_msg, sizeof(hover_msg), "🔗 [仮身] Перехід до: %s (Real Object #%d) [Клацніть для переходу]",
                 s->style.vobj_path[0] ? s->style.vobj_path : s->style.vobj_label,
                 s->style.target_robj);
        drw_tc_string(dev, 8, dev->height - 17, hover_msg, COLOR_NAVY, 0x00000000);
    } else {
        char status_msg[256];
        snprintf(status_msg, sizeof(status_msg), "TAD SPEC 3.20 | %d Spans | %u B | Навігація: [◄ Back(b)] [► Fwd(f)] [⌂ Home(h)] [↻ Reload(r)]",
                 tb->span_count, tb->total_bytes);
        drw_tc_string(dev, 8, dev->height - 17, status_msg, COLOR_BLACK, 0x00000000);
    }
}

/* ── Robust Path Resolution ────────────────────────────────────────────────── */
void tad_browser_resolve_path(const char *current_path, const char *target, char *out_resolved, size_t max_len) {
    if (!target || !out_resolved || max_len == 0) return;

    char norm_target[128];
    strncpy(norm_target, target, sizeof(norm_target) - 1);
    norm_target[sizeof(norm_target) - 1] = '\0';

    /* Convert .html to .tad */
    char *dot_html = strstr(norm_target, ".html");
    if (dot_html) strcpy(dot_html, ".tad");

    /* 1. Direct path check */
    FILE *f = fopen(norm_target, "rb");
    if (f) {
        fclose(f);
        strncpy(out_resolved, norm_target, max_len - 1);
        return;
    }

    /* 2. Relative to current file directory */
    if (current_path && current_path[0]) {
        char dir[128] = "";
        strncpy(dir, current_path, sizeof(dir) - 1);
        char *last_slash = strrchr(dir, '/');
        if (last_slash) {
            *(last_slash + 1) = '\0';
        } else {
            dir[0] = '\0';
        }

        char combined[256];
        if (strncmp(norm_target, "../", 3) == 0) {
            /* Handle parent jump: strip one folder from dir */
            char parent_dir[128] = "";
            strncpy(parent_dir, dir, sizeof(parent_dir) - 1);
            char *s1 = strrchr(parent_dir, '/');
            if (s1 && s1 > parent_dir) {
                *s1 = '\0';
                char *s2 = strrchr(parent_dir, '/');
                if (s2) *(s2 + 1) = '\0';
                else parent_dir[0] = '\0';
            }
            snprintf(combined, sizeof(combined), "%s%s", parent_dir, norm_target + 3);
        } else {
            snprintf(combined, sizeof(combined), "%s%s", dir, norm_target);
        }

        f = fopen(combined, "rb");
        if (f) {
            fclose(f);
            strncpy(out_resolved, combined, max_len - 1);
            return;
        }
    }

    /* 3. Prefix fallbacks: tad_bin/ or dharma/ */
    const char *prefixes[] = { "tad_bin/", "tad_bin/shared_data/", "tad_bin/os_spec/", "dharma/", NULL };
    for (int p = 0; prefixes[p] != NULL; p++) {
        char try_path[256];
        snprintf(try_path, sizeof(try_path), "%s%s", prefixes[p], norm_target);
        f = fopen(try_path, "rb");
        if (f) {
            fclose(f);
            strncpy(out_resolved, try_path, max_len - 1);
            return;
        }
    }

    /* Fallback default */
    strncpy(out_resolved, norm_target, max_len - 1);
}

/* ── History Navigation Implementation ─────────────────────────────────────── */
ER tad_browser_navigate(TAD_BROWSER *tb, const char *target_path) {
    if (!tb || !target_path) return E_PAR;

    char resolved[TAD_MAX_PATH];
    tad_browser_resolve_path(tb->file_path, target_path, resolved, sizeof(resolved));

    ER er = tad_browser_load_file(tb, resolved);
    if (er == E_OK) {
        /* Push to navigation history stack */
        if (tb->history_idx < TAD_MAX_HISTORY - 1) {
            tb->history_idx++;
            strncpy(tb->history[tb->history_idx].path, resolved, sizeof(tb->history[0].path) - 1);
            tb->history[tb->history_idx].scroll_y = 0;
            tb->history_count = tb->history_idx + 1;
        }
        tb->scroll_y = 0;
    }
    return er;
}

BOOL tad_browser_can_go_back(const TAD_BROWSER *tb) {
    return (tb && tb->history_idx > 0);
}

BOOL tad_browser_can_go_forward(const TAD_BROWSER *tb) {
    return (tb && tb->history_idx >= 0 && tb->history_idx < tb->history_count - 1);
}

void tad_browser_go_back(TAD_BROWSER *tb) {
    if (!tad_browser_can_go_back(tb)) return;
    tb->history_idx--;
    tad_browser_load_file(tb, tb->history[tb->history_idx].path);
    tb->scroll_y = tb->history[tb->history_idx].scroll_y;
}

void tad_browser_go_forward(TAD_BROWSER *tb) {
    if (!tad_browser_can_go_forward(tb)) return;
    tb->history_idx++;
    tad_browser_load_file(tb, tb->history[tb->history_idx].path);
    tb->scroll_y = tb->history[tb->history_idx].scroll_y;
}

void tad_browser_go_home(TAD_BROWSER *tb) {
    tad_browser_navigate(tb, "dharma/01_btron3_spec.tad");
}

void tad_browser_reload(TAD_BROWSER *tb) {
    if (!tb || !tb->file_path[0]) return;
    int saved_scroll = tb->scroll_y;
    tad_browser_load_file(tb, tb->file_path);
    tb->scroll_y = saved_scroll;
}

BOOL tad_browser_handle_mouse(TAD_BROWSER *tb, int mouse_x, int mouse_y, BOOL is_click, ID *out_clicked_robj, char *out_clicked_path) {
    if (!tb) return FALSE;

    /* Toolbar clicks (Top 0..30px) */
    if (mouse_y >= 0 && mouse_y <= 30) {
        if (is_click) {
            if (mouse_x >= 6 && mouse_x <= 70) {
                /* [◄ Back] */
                tad_browser_go_back(tb);
                return TRUE;
            } else if (mouse_x >= 76 && mouse_x <= 144) {
                /* [► Forward] */
                tad_browser_go_forward(tb);
                return TRUE;
            } else if (mouse_x >= 150 && mouse_x <= 210) {
                /* [⌂ Home] */
                tad_browser_go_home(tb);
                return TRUE;
            } else if (mouse_x >= 216 && mouse_x <= 286) {
                /* [↻ Reload] */
                tad_browser_reload(tb);
                return TRUE;
            }
        }
        return FALSE;
    }

    /* Document span clicks / hovers */
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
                if (out_clicked_path) strncpy(out_clicked_path, s->style.vobj_path[0] ? s->style.vobj_path : s->style.vobj_label, 127);
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
                tad_browser_navigate(&g_active_browser, clicked_path);
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
        if (key == BTRON_KEY_BACKSPACE || key == 'b' || key == 'B') {
            tad_browser_go_back(&g_active_browser);
        } else if (key == 'f' || key == 'F') {
            tad_browser_go_forward(&g_active_browser);
        } else if (key == 'h' || key == 'H') {
            tad_browser_go_home(&g_active_browser);
        } else if (key == 'r' || key == 'R') {
            tad_browser_reload(&g_active_browser);
        } else if (key == BTRON_KEY_UP || key == 'k') {
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
        tad_browser_navigate(&g_active_browser, filepath);
    }
    const char *w_title = title ? title : (filepath ? filepath : "BTRON TAD Browser (TAD文書ブラウザ)");
    WND *wnd = opn_wnd(w_title, 60, 40, 720, 500,
                       WND_ATTR_TITLE | WND_ATTR_CLOSE | WND_ATTR_RESIZE | WND_ATTR_BORDER);
    if (wnd) {
        wnd->paint = paint_browser_wnd;
        wnd->event_handler = handle_tad_browser_event;
    }
    return wnd;
}
