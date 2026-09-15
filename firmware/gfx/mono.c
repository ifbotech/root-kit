#include "mono.h"
#include "font.h"
#include <stddef.h>
#include <string.h>

static int isqrt_m(int v)
{
    int x;
    if (v <= 0) {
        return 0;
    }
    x = v > 255 ? 255 : v;
    x = (x + v / x) / 2;
    x = (x + v / x) / 2;
    x = (x + v / x) / 2;
    x = (x + v / x) / 2;
    while (x * x > v)               { x--; }
    while ((x + 1) * (x + 1) <= v)  { x++; }
    return x;
}

void rk_mono_init(rk_mono_t *m, uint8_t *buf, int w, int h)
{
    m->buf = buf;
    m->w   = w;
    m->h   = h;
}

void rk_mono_clear(rk_mono_t *m, bool encendido)
{
    memset(m->buf, encendido ? 0xFF : 0x00, (size_t)(m->w * m->h / 8));
}

void rk_mono_px(rk_mono_t *m, int x, int y, bool on)
{
    int idx;

    if (m == NULL || x < 0 || y < 0 || x >= m->w || y >= m->h) {
        return;
    }
    idx = (y / 8) * m->w + x;
    if (on) {
        m->buf[idx] |= (uint8_t)(1u << (y & 7));
    } else {
        m->buf[idx] &= (uint8_t)~(1u << (y & 7));
    }
}

bool rk_mono_get(const rk_mono_t *m, int x, int y)
{
    if (m == NULL || x < 0 || y < 0 || x >= m->w || y >= m->h) {
        return false;
    }
    return (m->buf[(y / 8) * m->w + x] & (1u << (y & 7))) != 0u;
}

void rk_mono_hline(rk_mono_t *m, int x, int y, int w, bool on)
{
    int i;
    for (i = 0; i < w; i++) {
        rk_mono_px(m, x + i, y, on);
    }
}

void rk_mono_vline(rk_mono_t *m, int x, int y, int h, bool on)
{
    int i;
    for (i = 0; i < h; i++) {
        rk_mono_px(m, x, y + i, on);
    }
}

void rk_mono_fill_rect(rk_mono_t *m, int x, int y, int w, int h, bool on)
{
    int j;
    for (j = 0; j < h; j++) {
        rk_mono_hline(m, x, y + j, w, on);
    }
}

void rk_mono_rect(rk_mono_t *m, int x, int y, int w, int h, bool on)
{
    if (w <= 0 || h <= 0) {
        return;
    }
    rk_mono_hline(m, x, y, w, on);
    rk_mono_hline(m, x, y + h - 1, w, on);
    rk_mono_vline(m, x, y, h, on);
    rk_mono_vline(m, x + w - 1, y, h, on);
}

void rk_mono_fill_round(rk_mono_t *m, int x, int y, int w, int h, int r, bool on)
{
    int j, inset, d;

    if (w <= 0 || h <= 0) {
        return;
    }
    if (r * 2 > w) { r = w / 2; }
    if (r * 2 > h) { r = h / 2; }
    if (r < 0)     { r = 0; }

    for (j = 0; j < h; j++) {
        inset = 0;
        if (j < r) {
            d = r - j - 1;
            inset = r - isqrt_m(r * r - d * d);
        } else if (j >= h - r) {
            d = r - (h - j);
            inset = r - isqrt_m(r * r - d * d);
        }
        rk_mono_hline(m, x + inset, y + j, w - inset * 2, on);
    }
}

void rk_mono_disc(rk_mono_t *m, int cx, int cy, int r, bool on)
{
    int dy, dx;
    for (dy = -r; dy <= r; dy++) {
        dx = isqrt_m(r * r - dy * dy);
        rk_mono_hline(m, cx - dx, cy + dy, dx * 2 + 1, on);
    }
}

void rk_mono_arco(rk_mono_t *m, int cx, int cy, int rx, int ry,
                  bool arriba, bool on)
{
    int x, y;

    if (rx <= 0 || ry <= 0) {
        return;
    }
    /* Recorremos en x y despejamos y: a estos radios da una curva más pareja
     * que ir por ángulos, y no hace falta trigonometría. */
    for (x = -rx; x <= rx; x++) {
        int t = (rx * rx - x * x);
        if (t < 0) {
            continue;
        }
        y = (ry * isqrt_m(t)) / rx;
        rk_mono_px(m, cx + x, arriba ? cy - y : cy + y, on);
    }
}

int rk_mono_text(rk_mono_t *m, int x, int y, const char *s, bool on)
{
    int x0 = x;

    if (s == NULL) {
        return 0;
    }
    /* Reutilizamos los glifos de la Terminal dibujando pixel a pixel: son
     * pocas letras en el Spore y evita mantener dos tipografías. */
    while (*s != '\0') {
        char uno[2];
        int  row, col;

        uno[0] = *s++;
        uno[1] = '\0';
        if (rk_text_w(uno, 1) == 0) {
            continue;
        }
        for (row = 0; row < RK_GLYPH_H; row++) {
            for (col = 0; col < RK_GLYPH_W; col++) {
                if (rk_glyph_pixel(uno[0], col, row)) {
                    rk_mono_px(m, x + col, y + row, on);
                }
            }
        }
        x += RK_GLYPH_W + RK_TRACK;
    }
    return x - x0;
}

int rk_mono_encendidos(const rk_mono_t *m)
{
    int i, n = 0, bytes;
    uint8_t v;

    if (m == NULL) {
        return 0;
    }
    bytes = m->w * m->h / 8;
    for (i = 0; i < bytes; i++) {
        for (v = m->buf[i]; v != 0u; v >>= 1) {
            n += (v & 1u);
        }
    }
    return n;
}
