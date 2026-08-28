/*
 * B-TRON Text Input Primitives (TIP) - Input Front-End (IFE): tip_ife.c
 * Cleanroom implementation conforming to btron-tip.tex Section 3 & 4.
 */

#include <btron/tip.h>
#include <btron/event.h>
#include <btron/mozc_engine.h>
#include <btron/troncode.h>
#include <btron/wnd.h>

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#else
#include <stddef.h>
#include <stdint.h>
#include <libstr.h>
#define strlen   tkl_strlen
#define strcmp   tkl_strcmp
#define strncpy  tkl_strncpy
#define memcpy   tkl_memcpy
#define memset   tkl_memset
static inline int snprintf(char *str, size_t size, const char *format, ...) {
    (void)format;
    if (size > 0) str[0] = '\0';
    return 0;
}
#endif

static TIP_CONTEXT g_tip;

ER tip_init(void) {
    memset(&g_tip, 0, sizeof(TIP_CONTEXT));
    g_tip.mode = TIP_MODE_HIRAGANA; /* Japanese IME active by default */
    g_tip.state = TIP_STATE_IDLE;
    mozc_engine_init();
    return E_OK;
}

TIP_DFA_STATE tip_get_state(void) {
    return g_tip.state;
}

TIP_INPUT_MODE tip_get_mode(void) {
    return g_tip.mode;
}

void tip_set_mode(TIP_INPUT_MODE mode) {
    g_tip.mode = mode;
    if (g_tip.state != TIP_STATE_IDLE) {
        tip_cancel();
    }
}

void tip_toggle_mode(void) {
    if (g_tip.mode == TIP_MODE_ASCII) {
        g_tip.mode = TIP_MODE_HIRAGANA;
    } else {
        g_tip.mode = TIP_MODE_ASCII;
    }
    tip_cancel();
}

BOOL tip_is_active(void) {
    return (g_tip.mode != TIP_MODE_ASCII && g_tip.state != TIP_STATE_IDLE);
}

/*
 * Theorem 2 (Composition Safety and Non-Latching):
 * Cancellation unconditionally restores DFA to TIP_STATE_IDLE in O(1).
 */
void tip_cancel(void) {
    g_tip.state = TIP_STATE_IDLE;
    g_tip.romaji_buf[0] = '\0';
    g_tip.romaji_len = 0;
    g_tip.reading_buf[0] = '\0';
    g_tip.reading_len = 0;
    g_tip.num_clauses = 0;
    g_tip.active_clause = 0;
    g_tip.candidate_window_visible = FALSE;
}

void tip_set_caret_pos(H x, H y) {
    g_tip.caret_pos.x = x;
    g_tip.caret_pos.y = y;
}

const char* tip_get_reading(void) {
    return g_tip.reading_buf;
}

const char* tip_get_converted_text(char *out_buf, int max_len) {
    if (!out_buf || max_len <= 0) return "";
    out_buf[0] = '\0';

    if (g_tip.state == TIP_STATE_IDLE) {
        return "";
    }

    if (g_tip.state == TIP_STATE_PRECOMP) {
        strncpy(out_buf, g_tip.reading_buf, max_len - 1);
        return out_buf;
    }

    /* Assembled converted bunsetsu clauses */
    int out_idx = 0;
    for (int i = 0; i < g_tip.num_clauses && out_idx < max_len - 1; i++) {
        const char *txt = g_tip.clauses[i].converted;
        int len = (int)strlen(txt);
        if (out_idx + len < max_len - 1) {
            memcpy(&out_buf[out_idx], txt, len);
            out_idx += len;
        }
    }
    out_buf[out_idx] = '\0';
    return out_buf;
}

int tip_get_composition_length(void) {
    char buf[TIP_MAX_PRECOMP_LEN];
    tip_get_converted_text(buf, sizeof(buf));
    return (int)strlen(buf);
}

/*
 * Event Processor: Dispatches keyboard events through DFA state machine
 */
