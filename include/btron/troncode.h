/*
 * B-TRON Specification Compatible Header: troncode.h
 * TRON Multilingual Character Code System & Font Engine Header.
 * Implements Section 2 of btron-tip.tex: Multi-Plane TRON Code vs. Unicode.
 */

#ifndef _BTRON_TRONCODE_H_
#define _BTRON_TRONCODE_H_

#include <btron/types.h>
#include <btron/error.h>
#include <btron/dp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TRON Code Plane Definitions (Section 2.2) */
#define TC_PLANE_ASCII     0x00
#define TC_PLANE_KANJI1    0x21 /* JIS X 0208 standard */
#define TC_PLANE_KANJI2    0x22 /* JIS X 0212 supplementary */
#define TC_PLANE_KANA      0x23
#define TC_PLANE_FALLBACK  0xFF /* Plane 255: Unicode mapping fallback */

/* TRON Code escape sequences */
#define TC_ESC_SINGLE_BYTE 0x00 /* 0x00 - 0x7F ASCII */
#define TC_ESC_PLANE_SHIFT 0xFE /* 2-byte Shift */
#define TC_ESC_EXT_MODE    0xFF /* 4-byte Universal */

/* Bidirectional Conversion Functions (Theorem 1: Round-trip Preservation) */
/* phi: U_UTF-8 -> T_TRON */
TC utf8_to_tc(const char *utf8_str, int *bytes_consumed);
int utf8_to_tc_string(const char *utf8_in, TC *tc_out, int max_chars);

/* phi^-1: T_TRON -> U_UTF-8 */
int tc_to_utf8(TC code, char *utf8_buf, int max_len);
int tc_to_utf8_string(const TC *tc_in, int tc_len, char *utf8_out, int max_bytes);

/* Render TRON Code text string onto Display Primitive Device */
ER drw_tc_string(GDEV *dev, H x, H y, const char *text, COLOR fg_col, COLOR bg_col);

/* Render TRON Code string with underline styling (TIP feedback) */
ER drw_tc_string_underlined(GDEV *dev, H x, H y, const char *text, COLOR fg_col, COLOR bg_col, BOOL dotted);

/* Get pixel bitmap for a character (8x16 ASCII / 16x16 Kanji/Kana) */
const UB* get_glyph_bitmap(TC code, H *out_width, H *out_height);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_TRONCODE_H_ */
