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

/* Los tres blits comparten el mismo recorte: se calcula la interseccion una
 * sola vez y despues se escribe con punteros directos. La version anterior
 * llamaba a rk_px por pixel, o sea cuatro comparaciones de limites por cada
 * uno; sobre los 6.912 pixeles del cuerpo de Tuga eso son ~27.000
 * comparaciones por cuadro que no hacian falta. */
typedef struct { int sx0, sy0, sx1, sy1; } clip_t;

static bool blit_clip(const rk_fb_t *fb, const rk_sprite_t *s,
                      int x, int y, clip_t *c)
{
    if (fb == NULL || s == NULL || s->idx == NULL || s->pal == NULL) {
        return false;
    }
    c->sx0 = (x < 0) ? -x : 0;
    c->sy0 = (y < 0) ? -y : 0;
    c->sx1 = (x + (int)s->w > fb->w) ? fb->w - x : (int)s->w;
    c->sy1 = (y + (int)s->h > fb->h) ? fb->h - y : (int)s->h;
    return (c->sx0 < c->sx1) && (c->sy0 < c->sy1);
}

void rk_blit(rk_fb_t *fb, const rk_sprite_t *s, int x, int y)
{
    clip_t c;
    int i, j;

    if (!blit_clip(fb, s, x, y, &c)) {
        return;
    }
    for (j = c.sy0; j < c.sy1; j++) {
        const uint8_t *src = &s->idx[(size_t)j * s->w + c.sx0];
        rk_color_t    *dst = &fb->px[(size_t)(y + j) * fb->w + x + c.sx0];
        for (i = c.sx0; i < c.sx1; i++) {
            uint8_t v = *src++;
            if (v != 0u) {
                *dst = s->pal[v];
            }
            dst++;
        }
    }
}

void rk_blit_tint(rk_fb_t *fb, const rk_sprite_t *s, int x, int y,
                  rk_color_t tint, uint8_t amount)
{
    clip_t c;
    rk_color_t cache[256];
    bool       hecho[256];
    int i, j;

    if (amount == 0u) {
        rk_blit(fb, s, x, y);
        return;
    }
    if (!blit_clip(fb, s, x, y, &c)) {
        return;
    }
    /* La paleta tiene a lo sumo unas pocas decenas de entradas y el sprite
     * miles de pixeles: conviene mezclar una vez por color y no por pixel. */
    memset(hecho, 0, sizeof hecho);

    for (j = c.sy0; j < c.sy1; j++) {
        const uint8_t *src = &s->idx[(size_t)j * s->w + c.sx0];
        rk_color_t    *dst = &fb->px[(size_t)(y + j) * fb->w + x + c.sx0];
        for (i = c.sx0; i < c.sx1; i++) {
            uint8_t v = *src++;
            if (v != 0u) {
                if (!hecho[v]) {
                    cache[v] = rk_mix(s->pal[v], tint, amount);
                    hecho[v] = true;
                }
                *dst = cache[v];
            }
            dst++;
        }
    }
}

void rk_blit_solid(rk_fb_t *fb, const rk_sprite_t *s, int x, int y, rk_color_t c)
{
    clip_t cl;
    int i, j;

    if (!blit_clip(fb, s, x, y, &cl)) {
        return;
    }
    for (j = cl.sy0; j < cl.sy1; j++) {
        const uint8_t *src = &s->idx[(size_t)j * s->w + cl.sx0];
        rk_color_t    *dst = &fb->px[(size_t)(y + j) * fb->w + x + cl.sx0];
        for (i = cl.sx0; i < cl.sx1; i++) {
            if (*src++ != 0u) {
                *dst = c;
            }
            dst++;
        }
    }
}

