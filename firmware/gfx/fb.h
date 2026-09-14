/* fb.h — framebuffer RGB565 y primitivas de dibujo.
 *
 * Por qué un renderer propio y no LVGL:
 *
 *   1. El AXS15231B de la Guition no soporta refresco parcial, así que hay
 *      que reenviar el cuadro entero siempre. La principal optimización de
 *      LVGL — redibujar sólo los rectángulos sucios — no sirve de nada acá.
 *   2. Un buffer RGB565 plano es exactamente lo que espera
 *      esp_lcd_panel_draw_bitmap(). El mismo código que corre en el
 *      simulador corre en la Terminal sin capa intermedia.
 *   3. El pixel art necesita escalado por enteros con vecino más cercano.
 *      LVGL escala pensando en suavizado, que es justo lo que no queremos.
 *
 * Trabajamos en un lienzo lógico de 160x240 y lo presentamos a 320x480:
 * exactamente 2x, así que cada pixel del arte son cuatro de pantalla, sin
 * interpolación. En el simulador el escalado lo hace SDL; en la Terminal
 * se hace al vuelo mientras se empuja el cuadro por QSPI.
 */
#ifndef ROOTKIT_FB_H
#define ROOTKIT_FB_H

#include <stdint.h>
#include <stdbool.h>

#define RK_CANVAS_W  160
#define RK_CANVAS_H  240
#define RK_SCALE       2   /* 160x240 -> 320x480 */

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

#endif /* ROOTKIT_FB_H */
