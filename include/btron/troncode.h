/*
 * B-TRON Specification Compatible Header: troncode.h
 * TRON Multilingual Character Code System & Font Engine Header.
 */

#ifndef _BTRON_TRONCODE_H_
#define _BTRON_TRONCODE_H_

#include <btron/types.h>
#include <btron/error.h>
#include <btron/dp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TRON Code Plane Definitions */
#define TC_PLANE_ASCII   0x00
#define TC_PLANE_KANJI1  0x21
#define TC_PLANE_KANA    0x22

/* Convert UTF-8 sequence to TRON 16-bit Code */
TC utf8_to_tc(const char *utf8_str, int *bytes_consumed);

/* Render TRON Code text string onto Display Primitive Device */
ER drw_tc_string(GDEV *dev, H x, H y, const char *text, COLOR fg_col, COLOR bg_col);

/* Get pixel bitmap for a character (8x16 ASCII / 16x16 Kanji/Kana) */
const UB* get_glyph_bitmap(TC code, H *out_width, H *out_height);

#ifdef __cplusplus
}
#endif

#endif /* _BTRON_TRONCODE_H_ */
