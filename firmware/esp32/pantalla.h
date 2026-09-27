/* pantalla.h — el panel físico.
 *
 * El núcleo dibuja en un framebuffer RGB565 cuadrado del lado corto del
 * panel. En el de 128x128 (el del producto) es la pantalla entera: 32 KB. En
 * uno que no es cuadrado, como el de 2,2" (240x320), las franjas que sobran
 * se pintan lisas del color de fondo: la cara y el QR se escalan con el lado
 * corto de todas formas.
 *
 * Si la memoria no alcanza para el cuadrado entero —en el ESP32 clásico el
 * bloque contiguo más grande ronda los 110 KB, y 240x240 pide 112,5— se usa
 * el cuadrado más grande que entre, centrado, con franjas a los cuatro
 * lados. La cara se ve un poco más chica, pero se ve.
 *
 * Sólo se mandan por SPI las filas que cambiaron respecto del cuadro
 * anterior (se comparan por hash, no guardando una copia). Una cara que
 * parpadea cambia unas pocas decenas de filas, no la pantalla entera.
 */
#ifndef ROOTKIT_ESP32_PANTALLA_H
#define ROOTKIT_ESP32_PANTALLA_H

#include <Arduino.h>
extern "C" {
#include "../gfx/fb.h"
}

bool     pantalla_iniciar(uint8_t brillo_inicial);
rk_fb_t *pantalla_fb(void);

/* Manda lo que cambió. `fondo` pinta las franjas si el panel no es cuadrado. */
void     pantalla_presentar(rk_color_t fondo);

/* Fuerza a mandar el cuadro entero la próxima vez (tras despertar). */
void     pantalla_invalidar(void);

/* El cuadrado de la cara: su lado y dónde empieza en el panel. */
int      pantalla_lado(void);
int      pantalla_x0(void);
int      pantalla_y0(void);

/* Deja las franjas de arriba y de abajo para quien llama: pantalla_presentar
 * deja de pintarlas y el que las reservó escribe en ellas con
 * pantalla_rect. Es lo que usa el firmware de banco para las lecturas. */
void     pantalla_reservar_franjas(bool reservar);

/* Manda un rectángulo de pixeles RGB565 a esa posición del panel. */
void     pantalla_rect(int x, int y, int w, int h, const rk_color_t *px);

/* 0 apaga la luz y duerme el controlador; 1-100 enciende. */
void     pantalla_brillo(uint8_t pct);
uint8_t  pantalla_brillo_actual(void);

#endif
