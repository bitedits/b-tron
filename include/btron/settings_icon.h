/*
 * settings_icon.h — Inline 32×32 GIF icon loader for Control Panel & Settings Applets
 * B-System (BTRON 3.20)
 *
 * Usage: include this header once per .c file that needs to draw a settings icon.
 * Call: draw_setting_gif_icon(dev, "appearance", x, y);
 *       The id_str must match a file in assets/icons/<id_str>.gif
 *
 * Works only in hosted builds (__STDC_HOSTED__ == 1).
 * In freestanding builds the function is a no-op.
 */

#ifndef _BTRON_SETTINGS_ICON_H_
#define _BTRON_SETTINGS_ICON_H_

#include <btron/dp.h>

#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define SI_LZW_DICT    4096
#define SI_MAX_PIXELS  4096  /* 64×64 = 4096 pixels max */

/* Static GIF decode buffers — one set is enough (no re-entrance needed) */
static uint16_t s_si_gif_prefix[SI_LZW_DICT];
static uint8_t  s_si_gif_suffix[SI_LZW_DICT];
static uint8_t  s_si_gif_stack [SI_LZW_DICT + 1];
static uint8_t  s_si_gif_raw   [SI_MAX_PIXELS];

/*
 * draw_setting_gif_icon — draw a 32×32 GIF icon at (dst_x, dst_y).
 * id_str is the base name (e.g. "appearance").
 * Searches the following prefixes in order until a file opens successfully.
 */
