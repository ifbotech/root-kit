/* panel.h — las dos pantallas del ROOTKIT, como datos.
 *
 * El mismo motor gráfico dibuja en las dos. Todo lo que cambia entre ellas
 * vive acá; las funciones de dibujo trabajan sobre `fb->w` y `fb->h`.
 *
 *   PRIME   TFT 2,2" ILI9341, 240x320, SPI — el ROOTKIT de maceta mediana
 *           área activa 36,5 x 47,5 mm  ->  paso 0,152 mm
 *
 *   MINI    TFT 1,44" ST7735 IPS, 128x128, SPI — el ROOTKIT mini
 *           área activa 25,9 x 25,9 mm  ->  paso 0,202 mm
 *
 * Los nombres PRIME y MINI quedaron de la arquitectura anterior; hoy son sólo
 * los dos tamaños del mismo aparato, con el mismo firmware.
 *
 * La cara se escala con el lado corto del panel, así que no necesita escalas
 * enteras: en la placa de 240x320 se dibuja un cuadrado de 240x240 y las
 * franjas se pintan lisas (ver esp32/pantalla.h). El texto sólo aparece en la
 * pantalla del QR, a escala 2 en el panel chico y 3 en el grande.
 */
#ifndef ROOTKIT_PANEL_H
#define ROOTKIT_PANEL_H

#include <stdint.h>

/* --------------------------------------------------------------- Prime -- */
#define RK_PRIME_W        240
#define RK_PRIME_H        320
#define RK_PRIME_PITCH_UM 152      /* micrómetros por pixel */
#define RK_PRIME_ART      2        /* escala entera del arte  */
#define RK_PRIME_TEXT     2        /* escala base del texto   */

/* ---------------------------------------------------------------- Mini -- */
#define RK_MINI_W         128
#define RK_MINI_H         128
#define RK_MINI_PITCH_UM  202
#define RK_MINI_ART       2
#define RK_MINI_TEXT      1

/* Cuántos pixeles del framebuffer ocupa cada panel. Sirve para dimensionar
 * buffers estáticos sin repetir la multiplicación. */
#define RK_PRIME_PX  (RK_PRIME_W * RK_PRIME_H)
#define RK_MINI_PX   (RK_MINI_W * RK_MINI_H)

typedef enum {
    RK_PANEL_PRIME = 0,
    RK_PANEL_MINI,
    RK_PANEL_COUNT
} rk_panel_t;

typedef struct {
    const char *nombre;
    int         w;
    int         h;
    int         pitch_um;   /* paso de pixel en micrómetros            */
    int         art;        /* escala entera a la que se dibuja el arte */
    int         text;       /* escala base del texto                    */
} rk_panel_info_t;

extern const rk_panel_info_t rk_panel[RK_PANEL_COUNT];

/* Cuántos milímetros mide en pantalla algo de `px` pixeles, en décimas de
 * milímetro para no arrastrar punto flotante. Es la función que convierte
 * una decisión de layout en un tamaño físico verificable, y por eso hay
 * tests que la usan para fijar el tamaño de la criatura en los dos paneles. */
int rk_panel_decimas_mm(rk_panel_t p, int px);

#endif /* ROOTKIT_PANEL_H */
