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
        } else if (strncmp(line, "[付箋: FIGURE", 13) == 0 || strncmp(line, "[画像:", 7) == 0 || strncmp(line, "[図形:", 7) == 0) {
            span->style = cur_style;
            span->style.is_image = TRUE;
            span->style.img_w = 480;
            span->style.img_h = 130;
            span->style.line_pitch = 146;
            span->style.indent = 20;

            const char *cap_ptr = strstr(line, "caption=\"");
            if (cap_ptr) {
                const char *end_cap = strchr(cap_ptr + 9, '"');
                int len = end_cap ? (int)(end_cap - (cap_ptr + 9)) : 60;
                if (len > 120) len = 120;
                memcpy(span->style.img_caption, cap_ptr + 9, len);
                span->style.img_caption[len] = '\0';
            } else {
                strncpy(span->style.img_caption, line, sizeof(span->style.img_caption) - 1);
            }
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
                            UB subid = p[offset];
                            TAD_SPAN *span = &tb->spans[tb->span_count++];
                            memset(span, 0, sizeof(TAD_SPAN));
                            span->style = cur_style;

                            if (subid == 10) {
                                /* BTRON3 Figure / Picture Segment (TS_FPRIM SubID 10) */
                                span->style.is_image = TRUE;
                                if (seg_len >= 5) {
                                    span->style.img_w = (p[offset + 1] << 8) | p[offset + 2];
                                    span->style.img_h = (p[offset + 3] << 8) | p[offset + 4];
                                    span->style.img_type = (seg_len >= 6) ? p[offset + 5] : 0;
                                }
                                if (span->style.img_w <= 0) span->style.img_w = 480;
                                if (span->style.img_h <= 0) span->style.img_h = 130;
                                span->style.line_pitch = span->style.img_h + 26;
                                span->style.indent = 20;

                                if (seg_len >= 8) {
                                    UH cap_len = (p[offset + 6] << 8) | p[offset + 7];
                                    if (cap_len > 0 && offset + 8 + cap_len <= offset + seg_len) {
                                        int copy_c = (cap_len < 127) ? cap_len : 127;
                                        memcpy(span->style.img_caption, &p[offset + 8], copy_c);
                                        span->style.img_caption[copy_c] = '\0';
                                    }
                                    UW next_off = offset + 8 + cap_len;
                                    if (next_off + 2 <= offset + seg_len) {
                                        UH src_len = (p[next_off] << 8) | p[next_off + 1];
                                        if (src_len > 0 && next_off + 2 + src_len <= offset + seg_len) {
                                            int copy_s = (src_len < 127) ? src_len : 127;
                                            memcpy(span->style.img_src, &p[next_off + 2], copy_s);
                                            span->style.img_src[copy_s] = '\0';
                                        }
                                    }
                                }
                                snprintf(span->text, sizeof(span->text), "[図形] %s", span->style.img_caption[0] ? span->style.img_caption : "Figure");
                            } else {
                                /* SubID 1: Vector separator line */
                                span->style.is_hr = TRUE;
                                span->style.line_pitch = 12;
                                span->text[0] = '\0';
                            }
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

        if (s->style.is_image) {
            int fig_w = s->style.img_w ? s->style.img_w : 576;
            int fig_h = s->style.img_h ? s->style.img_h : 180;
            if (fig_w < 200) fig_w = 200;
            if (fig_h < 40) fig_h = 40;

            int box_w = fig_w + 16;
            if (box_w < tb->doc_width - 40) box_w = tb->doc_width - 40;

            s->bounds.left = 16;
            s->bounds.top = cur_y;
            s->bounds.right = s->bounds.left + box_w;
            s->bounds.bottom = cur_y + fig_h + 46;
            cur_y += fig_h + 52;
            continue;
        }

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

static void render_figure_diagram(GDEV *dev, const TAD_SPAN *s, const RECT *cvs) {
    if (!dev || !s || !cvs) return;

    UB type = s->style.img_type;
    /* Auto-detect type from caption/src if type == 0 */
    if (type == 0) {
        const char *cap = s->style.img_caption;
        const char *src = s->style.img_src;
        if (strstr(cap, "タスク") || strstr(cap, "Task") || strstr(cap, "FSM") || strstr(src, "task") || strstr(cap, "стан") || strstr(cap, "proc")) type = 1;
        else if (strstr(cap, "ウィンドウ") || strstr(cap, "Window") || strstr(cap, "HMI") || strstr(src, "window") || strstr(cap, "вікн") || strstr(src, "hmi")) type = 2;
        else if (strstr(cap, "実身") || strstr(cap, "仮身") || strstr(cap, "Real") || strstr(cap, "Virtual") || strstr(src, "store") || strstr(cap, "об'єкт") || strstr(src, "fd")) type = 3;
        else if (strstr(cap, "TRONコード") || strstr(cap, "Code") || strstr(cap, "文字") || strstr(src, "tron_code") || strstr(cap, "шрифт")) type = 4;
        else if (strstr(cap, "図形") || strstr(cap, "Display") || strstr(cap, "DP") || strstr(src, "dp") || strstr(cap, "фігур") || strstr(src, "figure")) type = 5;
        else if (strstr(cap, "メモリ") || strstr(cap, "Memory") || strstr(cap, "JPL") || strstr(src, "indexfig") || strstr(cap, "пам'ят")) type = 6;
        else if (strstr(cap, "オーディオ") || strstr(cap, "Audio") || strstr(cap, "Meter") || strstr(src, "audio")) type = 7;
    }

    int mid_y = (cvs->top + cvs->bottom) / 2;
    int cvs_w = cvs->right - cvs->left;

    switch (type) {
        case 1: /* ── Type 1: Task State Transition FSM (μITRON / T-Kernel) ── */
            {
                int box_w = (cvs_w - 60) / 4;
                if (box_w > 90) box_w = 90;
                int b_h = 28;

                RECT b_dorm = { cvs->left + 8, mid_y - b_h/2, cvs->left + 8 + box_w, mid_y + b_h/2 };
                RECT b_rdy  = { b_dorm.right + 12, cvs->top + 6, b_dorm.right + 12 + box_w, cvs->top + 6 + b_h };
                RECT b_run  = { b_rdy.right + 12, mid_y - b_h/2, b_rdy.right + 12 + box_w, mid_y + b_h/2 };
                RECT b_wait = { b_dorm.right + 12, cvs->bottom - 6 - b_h, b_dorm.right + 12 + box_w, cvs->bottom - 6 };

                fill_rec(dev, &b_dorm, COLOR_LTGRAY); drw_rec(dev, &b_dorm);
                drw_tc_string(dev, b_dorm.left + 6, b_dorm.top + 6, "DORMANT", COLOR_DKGRAY, 0);

                fill_rec(dev, &b_rdy, COLOR_LTGRAY); drw_rec(dev, &b_rdy);
                drw_tc_string(dev, b_rdy.left + 12, b_rdy.top + 6, "READY", COLOR_BLUE, 0);

                fill_rec(dev, &b_run, COLOR_NAVY); drw_rec(dev, &b_run);
                drw_tc_string(dev, b_run.left + 10, b_run.top + 6, "RUNNING", COLOR_WHITE, 0);

                fill_rec(dev, &b_wait, COLOR_LTGRAY); drw_rec(dev, &b_wait);
                drw_tc_string(dev, b_wait.left + 8, b_wait.top + 6, "WAITING", COLOR_BLACK, 0);

                drw_lin(dev, b_dorm.right, b_dorm.top + 8, b_rdy.left, b_rdy.bottom - 4);
                drw_lin(dev, b_rdy.right, b_rdy.bottom - 4, b_run.left, b_run.top + 8);
                drw_lin(dev, b_run.left, b_run.bottom - 4, b_wait.right, b_wait.top + 8);
                drw_lin(dev, b_wait.left, b_wait.top + 8, b_rdy.left + 10, b_rdy.bottom);
            }
            break;

        case 2: /* ── Type 2: TRON HMI Window Geometry & Corner Resize Blueprint ── */
            {
                RECT w_box = { cvs->left + 10, cvs->top + 6, cvs->left + 220, cvs->bottom - 6 };
                fill_rec(dev, &w_box, COLOR_LTGRAY);
                drw_rec(dev, &w_box);

                RECT w_tb = { w_box.left + 2, w_box.top + 2, w_box.right - 2, w_box.top + 16 };
                fill_rec(dev, &w_tb, COLOR_NAVY);
                drw_tc_string(dev, w_tb.left + 4, w_tb.top + 1, "[■ Window Title]", COLOR_WHITE, 0);
                drw_tc_string(dev, w_tb.right - 12, w_tb.top + 1, "x", COLOR_WHITE, 0);

                RECT w_cl = { w_box.left + 4, w_tb.bottom + 2, w_box.right - 14, w_box.bottom - 4 };
                fill_rec(dev, &w_cl, COLOR_WHITE);
                drw_rec(dev, &w_cl);
                drw_tc_string(dev, w_cl.left + 4, w_cl.top + 6, "Client Viewport", COLOR_DKGRAY, 0);

                RECT w_sb = { w_box.right - 12, w_tb.bottom + 2, w_box.right - 2, w_box.bottom - 14 };
                fill_rec(dev, &w_sb, COLOR_LTGRAY); drw_rec(dev, &w_sb);
                RECT w_th = { w_sb.left + 1, w_sb.top + 8, w_sb.right - 1, w_sb.top + 20 };
                fill_rec(dev, &w_th, COLOR_DKGRAY);

                RECT w_rz = { w_box.right - 14, w_box.bottom - 14, w_box.right - 2, w_box.bottom - 2 };
                fill_rec(dev, &w_rz, COLOR_LTGRAY);
                drw_lin(dev, w_box.right - 12, w_box.bottom - 3, w_box.right - 3, w_box.bottom - 12);
                drw_lin(dev, w_box.right - 8,  w_box.bottom - 3, w_box.right - 3, w_box.bottom - 8);
                drw_lin(dev, w_box.right - 4,  w_box.bottom - 3, w_box.right - 3, w_box.bottom - 4);

                int anno_x = w_box.right + 15;
                drw_tc_string(dev, anno_x, cvs->top + 8, "▶ Titlebar + Close Button", COLOR_NAVY, 0);
                drw_tc_string(dev, anno_x, cvs->top + 26, "▶ Client Scroll Viewport", COLOR_BLACK, 0);
                drw_tc_string(dev, anno_x, cvs->top + 44, "▶ Right Scrollbar & Thumb", COLOR_DKGRAY, 0);
                drw_tc_string(dev, anno_x, cvs->top + 62, "▶ [▞] 16x16 Corner Resize Grip", COLOR_BLUE, 0);
            }
            break;

        case 3: /* ── Type 3: Real Object & Virtual Object Hyper-Tree ── */
            {
                RECT r_root = { cvs->left + 10, mid_y - 18, cvs->left + 130, mid_y + 18 };
                fill_rec(dev, &r_root, COLOR_NAVY);
                drw_rec(dev, &r_root);
                drw_tc_string(dev, r_root.left + 6, r_root.top + 3, "[実身 Real Object]", COLOR_WHITE, 0);
                drw_tc_string(dev, r_root.left + 6, r_root.top + 18, "ID #101: Cabinet", COLOR_WHITE, 0);

                int k_x = r_root.right + 40;
                int k_w = cvs->right - k_x - 10;
                if (k_w > 200) k_w = 200;

                RECT k1 = { k_x, cvs->top + 6, k_x + k_w, cvs->top + 24 };
                RECT k2 = { k_x, mid_y - 9, k_x + k_w, mid_y + 9 };
                RECT k3 = { k_x, cvs->bottom - 24, k_x + k_w, cvs->bottom - 6 };

                fill_rec(dev, &k1, COLOR_LTGRAY); drw_rec(dev, &k1);
                drw_tc_string(dev, k1.left + 6, k1.top + 3, "[仮身] 01_btron3_spec.tad", COLOR_BLUE, 0);

                fill_rec(dev, &k2, COLOR_LTGRAY); drw_rec(dev, &k2);
                drw_tc_string(dev, k2.left + 6, k2.top + 3, "[仮身] 05_kernel_core.tad", COLOR_BLUE, 0);

                fill_rec(dev, &k3, COLOR_LTGRAY); drw_rec(dev, &k3);
                drw_tc_string(dev, k3.left + 6, k3.top + 3, "[仮身] 07_gui_shell.tad", COLOR_BLUE, 0);

                drw_lin(dev, r_root.right, mid_y, k_x - 12, mid_y);
                drw_lin(dev, k_x - 12, k1.top + 9, k_x - 12, k3.top + 9);
                drw_lin(dev, k_x - 12, k1.top + 9, k1.left, k1.top + 9);
                drw_lin(dev, k_x - 12, k2.top + 9, k2.left, k2.top + 9);
                drw_lin(dev, k_x - 12, k3.top + 9, k3.left, k3.top + 9);
            }
            break;

        case 4: /* ── Type 4: TRON Multilingual Code Map ── */
            {
                int col_w = (cvs_w - 40) / 3;
                RECT c1 = { cvs->left + 10, cvs->top + 6, cvs->left + 10 + col_w, cvs->bottom - 6 };
                RECT c2 = { c1.right + 10, cvs->top + 6, c1.right + 10 + col_w, cvs->bottom - 6 };
                RECT c3 = { c2.right + 10, cvs->top + 6, cvs->right - 10, cvs->bottom - 6 };

                fill_rec(dev, &c1, COLOR_LTGRAY); drw_rec(dev, &c1);
                drw_tc_string(dev, c1.left + 6, c1.top + 4, "Plane 1: ASCII", COLOR_NAVY, 0);
                drw_tc_string(dev, c1.left + 6, c1.top + 22, "0x00..0x7F (8x16)", COLOR_BLACK, 0);
                drw_tc_string(dev, c1.left + 6, c1.top + 42, "A B C D E 1 2 3", COLOR_DKGRAY, 0);

                fill_rec(dev, &c2, COLOR_LTGRAY); drw_rec(dev, &c2);
                drw_tc_string(dev, c2.left + 6, c2.top + 4, "Plane 1: Cyrillic", COLOR_BLUE, 0);
                drw_tc_string(dev, c2.left + 6, c2.top + 22, "0x2700..0x27FF", COLOR_BLACK, 0);
                drw_tc_string(dev, c2.left + 6, c2.top + 42, "А Б В Є І Ї Ґ я", COLOR_NAVY, 0);

                fill_rec(dev, &c3, COLOR_LTGRAY); drw_rec(dev, &c3);
                drw_tc_string(dev, c3.left + 6, c3.top + 4, "Plane 1: JIS CJK", COLOR_BLACK, 0);
                drw_tc_string(dev, c3.left + 6, c3.top + 22, "0x2100+ (16x16)", COLOR_BLACK, 0);
                drw_tc_string(dev, c3.left + 6, c3.top + 42, "超漢字 漢 和 日本", COLOR_DKGRAY, 0);
            }
            break;

        case 5: /* ── Type 5: 2D Vector Geometry Primitives (DP) ── */
            {
                RECT g_r = { cvs->left + 15, cvs->top + 10, cvs->left + 95, cvs->bottom - 10 };
                fill_rec(dev, &g_r, COLOR_LTGRAY);
                drw_rec(dev, &g_r);
                drw_tc_string(dev, g_r.left + 8, g_r.top + 16, "Rectangle", COLOR_BLACK, 0);

                int cx = cvs->left + 160;
                int cy = mid_y;
                drw_lin(dev, cx - 35, cy, cx + 35, cy);
                drw_lin(dev, cx, cy - 30, cx, cy + 30);
                drw_tc_string(dev, cx + 5, cy - 20, "(x, y)", COLOR_NAVY, 0);

                int tx = cvs->left + 250;
                drw_lin(dev, tx, cvs->bottom - 10, tx + 35, cvs->top + 10);
                drw_lin(dev, tx + 35, cvs->top + 10, tx + 70, cvs->bottom - 10);
                drw_lin(dev, tx + 70, cvs->bottom - 10, tx, cvs->bottom - 10);
                drw_tc_string(dev, tx + 10, cvs->bottom - 26, "Polygon", COLOR_BLUE, 0);

                int el_x = cvs->right - 85;
                RECT el_r = { el_x, cvs->top + 10, cvs->right - 10, cvs->bottom - 10 };
                fill_rec(dev, &el_r, COLOR_LTGRAY);
                drw_rec(dev, &el_r);
                drw_tc_string(dev, el_r.left + 10, el_r.top + 16, "Ellipse", COLOR_DKGRAY, 0);
            }
            break;

        case 6: /* ── Type 6: NASA JPL Rule 3 Bounded Memory Pool ── */
            {
                int p_w = (cvs_w - 40) / 3;
                RECT m1 = { cvs->left + 10, cvs->top + 6, cvs->left + 10 + p_w, mid_y - 4 };
                RECT m2 = { m1.right + 10, cvs->top + 6, m1.right + 10 + p_w, mid_y - 4 };
                RECT m3 = { m2.right + 10, cvs->top + 6, cvs->right - 10, mid_y - 4 };

                fill_rec(dev, &m1, COLOR_LTGRAY); drw_rec(dev, &m1);
                drw_tc_string(dev, m1.left + 6, m1.top + 4, "T_CMPF: Fixed", COLOR_BLUE, 0);
                drw_tc_string(dev, m1.left + 6, m1.top + 18, "512 x 64B Blocks", COLOR_DKGRAY, 0);

                fill_rec(dev, &m2, COLOR_LTGRAY); drw_rec(dev, &m2);
                drw_tc_string(dev, m2.left + 6, m2.top + 4, "T_CMPV: Var Pool", COLOR_NAVY, 0);
                drw_tc_string(dev, m2.left + 6, m2.top + 18, "Static 256KB Arena", COLOR_DKGRAY, 0);

                fill_rec(dev, &m3, COLOR_LTGRAY); drw_rec(dev, &m3);
                drw_tc_string(dev, m3.left + 6, m3.top + 4, "Event Buffer", COLOR_BLACK, 0);
                drw_tc_string(dev, m3.left + 6, m3.top + 18, "256 EVT Slots", COLOR_DKGRAY, 0);

                RECT bar = { cvs->left + 10, mid_y + 4, cvs->right - 10, cvs->bottom - 6 };
                fill_rec(dev, &bar, COLOR_NAVY);
                drw_rec(dev, &bar);
                drw_tc_string(dev, bar.left + 10, bar.top + 4, "NASA JPL Safety Rule 3: Zero Post-Boot Dynamic malloc() Allowed", COLOR_WHITE, 0);
            }
            break;

        case 7: /* ── Type 7: SONY TC-K777ES Stereo Audio Deck & Controls ── */
            {
                RECT deck = { cvs->left + 10, cvs->top + 6, cvs->left + 170, cvs->bottom - 6 };
                fill_rec(dev, &deck, COLOR_BLACK);
                drw_rec(dev, &deck);
                drw_tc_string(dev, deck.left + 12, mid_y - 8, "(◎)", COLOR_WHITE, 0);
                drw_lin(dev, deck.left + 40, mid_y, deck.right - 40, mid_y);
                drw_tc_string(dev, deck.right - 40, mid_y - 8, "(◎)", COLOR_WHITE, 0);
                drw_tc_string(dev, deck.left + 24, deck.bottom - 16, "SONY TC-K777ES", COLOR_LTGRAY, 0);

                int m_x = deck.right + 15;
                int m_w = cvs->right - m_x - 10;
                RECT vu_l = { m_x, cvs->top + 10, m_x + m_w, cvs->top + 24 };
                RECT vu_r = { m_x, cvs->top + 30, m_x + m_w, cvs->top + 44 };

                fill_rec(dev, &vu_l, COLOR_LTGRAY); drw_rec(dev, &vu_l);
                RECT vu_l_bar = { vu_l.left + 2, vu_l.top + 2, vu_l.left + (m_w * 7)/10, vu_l.bottom - 2 };
                fill_rec(dev, &vu_l_bar, COLOR_NAVY);
                drw_tc_string(dev, vu_l.right - 45, vu_l.top + 1, "L: -2dB", COLOR_BLACK, 0);

                fill_rec(dev, &vu_r, COLOR_LTGRAY); drw_rec(dev, &vu_r);
                RECT vu_r_bar = { vu_r.left + 2, vu_r.top + 2, vu_r.left + (m_w * 6)/10, vu_r.bottom - 2 };
                fill_rec(dev, &vu_r_bar, COLOR_BLUE);
                drw_tc_string(dev, vu_r.right - 45, vu_r.top + 1, "R: -4dB", COLOR_BLACK, 0);

                drw_tc_string(dev, m_x, cvs->bottom - 18, "TRON HMI Standard Audio Instrument Deck", COLOR_DKGRAY, 0);
            }
            break;

        case 0:
        default: /* ── Type 0: Multi-Layer Architecture Stack ── */
            {
                int b_w = (cvs_w - 50) / 3;
                if (b_w > 140) b_w = 140;

                RECT b1 = { cvs->left + 10, cvs->top + 10, cvs->left + 10 + b_w, mid_y - 4 };
                fill_rec(dev, &b1, COLOR_LTGRAY);
                drw_rec(dev, &b1);
                drw_tc_string(dev, b1.left + 6, b1.top + 8, "TAD / Shell", COLOR_NAVY, 0);

                int arr1_x = b1.right + 2;
                drw_lin(dev, arr1_x, mid_y - 12, arr1_x + 14, mid_y - 12);
                drw_lin(dev, arr1_x + 14, mid_y - 12, arr1_x + 10, mid_y - 16);
                drw_lin(dev, arr1_x + 14, mid_y - 12, arr1_x + 10, mid_y - 8);

                RECT b2 = { arr1_x + 16, cvs->top + 10, arr1_x + 16 + b_w, mid_y - 4 };
                fill_rec(dev, &b2, COLOR_LTGRAY);
                drw_rec(dev, &b2);
                drw_tc_string(dev, b2.left + 6, b2.top + 8, "DP Graphics", COLOR_BLUE, 0);

                int arr2_x = b2.right + 2;
                drw_lin(dev, arr2_x, mid_y - 12, arr2_x + 14, mid_y - 12);
                drw_lin(dev, arr2_x + 14, mid_y - 12, arr2_x + 10, mid_y - 16);
                drw_lin(dev, arr2_x + 14, mid_y - 12, arr2_x + 10, mid_y - 8);

                RECT b3 = { arr2_x + 16, cvs->top + 10, cvs->right - 10, mid_y - 4 };
                fill_rec(dev, &b3, COLOR_LTGRAY);
                drw_rec(dev, &b3);
                drw_tc_string(dev, b3.left + 6, b3.top + 8, "μITRON Kernel", COLOR_BLACK, 0);

                RECT base = { cvs->left + 10, mid_y + 4, cvs->right - 10, cvs->bottom - 8 };
                fill_rec(dev, &base, COLOR_LTGRAY);
                drw_rec(dev, &base);
                drw_tc_string(dev, base.left + 10, base.top + 6, "NASA JPL Rule 3 Bounded Memory Engine (Zero Post-Boot malloc)", COLOR_DKGRAY, 0);
            }
            break;
    }
}

#define MAX_GIF_PIXELS (2048 * 3000)
#define MAX_LZW_DICT 4096

static UH s_gif_prefix[MAX_LZW_DICT];
static UB s_gif_suffix[MAX_LZW_DICT];
static UB s_gif_stack[MAX_LZW_DICT + 1];
static UB s_gif_raw[MAX_GIF_PIXELS];

/* ── Native BTRON3 GIF Specification Diagram Decoder (NASA JPL Rule 3 Bounded) ── */
static int decode_and_draw_gif(GDEV *dev, const char *filepath, int dst_x, int dst_y, int max_w, int max_h) {
    if (!dev || !filepath) return -1;
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return -1;

    UB hdr[13];
    if (fread(hdr, 1, 13, fp) != 13) {
        fclose(fp);
        return -1;
    }

    if (memcmp(hdr, "GIF87a", 6) != 0 && memcmp(hdr, "GIF89a", 6) != 0) {
        fclose(fp);
        return -1;
    }

    UB flags = hdr[10];
    int has_gct = (flags & 0x80) != 0;
    int gct_size = 1 << ((flags & 0x07) + 1);

    UW gct[256];
    memset(gct, 0, sizeof(gct));

    if (has_gct) {
        UB gct_raw[768];
        if (fread(gct_raw, 1, gct_size * 3, fp) != (size_t)(gct_size * 3)) {
            fclose(fp);
            return -1;
        }
        for (int i = 0; i < gct_size; i++) {
            UB r = gct_raw[i * 3 + 0];
            UB g = gct_raw[i * 3 + 1];
            UB b = gct_raw[i * 3 + 2];
            gct[i] = (r << 16) | (g << 8) | b;
        }
    }

    int trans_idx = -1;
    int img_w = 0, img_h = 0;
    BOOL img_read = FALSE;

    while (!feof(fp)) {
        int b = fgetc(fp);
        if (b == EOF || b == 0x3B) break;

        if (b == 0x21) {
            /* Extension block */
            int ext_label = fgetc(fp);
            if (ext_label == 0xF9) {
                int block_size = fgetc(fp);
                if (block_size == 4) {
                    UB gce[4];
                    if (fread(gce, 1, 4, fp) == 4 && (gce[0] & 0x01)) {
                        trans_idx = gce[3];
                    }
                }
                while (1) {
                    int sub_len = fgetc(fp);
                    if (sub_len <= 0) break;
                    fseek(fp, sub_len, SEEK_CUR);
                }
            } else {
                while (1) {
                    int sub_len = fgetc(fp);
                    if (sub_len <= 0) break;
                    fseek(fp, sub_len, SEEK_CUR);
                }
            }
        } else if (b == 0x2C) {
            /* Image Descriptor */
            UB desc[9];
            if (fread(desc, 1, 9, fp) != 9) break;

            img_w = desc[4] | (desc[5] << 8);
            img_h = desc[6] | (desc[7] << 8);
            UB iflags = desc[8];

            int has_lct = (iflags & 0x80) != 0;
            UW palette[256];
            memcpy(palette, gct, sizeof(palette));

            if (has_lct) {
                int lct_size = 1 << ((iflags & 0x07) + 1);
                UB lct_raw[768];
                if (fread(lct_raw, 1, lct_size * 3, fp) == (size_t)(lct_size * 3)) {
                    for (int i = 0; i < lct_size; i++) {
                        UB r = lct_raw[i * 3 + 0];
                        UB g = lct_raw[i * 3 + 1];
                        UB b = lct_raw[i * 3 + 2];
                        palette[i] = (r << 16) | (g << 8) | b;
                    }
                }
            } else if (!has_gct) {
                palette[0] = 0x00FFFFFF;
                palette[1] = 0x00000000;
            }

            int min_code_size = fgetc(fp);
            if (min_code_size < 2 || min_code_size > 8) min_code_size = 8;

            int clear_code = 1 << min_code_size;
            int eoi_code = clear_code + 1;
            int next_code = eoi_code + 1;
            int code_size = min_code_size + 1;
            int code_mask = (1 << code_size) - 1;

            for (int i = 0; i < clear_code; i++) {
                s_gif_prefix[i] = 0;
                s_gif_suffix[i] = (UB)i;
            }

            int bit_buf = 0;
            int bit_count = 0;
            int old_code = -1;
            int first_char = 0;
            int stack_top = 0;
            int pixel_count = 0;
            int total_pixels = img_w * img_h;
            if (total_pixels > MAX_GIF_PIXELS) total_pixels = MAX_GIF_PIXELS;

            UB sub_buf[256];
            int sub_len = 0;
            int sub_pos = 0;

            while (pixel_count < total_pixels) {
                while (bit_count < code_size) {
                    if (sub_pos >= sub_len) {
                        sub_len = fgetc(fp);
                        if (sub_len <= 0) break;
                        if (fread(sub_buf, 1, sub_len, fp) != (size_t)sub_len) break;
                        sub_pos = 0;
                    }
                    bit_buf |= (sub_buf[sub_pos++] << bit_count);
                    bit_count += 8;
                }

                if (bit_count < code_size) break;

                int code = bit_buf & code_mask;
                bit_buf >>= code_size;
                bit_count -= code_size;

                if (code == clear_code) {
                    code_size = min_code_size + 1;
                    code_mask = (1 << code_size) - 1;
                    next_code = eoi_code + 1;
                    old_code = -1;
                    continue;
                }
                if (code == eoi_code) break;

                int cur_code = code;
                if (cur_code >= next_code) {
                    s_gif_stack[stack_top++] = (UB)first_char;
                    cur_code = old_code;
                }

                while (cur_code >= clear_code && cur_code < MAX_LZW_DICT) {
                    s_gif_stack[stack_top++] = s_gif_suffix[cur_code];
                    cur_code = s_gif_prefix[cur_code];
                }
                first_char = s_gif_suffix[cur_code];
                s_gif_stack[stack_top++] = (UB)first_char;

                while (stack_top > 0 && pixel_count < total_pixels) {
                    s_gif_raw[pixel_count++] = s_gif_stack[--stack_top];
                }

                if (old_code >= 0 && next_code < MAX_LZW_DICT) {
                    s_gif_prefix[next_code] = old_code;
                    s_gif_suffix[next_code] = (UB)first_char;
                    next_code++;
                    if (next_code > code_mask && code_size < 12) {
                        code_size++;
                        code_mask = (1 << code_size) - 1;
                    }
                }
                old_code = code;
            }

            /* Draw 1:1 Pixel-Perfect (Centered if max_w > img_w) */
            int draw_x = dst_x;
            if (max_w > img_w) {
                draw_x = dst_x + (max_w - img_w) / 2;
            }
            int draw_y = dst_y;

            for (int y = 0; y < img_h; y++) {
                int out_y = draw_y + y;
                /* Clip between toolbar (32px) and statusbar (height - 22px) */
                if (out_y < 32 || out_y >= dev->height - 22) continue;
                if (max_h > 0 && y >= max_h) break;

                for (int x = 0; x < img_w; x++) {
                    int out_x = draw_x + x;
                    if (out_x < dst_x || out_x >= dst_x + max_w) continue;
                    if (out_x < 0 || out_x >= dev->width) continue;

                    UB p_idx = s_gif_raw[y * img_w + x];
                    if (p_idx == trans_idx) continue;

                    UW col = palette[p_idx];
                    dev->pixels[out_y * dev->width + out_x] = (COLOR)(0xFF000000 | col);
                }
            }

            img_read = TRUE;
            break;
        }
    }

    fclose(fp);
    return img_read ? 0 : -1;
}

static int resolve_and_draw_gif(const TAD_BROWSER *tb, GDEV *dev, const TAD_SPAN *s, const RECT *cvs) {
    if (!dev || !s || !cvs || s->style.img_src[0] == '\0') return -1;

    char candidate[256];
    int max_w = cvs->right - cvs->left;
    int max_h = cvs->bottom - cvs->top;

    /* 1. Direct path as specified */
    if (decode_and_draw_gif(dev, s->style.img_src, cvs->left + 2, cvs->top + 2, max_w - 4, max_h - 4) == 0) {
        return 0;
    }

    /* 2. Relative to tb->file_path */
    if (tb && tb->file_path[0] != '\0') {
        char doc_dir[256];
        strncpy(doc_dir, tb->file_path, sizeof(doc_dir) - 1);
        doc_dir[sizeof(doc_dir) - 1] = '\0';
        char *last_slash = strrchr(doc_dir, '/');
        if (last_slash) {
            *last_slash = '\0';
            snprintf(candidate, sizeof(candidate), "%s/%s", doc_dir, s->style.img_src);
            if (decode_and_draw_gif(dev, candidate, cvs->left + 2, cvs->top + 2, max_w - 4, max_h - 4) == 0) {
                return 0;
            }
        }
    }

    /* 3. Substituted tad_bin/ -> doc/ path */
    if (tb && tb->file_path[0] != '\0') {
        const char *p = tb->file_path;
        if (strncmp(p, "tad_bin/", 8) == 0) {
            char doc_dir[256];
            snprintf(doc_dir, sizeof(doc_dir), "doc/%s", p + 8);
            char *last_slash = strrchr(doc_dir, '/');
            if (last_slash) {
                *last_slash = '\0';
                snprintf(candidate, sizeof(candidate), "%s/%s", doc_dir, s->style.img_src);
                if (decode_and_draw_gif(dev, candidate, cvs->left + 2, cvs->top + 2, max_w - 4, max_h - 4) == 0) {
                    return 0;
                }
            }
        }
    }

    /* 4. Common specification directories */
    const char *common_dirs[] = {
        "doc/shared_data",
        "doc/os_spec/shell",
        "doc/os_spec/kernel",
        "doc/os_spec/dp",
        "tad_bin/shared_data",
        "tad_bin/os_spec/shell",
        "tad_bin/os_spec/kernel",
        "tad_bin/os_spec/dp",
        NULL
    };

    for (int i = 0; common_dirs[i]; i++) {
        snprintf(candidate, sizeof(candidate), "%s/%s", common_dirs[i], s->style.img_src);
        if (decode_and_draw_gif(dev, candidate, cvs->left + 2, cvs->top + 2, max_w - 4, max_h - 4) == 0) {
            return 0;
        }
    }

    return -1;
}

void tad_browser_paint(TAD_BROWSER *tb, GDEV *dev, const RECT *client_rect) {
    if (!tb || !dev || !client_rect) return;

    /* Background Fill */
    RECT bg = { 0, 0, dev->width, dev->height };
    fill_rec(dev, &bg, COLOR_WHITE);

    int view_h = client_rect->bottom - client_rect->top;
    tb->page_height = view_h - 60; /* Account for toolbar & status bar */

    /* ── Render Visible Document Spans (Strictly Clipped to Content Viewport) ── */
    RECT orig_clip = dev->clip;
    RECT content_clip = { 0, 32, dev->width, dev->height - 22 };
    set_clip(dev, &content_clip);

    for (int i = 0; i < tb->span_count; i++) {
        TAD_SPAN *s = &tb->spans[i];
        int vy = s->bounds.top - tb->scroll_y;
        int span_h = s->bounds.bottom - s->bounds.top;

        /* Skip items entirely outside the vertical viewport */
        if (vy + span_h < 32 || vy >= dev->height - 22) continue;

        if (s->style.is_image) {
            /* ── BTRON3 Figure / Picture Box Container (TS_FPRIM 0xFFB0 SubID 10) ── */
            RECT pic_box = { s->bounds.left, vy, s->bounds.right, vy + span_h - 4 };
            fill_rec(dev, &pic_box, COLOR_LTGRAY);
            drw_rec(dev, &pic_box);

            /* Title Header */
            RECT hdr = { pic_box.left, pic_box.top, pic_box.right, pic_box.top + 20 };
            fill_rec(dev, &hdr, COLOR_NAVY);
            char hdr_title[128];
            snprintf(hdr_title, sizeof(hdr_title), "[🖼 %s]", s->style.img_src[0] ? s->style.img_src : "BTRON3 図形・実身画像");
            drw_tc_string(dev, hdr.left + 8, hdr.top + 3, hdr_title, COLOR_WHITE, 0x00000000);

            /* Canvas Area */
            RECT cvs = { pic_box.left + 4, pic_box.top + 22, pic_box.right - 4, pic_box.bottom - 22 };
            fill_rec(dev, &cvs, COLOR_WHITE);
            drw_rec(dev, &cvs);

            /* Decode and render exact specification GIF diagram */
            int ret = resolve_and_draw_gif(tb, dev, s, &cvs);
            if (ret != 0) {
                render_figure_diagram(dev, s, &cvs);
            }

            /* Caption */
            drw_tc_string(dev, pic_box.left + 8, pic_box.bottom - 17, s->style.img_caption[0] ? s->style.img_caption : "BTRON3 Figure Segment", COLOR_BLACK, 0x00000000);
        } else if (s->style.is_hr) {
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

    /* Restore device clip for window chrome layer */
    set_clip(dev, &orig_clip);

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
