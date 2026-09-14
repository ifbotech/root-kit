#include "fb.h"
#include <stddef.h>

/* Raíz cuadrada entera por Newton. Converge de sobra para los radios que
 * usamos (< 128) y evita arrastrar libm al firmware. */
static int isqrt_i(int v)
{
    int x;
    if (v <= 0) {
        return 0;
    }
    x = v;
    if (x > 255) { x = 255; }
    x = (x + v / x) / 2;
    x = (x + v / x) / 2;
    x = (x + v / x) / 2;
    x = (x + v / x) / 2;
    while (x * x > v)       { x--; }
    while ((x + 1) * (x + 1) <= v) { x++; }
    return x;
}


void rk_fb_init(rk_fb_t *fb, rk_color_t *px, int w, int h)
{
    fb->px = px;
    fb->w  = w;
    fb->h  = h;
}

void rk_fb_clear(rk_fb_t *fb, rk_color_t c)
{
    int n = fb->w * fb->h;
    int i;
    for (i = 0; i < n; i++) {
        fb->px[i] = c;
    }
}

void rk_px(rk_fb_t *fb, int x, int y, rk_color_t c)
{
    if (x < 0 || y < 0 || x >= fb->w || y >= fb->h) {
        return;
    }
    fb->px[y * fb->w + x] = c;
}

void rk_hline(rk_fb_t *fb, int x, int y, int w, rk_color_t c)
{
    int i;
    if (y < 0 || y >= fb->h) {
        return;
    }
    if (x < 0) { w += x; x = 0; }
    if (x + w > fb->w) { w = fb->w - x; }
    for (i = 0; i < w; i++) {
        fb->px[y * fb->w + x + i] = c;
    }
}

void rk_vline(rk_fb_t *fb, int x, int y, int h, rk_color_t c)
{
    int i;
    if (x < 0 || x >= fb->w) {
        return;
    }
    if (y < 0) { h += y; y = 0; }
    if (y + h > fb->h) { h = fb->h - y; }
    for (i = 0; i < h; i++) {
        fb->px[(y + i) * fb->w + x] = c;
    }
}

void rk_fill_rect(rk_fb_t *fb, int x, int y, int w, int h, rk_color_t c)
{
    int j;
    for (j = 0; j < h; j++) {
        rk_hline(fb, x, y + j, w, c);
    }
}

void rk_rect(rk_fb_t *fb, int x, int y, int w, int h, rk_color_t c)
{
    rk_hline(fb, x, y, w, c);
    rk_hline(fb, x, y + h - 1, w, c);
    rk_vline(fb, x, y, h, c);
    rk_vline(fb, x + w - 1, y, h, c);
}

void rk_fill_round(rk_fb_t *fb, int x, int y, int w, int h, int r, rk_color_t c)
{
    int j, inset, d;
    if (r * 2 > w) { r = w / 2; }
    if (r * 2 > h) { r = h / 2; }
    if (r < 0)     { r = 0; }

    for (j = 0; j < h; j++) {
        inset = 0;
        if (j < r) {
            d = r - j - 1;                 /* distancia vertical al centro */
            inset = r - isqrt_i(r * r - d * d);
        } else if (j >= h - r) {
            d = r - (h - j);
            inset = r - isqrt_i(r * r - d * d);
        }
        rk_hline(fb, x + inset, y + j, w - inset * 2, c);
    }
}

void rk_disc(rk_fb_t *fb, int cx, int cy, int r, rk_color_t c)
{
    int dy, dx;
    for (dy = -r; dy <= r; dy++) {
        dx = isqrt_i(r * r - dy * dy);
        rk_hline(fb, cx - dx, cy + dy, dx * 2 + 1, c);
    }
}

rk_color_t rk_mix(rk_color_t a, rk_color_t b, uint8_t t)
{
    int ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
    int br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
    int r  = ar + (br - ar) * t / 255;
    int g  = ag + (bg - ag) * t / 255;
    int bl = ab + (bb - ab) * t / 255;
    return (rk_color_t)((r << 11) | (g << 5) | bl);
}

rk_color_t rk_dim(rk_color_t c, uint8_t amount)
{
    return rk_mix(c, 0x0000, amount);
}

void rk_vgradient(rk_fb_t *fb, int x, int y, int w, int h,
                  rk_color_t top, rk_color_t bottom)
{
    int j;
    if (h <= 0) {
        return;
    }
    for (j = 0; j < h; j++) {
        uint8_t t = (h == 1) ? 0 : (uint8_t)(j * 255 / (h - 1));
        rk_hline(fb, x, y + j, w, rk_mix(top, bottom, t));
    }
}

void rk_blit(rk_fb_t *fb, const rk_sprite_t *s, int x, int y)
{
    int i, j;
    for (j = 0; j < s->h; j++) {
        for (i = 0; i < s->w; i++) {
            uint8_t v = s->idx[j * s->w + i];
            if (v != 0) {
                rk_px(fb, x + i, y + j, s->pal[v]);
            }
        }
    }
}

void rk_blit_tint(rk_fb_t *fb, const rk_sprite_t *s, int x, int y,
                  rk_color_t tint, uint8_t amount)
{
    int i, j;
    if (amount == 0) {
        rk_blit(fb, s, x, y);
        return;
    }
    for (j = 0; j < s->h; j++) {
        for (i = 0; i < s->w; i++) {
            uint8_t v = s->idx[j * s->w + i];
            if (v != 0) {
                rk_px(fb, x + i, y + j, rk_mix(s->pal[v], tint, amount));
            }
        }
    }
}

void rk_blit_solid(rk_fb_t *fb, const rk_sprite_t *s, int x, int y, rk_color_t c)
{
    int i, j;
    for (j = 0; j < s->h; j++) {
        for (i = 0; i < s->w; i++) {
            if (s->idx[j * s->w + i] != 0) {
                rk_px(fb, x + i, y + j, c);
            }
        }
    }
}

static const int8_t SIN64[64] = {
       0,   12,   25,   37,   49,   60,   71,   81,
      90,   98,  106,  112,  117,  122,  125,  126,
     127,  126,  125,  122,  117,  112,  106,   98,
      90,   81,   71,   60,   49,   37,   25,   12,
       0,  -12,  -25,  -37,  -49,  -60,  -71,  -81,
     -90,  -98, -106, -112, -117, -122, -125, -126,
    -127, -126, -125, -122, -117, -112, -106,  -98,
     -90,  -81,  -71,  -60,  -49,  -37,  -25,  -12,
};

int rk_sin8(uint8_t phase)
{
    return SIN64[phase >> 2];
}

uint16_t rk_hash(uint16_t v)
{
    v ^= (uint16_t)(v << 7);
    v ^= (uint16_t)(v >> 9);
    v ^= (uint16_t)(v << 8);
    return v;
}
