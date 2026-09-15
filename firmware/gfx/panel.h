/* panel.h — las dos pantallas del ROOTKIT, como datos.
 *
 * El sistema tiene dos paneles de tamaños muy distintos y el mismo motor
 * gráfico dibuja en los dos. Todo lo que cambia entre ellos vive acá, y
 * ninguna otra parte del código tiene constantes de tamaño escritas a mano:
 * las funciones de dibujo trabajan sobre `fb->w` y `fb->h`.
 *
 *   PRIME   TFT 2,2" ILI9341, 240x320, SPI, enchufado
 *           área activa 36,5 x 47,5 mm  ->  paso 0,152 mm
 *           el adulto a 2x mide 144 px = 21,9 mm
 *
 *   MINI    TFT 1,44" ST7735 IPS, 128x128, SPI, a batería
 *           área activa 25,9 x 25,9 mm  ->  paso 0,202 mm
 *           el brote a 2x mide 64 px = 12,9 mm
 *
 * POR QUÉ NATIVO Y NO UN LIENZO LÓGICO
 *
 * La versión anterior rasterizaba a 160x240 y escalaba 2x al presentar,
 * porque el panel de la Terminal era de 320x480 y 300 KB de framebuffer no
 * entraban en la SRAM interna. Con 240x320 el cuadro son 150 KB y con
 * 128x128 son 32 KB: los dos entran holgados, así que se dibuja directo en
 * resolución nativa y desaparece una capa entera de conversión de
 * coordenadas. El arte se sigue escalando por enteros, que es lo que de
 * verdad importaba del lienzo lógico.
 *
 * LA CONSECUENCIA TIPOGRÁFICA, QUE NO ES OBVIA
 *
 * Al dibujar nativo, un glifo de 5x7 a escala 1 mide 0,76 x 1,06 mm en el
 * Prime: ilegible. Por eso el texto del Prime arranca en escala 2 y los
 * títulos van en 3. En el Mini el paso es mayor y la distancia de lectura
 * menor, así que la escala 1 sirve para la línea de datos y la 2 para lo
 * que tiene que leerse de reojo.
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