BOOL tip_process_key(UW key, UW modifiers, char *out_commit, int max_commit) {
    if (out_commit) out_commit[0] = '\0';

    /* Mode Toggle: Ctrl+Space, Shift+Space, Alt+Space, Hankaku/Zenkaku, or F10 */
    if ((key == ' ' && (modifiers & 0x03C3)) || /* Any Ctrl, Shift, or Alt modifier on Space */
        key == 0x100 ||                          /* Hankaku / Zenkaku key */
        key == BTRON_KEY_F10) {                  /* F10 (0x40000043): Standard Mode Toggle */
        tip_toggle_mode();
        return TRUE;
    }

    /* Functional Key Transformations during composition */
    if (g_tip.state != TIP_STATE_IDLE) {
        if (key == BTRON_KEY_F6) {
            /* F6: Convert active composition to Hiragana */
            if (g_tip.state == TIP_STATE_PRECOMP) {
                mozc_lattice_search(g_tip.reading_buf, g_tip.clauses, &g_tip.num_clauses, TIP_MAX_CLAUSES);
                g_tip.state = TIP_STATE_CONVERTING;
            }
            for (int i = 0; i < g_tip.num_clauses; i++) {
                strncpy(g_tip.clauses[i].converted, g_tip.clauses[i].reading, TIP_MAX_STR_LEN - 1);
            }
            return TRUE;
        }
        if (key == BTRON_KEY_F7) {
            /* F7: Convert active composition to Fullwidth Katakana */
            if (g_tip.state == TIP_STATE_PRECOMP) {
                mozc_lattice_search(g_tip.reading_buf, g_tip.clauses, &g_tip.num_clauses, TIP_MAX_CLAUSES);
                g_tip.state = TIP_STATE_CONVERTING;
            }
            for (int i = 0; i < g_tip.num_clauses; i++) {
                mozc_hiragana_to_katakana(g_tip.clauses[i].reading, g_tip.clauses[i].converted, TIP_MAX_STR_LEN);
            }
            return TRUE;
        }
        if (key == BTRON_KEY_F8) {
            /* F8: Convert active composition to Halfwidth Katakana */
            if (g_tip.state == TIP_STATE_PRECOMP) {
                mozc_lattice_search(g_tip.reading_buf, g_tip.clauses, &g_tip.num_clauses, TIP_MAX_CLAUSES);
                g_tip.state = TIP_STATE_CONVERTING;
            }
            for (int i = 0; i < g_tip.num_clauses; i++) {
                mozc_hiragana_to_halfwidth_katakana(g_tip.clauses[i].reading, g_tip.clauses[i].converted, TIP_MAX_STR_LEN);
            }
            return TRUE;
        }
        if (key == BTRON_KEY_F9) {
            /* F9: Convert active composition to Fullwidth Alphanumeric */
            g_tip.state = TIP_STATE_CONVERTING;
            g_tip.num_clauses = 1;
            g_tip.active_clause = 0;
            strncpy(g_tip.clauses[0].reading, g_tip.reading_buf, TIP_MAX_STR_LEN - 1);
            mozc_alphanumeric_to_fullwidth(g_tip.romaji_buf, g_tip.clauses[0].converted, TIP_MAX_STR_LEN);
            return TRUE;
        }
    } else {
        /* In IDLE mode, F6 / F7 switches global input mode */
        if (key == BTRON_KEY_F6) {
            tip_set_mode(TIP_MODE_HIRAGANA);
            return TRUE;
        }
        if (key == BTRON_KEY_F7) {
            tip_set_mode(TIP_MODE_KATAKANA);
            return TRUE;
        }
    }

    if (g_tip.mode == TIP_MODE_ASCII) {
        return FALSE; /* Not handled by TIP in direct ASCII mode */
    }

    /* Theorem 2: ESC key restores to IDLE in O(1) */
    if (key == 0x1B /* ESC */) {
        if (g_tip.state != TIP_STATE_IDLE) {
            tip_cancel();
            return TRUE;
        }
        return FALSE;
    }

    /* Backspace handling */
    if (key == 0x08 /* Backspace */) {
        if (g_tip.state == TIP_STATE_CANDIDATE_SELECT) {
            g_tip.state = TIP_STATE_CONVERTING;
            g_tip.candidate_window_visible = FALSE;
            return TRUE;
        }
        if (g_tip.state == TIP_STATE_CONVERTING) {
            g_tip.state = TIP_STATE_PRECOMP;
            g_tip.candidate_window_visible = FALSE;
            return TRUE;
        }
        if (g_tip.state == TIP_STATE_PRECOMP) {
            if (g_tip.romaji_len > 0) {
                g_tip.romaji_len--;
                g_tip.romaji_buf[g_tip.romaji_len] = '\0';
                mozc_romaji_to_hiragana(g_tip.romaji_buf, g_tip.reading_buf, sizeof(g_tip.reading_buf));
                g_tip.reading_len = (int)strlen(g_tip.reading_buf);

                if (g_tip.romaji_len == 0) {
                    g_tip.state = TIP_STATE_IDLE;
                }
                return TRUE;
            }
        }
        return FALSE;
    }

    /* Enter / Return confirmation */
    if (key == '\r' || key == '\n') {
        if (g_tip.state != TIP_STATE_IDLE) {
            if (out_commit && max_commit > 0) {
                tip_get_converted_text(out_commit, max_commit);
            }
            tip_cancel();
            return TRUE;
        }
        return FALSE;
    }

    /* Space key: trigger Mozc Kana-Kanji conversion or advance candidate */
    if (key == ' ') {
        if (g_tip.state == TIP_STATE_PRECOMP) {
            /* Execute Mozc lattice search and show candidate window immediately */
            mozc_lattice_search(g_tip.reading_buf, g_tip.clauses, &g_tip.num_clauses, TIP_MAX_CLAUSES);
            g_tip.active_clause = 0;
            g_tip.state = TIP_STATE_CONVERTING;
            g_tip.candidate_window_visible = TRUE;
            return TRUE;
        } else if (g_tip.state == TIP_STATE_CONVERTING || g_tip.state == TIP_STATE_CANDIDATE_SELECT) {
            /* Open Candidate window / cycle candidate */
            g_tip.state = TIP_STATE_CANDIDATE_SELECT;
            g_tip.candidate_window_visible = TRUE;
            if (g_tip.num_clauses > 0) {
                TIP_CLAUSE *c = &g_tip.clauses[g_tip.active_clause];
                if (c->num_candidates > 0) {
                    c->selected_candidate = (c->selected_candidate + 1) % c->num_candidates;
                    strncpy(c->converted, c->candidates[c->selected_candidate].value, TIP_MAX_STR_LEN - 1);
                }
            }
            return TRUE;
        }
        return FALSE;
    }

    /* Numeric selection in candidate select mode (1-9) */
    if (g_tip.state == TIP_STATE_CANDIDATE_SELECT && key >= '1' && key <= '9') {
        int sel = (key - '1');
        if (g_tip.num_clauses > 0) {
            TIP_CLAUSE *c = &g_tip.clauses[g_tip.active_clause];
            if (sel < c->num_candidates) {
                c->selected_candidate = sel;
                strncpy(c->converted, c->candidates[sel].value, TIP_MAX_STR_LEN - 1);
                /* Advance to next clause or confirm */
                if (g_tip.active_clause + 1 < g_tip.num_clauses) {
                    g_tip.active_clause++;
                } else {
                    g_tip.state = TIP_STATE_CONVERTING;
                    g_tip.candidate_window_visible = FALSE;
                }
            }
        }
        return TRUE;
    }

    /* Printable ASCII character input */
    if (key >= 32 && key <= 126) {
        if (g_tip.state == TIP_STATE_CONVERTING || g_tip.state == TIP_STATE_CANDIDATE_SELECT) {
            /* Commit current conversion first */
            if (out_commit && max_commit > 0) {
                tip_get_converted_text(out_commit, max_commit);
            }
            tip_cancel();
        }

        if (g_tip.romaji_len < TIP_MAX_PRECOMP_LEN - 2) {
            g_tip.romaji_buf[g_tip.romaji_len++] = (char)key;
            g_tip.romaji_buf[g_tip.romaji_len] = '\0';

            mozc_romaji_to_hiragana(g_tip.romaji_buf, g_tip.reading_buf, sizeof(g_tip.reading_buf));
            g_tip.reading_len = (int)strlen(g_tip.reading_buf);
            g_tip.state = TIP_STATE_PRECOMP;
            return TRUE;
        }
    }

    return FALSE;
}


