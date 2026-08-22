/*
 * B-TRON Display Primitives Implementation: dp_core.c
 */

#include <btron/dp.h>
#include <stdlib.h>
#include <string.h>

GDEV* opn_dev(H w, H h) {
    if (w <= 0 || h <= 0) return NULL;
    GDEV *dev = (GDEV*)calloc(1, sizeof(GDEV));
    if (!dev) return NULL;

    dev->width = w;
    dev->height = h;
    dev->pixels = (COLOR*)calloc(w * h, sizeof(COLOR));
    if (!dev->pixels) {
        free(dev);
        return NULL;
    }

    dev->clip.left = 0;
    dev->clip.top = 0;
    dev->clip.right = w;
    dev->clip.bottom = h;

    /* Fill background with default transparent/black */
    for (int i = 0; i < w * h; i++) {
        dev->pixels[i] = 0x00000000;
    }

    return dev;
}

void cls_dev(GDEV *dev) {
    if (dev) {
        if (dev->pixels) free(dev->pixels);
        free(dev);
    }
}

void set_clip(GDEV *dev, const RECT *clip) {
    if (!dev) return;
    if (clip) {
        dev->clip = *clip;
        if (dev->clip.left < 0) dev->clip.left = 0;
        if (dev->clip.top < 0) dev->clip.top = 0;
        if (dev->clip.right > dev->width) dev->clip.right = dev->width;
        if (dev->clip.bottom > dev->height) dev->clip.bottom = dev->height;
    } else {
        dev->clip.left = 0;
        dev->clip.top = 0;
        dev->clip.right = dev->width;
        dev->clip.bottom = dev->height;
    }
}

ER drw_pnt(GDEV *dev, H x, H y) {
    if (!dev || !dev->pixels) return E_PAR;
    if (x < dev->clip.left || x >= dev->clip.right ||
        y < dev->clip.top || y >= dev->clip.bottom) {
        return E_OK; /* Clipped */
    }

    dev->pixels[y * dev->width + x] = COLOR_BLACK;
    return E_OK;
}

ER drw_lin(GDEV *dev, H x1, H y1, H x2, H y2) {
    if (!dev || !dev->pixels) return E_PAR;

    H dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    H dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    H err = dx + dy, e2;

    H x = x1, y = y1;

    while (1) {
        if (x >= dev->clip.left && x < dev->clip.right &&
            y >= dev->clip.top && y < dev->clip.bottom) {
            dev->pixels[y * dev->width + x] = COLOR_BLACK;
        }

        if (x == x2 && y == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }

    return E_OK;
}

ER drw_rec(GDEV *dev, const RECT *r) {
    if (!dev || !r) return E_PAR;

    drw_lin(dev, r->left, r->top, r->right - 1, r->top);
    drw_lin(dev, r->right - 1, r->top, r->right - 1, r->bottom - 1);
    drw_lin(dev, r->right - 1, r->bottom - 1, r->left, r->bottom - 1);
    drw_lin(dev, r->left, r->bottom - 1, r->left, r->top);

    return E_OK;
}

ER fill_rec(GDEV *dev, const RECT *r, COLOR col) {
    if (!dev || !r || !dev->pixels) return E_PAR;

    H left   = r->left < dev->clip.left ? dev->clip.left : r->left;
    H top    = r->top < dev->clip.top ? dev->clip.top : r->top;
    H right  = r->right > dev->clip.right ? dev->clip.right : r->right;
    H bottom = r->bottom > dev->clip.bottom ? dev->clip.bottom : r->bottom;

    for (H y = top; y < bottom; y++) {
        for (H x = left; x < right; x++) {
            dev->pixels[y * dev->width + x] = col;
        }
    }

    return E_OK;
}

ER drw_ovl(GDEV *dev, const RECT *r) {
    if (!dev || !r) return E_PAR;
    /* Draw approximate ellipse outline bounding box r */
    H rx = (r->right - r->left) / 2;
    H ry = (r->bottom - r->top) / 2;
    H cx = r->left + rx;
    H cy = r->top + ry;

    for (H y = r->top; y < r->bottom; y++) {
        for (H x = r->left; x < r->right; x++) {
            H dx = x - cx;
            H dy = y - cy;
            if (rx > 0 && ry > 0) {
                float val = ((float)(dx*dx)/(rx*rx)) + ((float)(dy*dy)/(ry*ry));
                if (val >= 0.85f && val <= 1.15f) {
                    if (x >= dev->clip.left && x < dev->clip.right &&
                        y >= dev->clip.top && y < dev->clip.bottom) {
                        dev->pixels[y * dev->width + x] = COLOR_BLACK;
                    }
                }
            }
        }
    }
    return E_OK;
}

ER fill_ovl(GDEV *dev, const RECT *r, COLOR col) {
    if (!dev || !r) return E_PAR;
    H rx = (r->right - r->left) / 2;
    H ry = (r->bottom - r->top) / 2;
    H cx = r->left + rx;
    H cy = r->top + ry;

    for (H y = r->top; y < r->bottom; y++) {
        for (H x = r->left; x < r->right; x++) {
            H dx = x - cx;
            H dy = y - cy;
            if (rx > 0 && ry > 0) {
                float val = ((float)(dx*dx)/(rx*rx)) + ((float)(dy*dy)/(ry*ry));
                if (val <= 1.0f) {
                    if (x >= dev->clip.left && x < dev->clip.right &&
                        y >= dev->clip.top && y < dev->clip.bottom) {
                        dev->pixels[y * dev->width + x] = col;
                    }
                }
            }
        }
    }
    return E_OK;
}