static void draw_setting_gif_icon(GDEV *dev, const char *id_str, int dst_x, int dst_y) {
    if (!dev || !dev->pixels || !id_str || id_str[0] == '\0') return;

    static const char *prefixes[] = {
        "assets/icons/",
        "../assets/icons/",
        "assets/",
        "../assets/",
        NULL
    };

    FILE *fp = NULL;
    char path[128];
    for (int p = 0; prefixes[p]; p++) {
        snprintf(path, sizeof(path), "%s%s.gif", prefixes[p], id_str);
        fp = fopen(path, "rb");
        if (fp) break;
    }
    if (!fp) return;

    /* --- GIF89a / GIF87a decoder (32×32 subset) --- */
    uint8_t hdr[13];
    if (fread(hdr, 1, 13, fp) != 13) { fclose(fp); return; }
    if (memcmp(hdr, "GIF87a", 6) != 0 && memcmp(hdr, "GIF89a", 6) != 0) { fclose(fp); return; }

    uint8_t flags = hdr[10];
    int has_gct  = (flags & 0x80) != 0;
    int gct_size = 1 << ((flags & 0x07) + 1);
    uint32_t gct[256];
    memset(gct, 0, sizeof(gct));

    if (has_gct) {
        uint8_t gct_raw[768];
        if (fread(gct_raw, 1, gct_size * 3, fp) != (size_t)(gct_size * 3)) { fclose(fp); return; }
        for (int i = 0; i < gct_size; i++) {
            gct[i] = ((uint32_t)gct_raw[i*3] << 16) | ((uint32_t)gct_raw[i*3+1] << 8) | gct_raw[i*3+2];
        }
    }

    int trans_idx = -1;
    int img_w = 0, img_h = 0;
    int img_read = 0;

    while (!feof(fp)) {
        int b = fgetc(fp);
        if (b == EOF || b == 0x3B) break;
        if (b == 0x21) {
            int ext_label = fgetc(fp);
            if (ext_label == 0xF9) {
                int block_size = fgetc(fp);
                if (block_size == 4) {
                    uint8_t gce[4];
                    if (fread(gce, 1, 4, fp) == 4 && (gce[0] & 0x01)) trans_idx = gce[3];
                }
                while (1) { int l = fgetc(fp); if (l <= 0) break; fseek(fp, l, SEEK_CUR); }
            } else {
                while (1) { int l = fgetc(fp); if (l <= 0) break; fseek(fp, l, SEEK_CUR); }
            }
        } else if (b == 0x2C) {
            uint8_t idesc[9];
            if (fread(idesc, 1, 9, fp) != 9) break;
            img_w = idesc[4] | (idesc[5] << 8);
            img_h = idesc[6] | (idesc[7] << 8);
            uint8_t iflags = idesc[8];
            int has_lct  = (iflags & 0x80) != 0;
            int lct_size = 1 << ((iflags & 0x07) + 1);
            uint32_t lct[256];
            memset(lct, 0, sizeof(lct));
            uint32_t *palette = gct;
            if (has_lct) {
                uint8_t lct_raw[768];
                if (fread(lct_raw, 1, lct_size * 3, fp) != (size_t)(lct_size * 3)) break;
                for (int i = 0; i < lct_size; i++) {
                    lct[i] = ((uint32_t)lct_raw[i*3] << 16) | ((uint32_t)lct_raw[i*3+1] << 8) | lct_raw[i*3+2];
                }
                palette = lct;
            }

            int min_code_size = fgetc(fp);
            if (min_code_size < 2 || min_code_size > 8) break;
            int clear_code = 1 << min_code_size;
            int eoi_code   = clear_code + 1;
            int code_size  = min_code_size + 1;
            int code_mask  = (1 << code_size) - 1;
            int next_code  = eoi_code + 1;

            int bit_count = 0;
            uint32_t bit_buf = 0;
            int pixel_count = 0;
            int total_pixels = img_w * img_h;
            if (total_pixels > SI_MAX_PIXELS) total_pixels = SI_MAX_PIXELS;

            int old_code = -1, first_char = 0, stack_top = 0;
            for (int i = 0; i < clear_code; i++) {
                s_si_gif_prefix[i] = 0;
                s_si_gif_suffix[i] = (uint8_t)i;
            }

            uint8_t sub_buf[256];
            int sub_len = 0, sub_pos = 0;

            while (pixel_count < total_pixels) {
                while (bit_count < code_size) {
                    if (sub_pos >= sub_len) {
                        sub_len = fgetc(fp);
                        if (sub_len <= 0) break;
                        if (fread(sub_buf, 1, sub_len, fp) != (size_t)sub_len) break;
                        sub_pos = 0;
                    }
                    bit_buf |= ((uint32_t)sub_buf[sub_pos++] << bit_count);
                    bit_count += 8;
                }
                if (bit_count < code_size) break;
                int code = (int)(bit_buf & (uint32_t)code_mask);
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
                    s_si_gif_stack[stack_top++] = (uint8_t)first_char;
                    cur_code = old_code;
                }
                while (cur_code >= clear_code && cur_code < SI_LZW_DICT) {
                    s_si_gif_stack[stack_top++] = s_si_gif_suffix[cur_code];
                    cur_code = s_si_gif_prefix[cur_code];
                }
                first_char = s_si_gif_suffix[cur_code];
                s_si_gif_stack[stack_top++] = (uint8_t)first_char;

                while (stack_top > 0 && pixel_count < total_pixels) {
                    s_si_gif_raw[pixel_count++] = s_si_gif_stack[--stack_top];
                }

                if (old_code >= 0 && next_code < SI_LZW_DICT) {
                    s_si_gif_prefix[next_code] = (uint16_t)old_code;
                    s_si_gif_suffix[next_code] = (uint8_t)first_char;
                    next_code++;
                    if (next_code > code_mask && code_size < 12) {
                        code_size++;
                        code_mask = (1 << code_size) - 1;
                    }
                }
                old_code = code;
            }

            for (int y = 0; y < img_h && y < 64; y++) {
                    int out_y = dst_y + y;
                    if (out_y < 0 || out_y >= dev->height) continue;
                    if (out_y < dev->clip.top || out_y >= dev->clip.bottom) continue;
                    for (int x = 0; x < img_w && x < 64; x++) {
                        int out_x = dst_x + x;
                        if (out_x < 0 || out_x >= dev->width) continue;
                        if (out_x < dev->clip.left || out_x >= dev->clip.right) continue;
                        uint8_t p_idx = s_si_gif_raw[y * img_w + x];
                        if ((int)p_idx == trans_idx) continue;
                        uint32_t col = palette[p_idx];
                        dev->pixels[out_y * dev->width + out_x] = (COLOR)(0xFF000000 | col);
                    }
                }
                img_read = 1;
            }
            break;
        }
    }
    fclose(fp);
    (void)img_read;
}

#else  /* freestanding */

static void draw_setting_gif_icon(GDEV *dev, const char *id_str, int dst_x, int dst_y) {
    (void)dev; (void)id_str; (void)dst_x; (void)dst_y;
}

#endif /* __STDC_HOSTED__ */

#endif /* _BTRON_SETTINGS_ICON_H_ */