H tip_get_caret_x(void) {
    return g_tip.caret_pos.x > 0 ? g_tip.caret_pos.x : 240;
}

H tip_get_caret_y(void) {
    return g_tip.caret_pos.y > 0 ? g_tip.caret_pos.y : 200;
}

BOOL tip_is_candidate_window_visible(void) {
    return g_tip.candidate_window_visible;
}

/*
 * Render Floating Candidate Window in classic Sakamura double-bordered style (btron-tip.tex Section 4.2)
 */
void tip_render_candidate_window(GDEV *dev, H caret_x, H caret_y) {
    if (!dev || !g_tip.candidate_window_visible ||
        (g_tip.state != TIP_STATE_CONVERTING && g_tip.state != TIP_STATE_CANDIDATE_SELECT)) {
        return;
    }

    if (g_tip.num_clauses == 0) return;
    const TIP_CLAUSE *clause = &g_tip.clauses[g_tip.active_clause];
    if (clause->num_candidates == 0) return;

    H win_w = 260;
    H item_h = 18;
    H header_h = 22;
    H footer_h = 20;
    H win_h = header_h + clause->num_candidates * item_h + footer_h + 4;

    H win_x = caret_x;
    H win_y = caret_y + 20;

    /* Prevent overflowing screen boundary */
    if (win_x + win_w > dev->width) win_x = dev->width - win_w - 4;
    if (win_y + win_h > dev->height) win_y = dev->height - win_h - 4;
    if (win_x < 4) win_x = 4;
    if (win_y < 4) win_y = 4;

    RECT win_rect = { win_x, win_y, win_x + win_w, win_y + win_h };

    /* 1. Window Background */
    fill_rec(dev, &win_rect, COLOR_WHITE);

    /* 2. Sakamura Double Border (border width = 2) */
    drw_rec(dev, &win_rect);
    RECT inner_rect = { win_x + 2, win_y + 2, win_x + win_w - 2, win_y + win_h - 2 };
    drw_rec(dev, &inner_rect);

    /* 3. Title Bar: "Candidates - Mozc [x]" */
    RECT title_rect = { win_x + 3, win_y + 3, win_x + win_w - 3, win_y + header_h };
    fill_rec(dev, &title_rect, COLOR_LTGRAY);
    drw_lin(dev, win_x + 2, win_y + header_h, win_x + win_w - 2, win_y + header_h);
    drw_tc_string(dev, win_x + 8, win_y + 5, "Candidates - Mozc", COLOR_NAVY, 0x00000000);
    drw_tc_string(dev, win_x + win_w - 22, win_y + 5, "[x]", COLOR_BLACK, 0x00000000);

    /* 4. Candidate List with Categories */
    H list_y = win_y + header_h + 2;
    for (int i = 0; i < clause->num_candidates; i++) {
        RECT item_rect = { win_x + 4, list_y, win_x + win_w - 4, list_y + item_h };
        COLOR fg = COLOR_BLACK;
        COLOR bg = 0x00000000;

        if (i == clause->selected_candidate) {
            fill_rec(dev, &item_rect, COLOR_NAVY);
            fg = COLOR_WHITE;
        }

        char item_buf[64];
        snprintf(item_buf, sizeof(item_buf), "%d  %s", i + 1, clause->candidates[i].value);
        drw_tc_string(dev, win_x + 8, list_y + 2, item_buf, fg, bg);

        /* Category Annotation */
        if (clause->candidates[i].annotation[0] != '\0') {
            char ann_buf[32];
            snprintf(ann_buf, sizeof(ann_buf), "(%s)", clause->candidates[i].annotation);
            drw_tc_string(dev, win_x + 160, list_y + 2, ann_buf, i == clause->selected_candidate ? COLOR_YELLOW : COLOR_GRAY, bg);
        }

        list_y += item_h;
    }

    /* 5. Footer separator and hints */
    drw_lin(dev, win_x + 4, list_y + 2, win_x + win_w - 4, list_y + 2);
    drw_tc_string(dev, win_x + 8, list_y + 5, "1-9 Select   Esc Cancel", COLOR_GRAY, 0x00000000);
}

