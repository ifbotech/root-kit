/* fb.h — framebuffer RGB565 y primitivas de dibujo.
 *
 * Por qué un renderer propio y no LVGL:
 *
 *   1. El pixel art necesita escalado por enteros con vecino más cercano.
 *      LVGL escala pensando en suavizado, que es justo lo que no queremos.
 *   2. Un buffer RGB565 plano es exactamente lo que espera
 *      esp_lcd_panel_draw_bitmap(). El mismo código que corre en el
 *      simulador corre en el Prime y en cada Mini sin capa intermedia.
 *   3. El sistema tiene DOS paneles de tamaños distintos —240x320 y
 *      128x128— y LVGL pesa lo mismo en los dos. Acá el Mini paga
 *      exactamente las primitivas que usa.
 *
 * Ninguna función de este archivo sabe de qué panel se trata: todas
 * trabajan sobre fb->w y fb->h. Los tamaños concretos viven en panel.h,
 * y por eso el mismo código de dibujo compone las dos pantallas.
 */
#ifndef ROOTKIT_FB_H
#define ROOTKIT_FB_H

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t rk_color_t;   /* RGB565, el formato nativo del panel */

/* Componentes de 8 bits a RGB565. */
#define RK_RGB(r, g, b)                      \
    ((rk_color_t)((((uint16_t)(r) & 0xF8) << 8) | \
                  (((uint16_t)(g) & 0xFC) << 3) | \
                  (((uint16_t)(b) & 0xF8) >> 3)))

typedef struct {
    rk_color_t *px;
    int         w;
    int         h;
} rk_fb_t;

/* Bitmap indexado. El índice 0 es siempre transparente, de modo que el
 * mismo formato sirve para el cuerpo del simbionte y para los overlays. */
typedef struct {
    uint8_t           w;
    uint8_t           h;
    const uint8_t    *idx;   /* w*h índices de paleta                  */
    const rk_color_t *pal;   /* paleta; pal[0] no se usa nunca         */
    uint8_t           ncol;
} rk_sprite_t;

void rk_fb_init(rk_fb_t *fb, rk_color_t *px, int w, int h);
void rk_fb_clear(rk_fb_t *fb, rk_color_t c);

void rk_px(rk_fb_t *fb, int x, int y, rk_color_t c);
void rk_hline(rk_fb_t *fb, int x, int y, int w, rk_color_t c);
void rk_vline(rk_fb_t *fb, int x, int y, int h, rk_color_t c);
void rk_fill_rect(rk_fb_t *fb, int x, int y, int w, int h, rk_color_t c);
void rk_rect(rk_fb_t *fb, int x, int y, int w, int h, rk_color_t c);
void rk_fill_round(rk_fb_t *fb, int x, int y, int w, int h, int r, rk_color_t c);
void rk_vgradient(rk_fb_t *fb, int x, int y, int w, int h,
                  rk_color_t top, rk_color_t bottom);
void rk_disc(rk_fb_t *fb, int cx, int cy, int r, rk_color_t c);

/* Mezcla dos colores; t va de 0 (a) a 255 (b). */
rk_color_t rk_mix(rk_color_t a, rk_color_t b, uint8_t t);
/* Escurece un color: amount 0 deja igual, 255 lo lleva a negro. */
rk_color_t rk_dim(rk_color_t c, uint8_t amount);

/* Seno entero: fase 0..255 recorre una vuelta, devuelve -127..127.
 * Toda la animación del simbionte (respiración, tiritar, burbujas) sale de
 * acá, y así el firmware no arrastra libm. */
int rk_sin8(uint8_t phase);
/* Ruido determinista para partículas: misma entrada, misma salida siempre. */
uint16_t rk_hash(uint16_t v);

void rk_blit(rk_fb_t *fb, const rk_sprite_t *s, int x, int y);
/* Igual que rk_blit pero tiñendo el sprite hacia `tint`. Se usa para que el
 * simbionte se ponga azulado de frío o rojizo de calor sin duplicar arte. */
void rk_blit_tint(rk_fb_t *fb, const rk_sprite_t *s, int x, int y,
                  rk_color_t tint, uint8_t amount);
/* Silueta sólida: útil para sombras y para el parpadeo de alerta. */
void rk_blit_solid(rk_fb_t *fb, const rk_sprite_t *s, int x, int y, rk_color_t c);

/* Blit escalado por enteros, vecino más cercano. Es la única forma en que
 * el arte llega a pantalla: el adulto va a 2x en el Prime y el brote a 2x en
 * el Mini, y en los dos casos un pixel de arte son cuatro de panel, exactos,
 * sin interpolación. Con scale <= 1 delega en rk_blit. */
void rk_blit_scaled(rk_fb_t *fb, const rk_sprite_t *s, int x, int y, int scale);
/* Igual, pero tiñendo hacia `tint`. Es el que usa el rig del simbionte para
 * ponerse azulado de frío o rojizo de calor sin duplicar arte. */
void rk_blit_scaled_tint(rk_fb_t *fb, const rk_sprite_t *s, int x, int y,
                         int scale, rk_color_t tint, uint8_t amount);

#endif /* ROOTKIT_FB_H */
