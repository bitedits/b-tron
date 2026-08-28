/*
 * B-TRON Specification Compatible Header: tip.h
 * Text Input Primitives (TIP) & Input Method Editor (IME) Subsystem.
 * Specification: btron-tip.tex (Cleanroom BTRON3 SPEC 3.20 Section 3.7) & IME.md
 */

#ifndef _BTRON_TIP_H_
#define _BTRON_TIP_H_

#include <btron/types.h>
#include <btron/error.h>
#include <btron/dp.h>
#include <btron/wnd.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TIP Input Modes (REQ-2.2) */
typedef enum {
    TIP_MODE_ASCII = 0,
    TIP_MODE_HIRAGANA,
    TIP_MODE_KATAKANA
} TIP_INPUT_MODE;

/* TIP DFA State Machine States (Section 6, Formal Verification) */
typedef enum {
    TIP_STATE_IDLE = 0,
    TIP_STATE_PRECOMP,
    TIP_STATE_CONVERTING,
    TIP_STATE_CANDIDATE_SELECT
} TIP_DFA_STATE;

#define TIP_MAX_PRECOMP_LEN  256
#define TIP_MAX_CLAUSES      32
#define TIP_MAX_CANDIDATES   16
#define TIP_MAX_STR_LEN      64

/* Candidate Entry with Semantic Category Annotation (Section 4.2 / REQ-5) */
typedef struct {
    char value[TIP_MAX_STR_LEN];
    char annotation[TIP_MAX_STR_LEN];
    int cost;
} TIP_CANDIDATE;

/* Bunsetsu Clause Segment (Section 3.2) */
typedef struct {
    char reading[TIP_MAX_STR_LEN];
    char converted[TIP_MAX_STR_LEN];
    TIP_CANDIDATE candidates[TIP_MAX_CANDIDATES];
    int num_candidates;
    int selected_candidate;
} TIP_CLAUSE;

/* TIP Composition Context */
typedef struct {
    TIP_INPUT_MODE mode;
    TIP_DFA_STATE state;

    char romaji_buf[TIP_MAX_PRECOMP_LEN];
    int romaji_len;

    char reading_buf[TIP_MAX_PRECOMP_LEN];
    int reading_len;

    TIP_CLAUSE clauses[TIP_MAX_CLAUSES];
    int num_clauses;
    int active_clause;

    /* Window positioning */
    PNT caret_pos;
    BOOL candidate_window_visible;
} TIP_CONTEXT;

/* Global TIP API */
ER tip_init(void);
TIP_DFA_STATE tip_get_state(void);
TIP_INPUT_MODE tip_get_mode(void);
void tip_set_mode(TIP_INPUT_MODE mode);
void tip_toggle_mode(void);
BOOL tip_is_active(void);
BOOL tip_is_candidate_window_visible(void);

/* Feed keyboard event to TIP. Returns TRUE if event was handled by TIP.
 * If text is committed, out_commit receives the UTF-8 confirmed string. */
BOOL tip_process_key(UW key, UW modifiers, char *out_commit, int max_commit);

/* Cancel composition and restore DFA to TIP_STATE_IDLE in O(1) time (Theorem 2) */
void tip_cancel(void);

/* Get current composition display string and active reading */
const char* tip_get_reading(void);
const char* tip_get_converted_text(char *out_buf, int max_len);
int tip_get_composition_length(void);

/* Render floating candidate window with classic B-TRON double border */
void tip_render_candidate_window(GDEV *dev, H caret_x, H caret_y);

/* Update caret coordinates for candidate window positioning */
void tip_set_caret_pos(H x, H y);
H tip_get_caret_x(void);
H tip_get_caret_y(void);

/* ── BTRON3 SPEC 3.20 Section 3.7 TIP Port API Compatibility Layer ── */
#define TIP_OUT 0x0001 /* Committed characters output */
#define TIP_CNV 0x0002 /* Converted preedit modified */
#define TIP_CAR 0x0004 /* Caret position changed */
#define TIP_CL  0x0008 /* Candidate list changed */

typedef struct {
    UW result;      /* Result flags: TIP_OUT, TIP_CNV, TIP_CAR, TIP_CL */
    char *out_str;  /* Buffer for committed string */
    int  out_len;   /* Length of committed string */
    char *cnv_str;  /* Buffer for active composition string */
    int  cnv_len;   /* Length of active composition string */
    int  car_pos;   /* Caret offset in composition string */
} TIPREC;

ID iopn_tip(WND *wnd);
ER icls_tip(ID tipid);
ER ichg_mod(ID tipid, W mode);
W  iput_key(ID tipid, UW key, UW mod, TIPREC *rec);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_TIP_H_ */