/* ── BTRON3 SPEC 3.20 Section 3.7 Syscall Implementation ── */
#define MAX_TIP_PORTS 16
static TIP_CONTEXT g_tip_ports[MAX_TIP_PORTS];
static BOOL g_port_used[MAX_TIP_PORTS] = { FALSE };

ID iopn_tip(WND *wnd) {
    (void)wnd;
    for (int i = 0; i < MAX_TIP_PORTS; i++) {
        if (!g_port_used[i]) {
            g_port_used[i] = TRUE;
            memset(&g_tip_ports[i], 0, sizeof(TIP_CONTEXT));
            g_tip_ports[i].mode = TIP_MODE_HIRAGANA;
            g_tip_ports[i].state = TIP_STATE_IDLE;
            return (ID)(i + 1);
        }
    }
    return (ID)E_LIMIT;
}

ER icls_tip(ID tipid) {
    int idx = (int)tipid - 1;
    if (idx < 0 || idx >= MAX_TIP_PORTS || !g_port_used[idx]) return E_ID;
    g_port_used[idx] = FALSE;
    return E_OK;
}

ER ichg_mod(ID tipid, W mode) {
    int idx = (int)tipid - 1;
    if (idx < 0 || idx >= MAX_TIP_PORTS || !g_port_used[idx]) return E_ID;
    g_tip_ports[idx].mode = (TIP_INPUT_MODE)mode;
    return E_OK;
}

W iput_key(ID tipid, UW key, UW mod, TIPREC *rec) {
    (void)tipid;
    if (!rec) return 0;
    rec->result = 0;

    char commit[TIP_MAX_PRECOMP_LEN] = "";
    BOOL handled = tip_process_key(key, mod, commit, sizeof(commit));

    if (commit[0] != '\0') {
        rec->result |= TIP_OUT;
        if (rec->out_str && rec->out_len > 0) {
            strncpy(rec->out_str, commit, rec->out_len - 1);
            rec->out_str[rec->out_len - 1] = '\0';
        }
    }

    if (g_tip.state != TIP_STATE_IDLE) {
        rec->result |= TIP_CNV;
        if (rec->cnv_str && rec->cnv_len > 0) {
            tip_get_converted_text(rec->cnv_str, rec->cnv_len);
        }
        rec->car_pos = tip_get_composition_length();
        rec->result |= TIP_CAR;
    }

    if (tip_is_candidate_window_visible()) {
        rec->result |= TIP_CL;
    }

    return handled ? 1 : 0;
}
