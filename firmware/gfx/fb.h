/* fb.h — framebuffer RGB565 y primitivas de dibujo.
 *
 * Por qué un renderer propio y no LVGL:
 *
 *   1. La cara es procedural: elipses, arcos y trazos calculados, no sprites.
 *      LVGL trae un motor de widgets que acá no se usaría nunca.
 *   2. Un buffer RGB565 plano es exactamente lo que espera el panel. El
 *      mismo código corre en el simulador, en las placas y, compilado a
 *      WebAssembly, en la app, sin capa intermedia.
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

/* Primitivas de elipse y arco. El rig de caras es procedural y no de sprites
 * —ver art/face.h— asi que todo lo que dibuja un ojo, una ceja o una boca
 * sale de estas cuatro funciones. A tamano de cara un arco calculado se ve
 * intencional donde un sprite escalado se ve blando. */
void rk_elipse(rk_fb_t *fb, int cx, int cy, int rx, int ry, rk_color_t c);
void rk_elipse_ring(rk_fb_t *fb, int cx, int cy, int rx, int ry,
                    int grosor, rk_color_t c);
/* Media elipse, de `grosor` pixeles. `arriba` elige que mitad. Es lo que
 * dibuja un ojo feliz, un parpado y una sonrisa. */
void rk_arco(rk_fb_t *fb, int cx, int cy, int rx, int ry,
             bool arriba, int grosor, rk_color_t c);
/* Segmento de grosor arbitrario. Las cejas son esto y nada mas. */
void rk_linea(rk_fb_t *fb, int x0, int y0, int x1, int y1,
              int grosor, rk_color_t c);

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


#endif /* ROOTKIT_FB_H */