/* ---------------------------------------------------- blit escalado ----- */
/* El escalado por enteros es la única forma en que el arte llega a pantalla,
 * así que vale la pena que no sea rk_px en un doble bucle. Se calcula el
 * recorte en coordenadas del SPRITE, y después cada pixel de arte se escribe
 * como una fila de `scale` colores que se replica `scale` veces con memcpy.
 * Sobre el adulto a 2x eso son 6.912 iteraciones en vez de 27.648 llamadas. */
static bool blit_clip_scaled(const rk_fb_t *fb, const rk_sprite_t *s,
                             int x, int y, int scale, clip_t *c)
{
    if (fb == NULL || s == NULL || s->idx == NULL || s->pal == NULL ||
        scale <= 0) {
        return false;
    }
    /* División hacia arriba en los bordes negativos: el primer pixel de arte
     * visible es el que tiene al menos una columna dentro del framebuffer. */
    c->sx0 = (x < 0) ? (-x) / scale : 0;
    c->sy0 = (y < 0) ? (-y) / scale : 0;
    c->sx1 = (x + (int)s->w * scale > fb->w)
             ? (fb->w - x + scale - 1) / scale : (int)s->w;
    c->sy1 = (y + (int)s->h * scale > fb->h)
             ? (fb->h - y + scale - 1) / scale : (int)s->h;
    if (c->sx1 > (int)s->w) { c->sx1 = (int)s->w; }
    if (c->sy1 > (int)s->h) { c->sy1 = (int)s->h; }
    return (c->sx0 < c->sx1) && (c->sy0 < c->sy1);
}

/* Pinta un bloque de scale x scale recortado contra el framebuffer. */
static void bloque(rk_fb_t *fb, int px, int py, int scale, rk_color_t c)
{
    int x0 = px < 0 ? 0 : px;
    int y0 = py < 0 ? 0 : py;
    int x1 = px + scale > fb->w ? fb->w : px + scale;
    int y1 = py + scale > fb->h ? fb->h : py + scale;
    int i, j;

    for (j = y0; j < y1; j++) {
        rk_color_t *dst = &fb->px[(size_t)j * fb->w + x0];
        for (i = x0; i < x1; i++) {
            *dst++ = c;
        }
    }
}

void rk_blit_scaled(rk_fb_t *fb, const rk_sprite_t *s, int x, int y, int scale)
{
    clip_t c;
    int i, j;

    if (scale <= 1) {
        rk_blit(fb, s, x, y);
        return;
    }
    if (!blit_clip_scaled(fb, s, x, y, scale, &c)) {
        return;
    }
    for (j = c.sy0; j < c.sy1; j++) {
        const uint8_t *src = &s->idx[(size_t)j * s->w + c.sx0];
        for (i = c.sx0; i < c.sx1; i++) {
            uint8_t v = *src++;
            if (v != 0u) {
                bloque(fb, x + i * scale, y + j * scale, scale, s->pal[v]);
            }
        }
    }
}

void rk_blit_scaled_tint(rk_fb_t *fb, const rk_sprite_t *s, int x, int y,
                         int scale, rk_color_t tint, uint8_t amount)
{
    clip_t     c;
    rk_color_t cache[256];
    bool       hecho[256];
    int        i, j;

    if (amount == 0u) {
        rk_blit_scaled(fb, s, x, y, scale);
        return;
    }
    if (scale <= 1) {
        rk_blit_tint(fb, s, x, y, tint, amount);
        return;
    }
    if (!blit_clip_scaled(fb, s, x, y, scale, &c)) {
        return;
    }
    memset(hecho, 0, sizeof hecho);

    for (j = c.sy0; j < c.sy1; j++) {
        const uint8_t *src = &s->idx[(size_t)j * s->w + c.sx0];
        for (i = c.sx0; i < c.sx1; i++) {
            uint8_t v = *src++;
            if (v != 0u) {
                if (!hecho[v]) {
                    cache[v] = rk_mix(s->pal[v], tint, amount);
                    hecho[v] = true;
                }
                bloque(fb, x + i * scale, y + j * scale, scale, cache[v]);
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
