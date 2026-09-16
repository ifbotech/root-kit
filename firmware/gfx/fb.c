#include "fb.h"
#include <stddef.h>
#include <string.h>

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
    /* De a dos pixeles por escritura. En el escritorio es una mejora modesta;
     * en el ESP32 importa bastante mas, porque una escritura de 32 bits
     * alineada cuesta lo mismo que una de 16 y el bus se aprovecha al doble. */
    int       n = fb->w * fb->h;
    uint16_t *p = fb->px;
    uint32_t  par = ((uint32_t)c << 16) | (uint32_t)c;
    uint32_t *p32;
    int       i, pares;

    if (n <= 0) {
        return;
    }
    /* Alinear a 4 bytes si el buffer arranca impar. */
    if (((uintptr_t)p & 3u) != 0u) {
        *p++ = c;
        n--;
    }
    p32   = (uint32_t *)(void *)p;
    pares = n / 2;
    for (i = 0; i < pares; i++) {
        p32[i] = par;
    }
    if (n & 1) {
        p[n - 1] = c;
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

void rk_elipse(rk_fb_t *fb, int cx, int cy, int rx, int ry, rk_color_t c)
{
    int dy, dx;

    if (fb == NULL || rx <= 0 || ry <= 0) {
        return;
    }
    for (dy = -ry; dy <= ry; dy++) {
        /* x = rx * sqrt(1 - (dy/ry)^2), en enteros y sin libm. Se escala por
         * 1024 antes de la raiz para no perder resolucion en el redondeo. */
        int t = 1024 - (dy * dy * 1024) / (ry * ry);
        if (t < 0) {
            continue;
        }
        dx = rx * isqrt_i(t * 16) / 128;
        rk_hline(fb, cx - dx, cy + dy, dx * 2 + 1, c);
    }
}

void rk_elipse_ring(rk_fb_t *fb, int cx, int cy, int rx, int ry,
                    int grosor, rk_color_t c)
{
    int dy, irx, iry;

    if (fb == NULL || rx <= 0 || ry <= 0) {
        return;
    }
    if (grosor < 1) {
        grosor = 1;
    }
    irx = rx - grosor;
    iry = ry - grosor;

    /* Se dibuja SOLO el anillo, fila por fila, sin tocar el interior.
     *
     * La version anterior rellenaba la elipse entera y despues vaciaba el
     * centro pintandolo de negro. Eso no es un anillo: es un disco negro con
     * borde, y se comia todo lo que ya estuviera dibujado adentro. En la
     * primera lamina de caras los seis modelos salieron con los ojos tapados
     * por un ovalo negro, y el error estaba aca. */
    for (dy = -ry; dy <= ry; dy++) {
        int t = 1024 - (dy * dy * 1024) / (ry * ry);
        int ox, ix;
        if (t < 0) {
            continue;
        }
        ox = rx * isqrt_i(t * 16) / 128;

        if (irx > 0 && iry > 0 && dy > -iry && dy < iry) {
            int ti = 1024 - (dy * dy * 1024) / (iry * iry);
            ix = (ti > 0) ? irx * isqrt_i(ti * 16) / 128 : 0;
            if (ix < ox) {
                rk_hline(fb, cx - ox, cy + dy, ox - ix, c);
                rk_hline(fb, cx + ix + 1, cy + dy, ox - ix, c);
            }
        } else {
            rk_hline(fb, cx - ox, cy + dy, ox * 2 + 1, c);
        }
    }
}

void rk_arco(rk_fb_t *fb, int cx, int cy, int rx, int ry,
             bool arriba, int grosor, rk_color_t c)
{
    int x;

    if (fb == NULL || rx <= 0 || ry <= 0) {
        return;
    }
    if (grosor < 1) {
        grosor = 1;
    }
    for (x = -rx; x <= rx; x++) {
        int t = 1024 - (x * x * 1024) / (rx * rx);
        int dy;
        if (t < 0) {
            continue;
        }
        dy = ry * isqrt_i(t * 16) / 128;
        if (arriba) {
            rk_vline(fb, cx + x, cy - dy, grosor, c);
        } else {
            rk_vline(fb, cx + x, cy + dy - grosor + 1, grosor, c);
        }
    }
}

void rk_linea(rk_fb_t *fb, int x0, int y0, int x1, int y1,
              int grosor, rk_color_t c)
{
    int dx = (x1 > x0) ? x1 - x0 : x0 - x1;
    int dy = (y1 > y0) ? y1 - y0 : y0 - y1;
    int pasos = (dx > dy ? dx : dy);
    int i, r;

    if (fb == NULL) {
        return;
    }
    if (grosor < 1) {
        grosor = 1;
    }
    r = grosor / 2;
    if (pasos == 0) {
        rk_fill_rect(fb, x0 - r, y0 - r, grosor, grosor, c);
        return;
    }
    for (i = 0; i <= pasos; i++) {
        int px = x0 + (x1 - x0) * i / pasos;
        int py = y0 + (y1 - y0) * i / pasos;
        /* Un disco por paso: con un cuadrado las cejas inclinadas quedan
         * escalonadas y a tamano de cara el escalon se ve. */
        if (grosor <= 2) {
            rk_fill_rect(fb, px - r, py - r, grosor, grosor, c);
        } else {
            rk_disc(fb, px, py, r, c);
        }
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
