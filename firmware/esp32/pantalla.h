/* pantalla.h — el panel físico.
 *
 * El núcleo dibuja en un framebuffer RGB565 cuadrado del lado corto del
 * panel. En el de 128x128 (el del producto) es la pantalla entera: 32 KB. Si
 * algún día vuelve un panel que no sea cuadrado, las franjas que sobran se
 * pintan lisas del color de fondo: la cara y el QR se escalan con el lado
 * corto de todas formas.
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

/* 0 apaga la luz y duerme el controlador; 1-100 enciende. */
void     pantalla_brillo(uint8_t pct);
uint8_t  pantalla_brillo_actual(void);

#endif
