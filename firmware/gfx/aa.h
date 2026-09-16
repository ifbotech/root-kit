/* aa.h — formas con antialiasing, en punto fijo.
 *
 * POR QUÉ HACE FALTA
 *
 * El estilo de los personajes pasó de pixel art a ilustración plana del tipo
 * Duolingo: formas redondas, grandes y limpias, sin contorno negro. Ese estilo
 * vive o muere en el borde de las curvas. Un círculo sin antialiasing en una
 * pantalla de 128x128 se ve escalonado, y el escalón es exactamente lo que
 * distingue "pixel art" de "ilustración". No hay forma de hacer este estilo
 * sin suavizar bordes.
 *
 * CÓMO FUNCIONA
 *
 * Cada forma sabe calcular, para un punto, a qué distancia está de su borde
 * (distancia con signo: negativa adentro). De esa distancia sale cuánto del
 * pixel queda cubierto, y el color se mezcla con el fondo en esa proporción.
 * Un pixel que el borde corta por la mitad queda mitad color, mitad fondo, y
 * a la distancia de lectura eso se percibe como una curva lisa.
 *
 * Las formas se combinan por INTERSECCIÓN (la cobertura es el mínimo de las
 * coberturas). Con eso alcanza para todo lo que la cara necesita: un párpado
 * es "el ojo, menos lo que queda arriba de una recta", una lágrima es "un
 * círculo más un triángulo", un triángulo es la intersección de tres
 * semiplanos.
 *
 * POR QUÉ PUNTO FIJO
 *
 * El ESP32-C3 es RISC-V sin unidad de punto flotante: cada operación con
 * `float` se emula en software y es un orden de magnitud más lenta. Todo
 * acá trabaja con enteros. Las posiciones van en Q4 —un pixel son 16
 * unidades— y eso permite ubicar formas entre pixeles, que es lo que hace
 * que una pupila que se mueve medio pixel se vea deslizar y no saltar.
 *
 * La raíz cuadrada, que es lo caro, sólo se calcula en la franja de un pixel
 * alrededor del borde. Adentro y afuera de esa franja la cobertura se decide
 * comparando cuadrados, sin raíz.
 */
#ifndef ROOTKIT_AA_H
#define ROOTKIT_AA_H

#include <stdint.h>
#include <stdbool.h>
#include "fb.h"

/* Un pixel son 16 unidades Q4. */
#define RK_Q4(px)      ((int32_t)(px) * 16)
/* Centro geométrico de un pixel, en Q4. */
#define RK_Q4C(px)     ((int32_t)(px) * 16 + 8)

typedef enum {
    RK_FORMA_ELIPSE = 0,    /* centro (cx,cy), radios (rx,ry)                 */
    RK_FORMA_SEMIPLANO,     /* pasa por (cx,cy), normal (nx,ny) hacia AFUERA  */
    RK_FORMA_CAPSULA,       /* segmento (cx,cy)-(x1,y1) con radio rx          */
    RK_FORMA_ANILLO,        /* centro (cx,cy), radio rx, grosor ry            */
    RK_FORMA_COUNT
} rk_forma_tipo_t;

typedef struct {
    uint8_t tipo;           /* rk_forma_tipo_t                                */
    bool    invertir;       /* cubre lo que la forma NO cubre                 */
    int32_t cx, cy;         /* Q4                                             */
    int32_t rx, ry;         /* Q4                                             */
    int32_t x1, y1;         /* Q4, sólo cápsula                               */
    int32_t nx, ny;         /* Q16 unitaria, sólo semiplano                   */
} rk_forma_t;

/* Constructores. Todo en Q4 salvo el ángulo. */
rk_forma_t rk_elipse_q4(int32_t cx, int32_t cy, int32_t rx, int32_t ry);
rk_forma_t rk_circulo_q4(int32_t cx, int32_t cy, int32_t r);
rk_forma_t rk_capsula_q4(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                         int32_t r);
rk_forma_t rk_anillo_q4(int32_t cx, int32_t cy, int32_t r, int32_t grosor);
/* Semiplano que CUBRE el lado "de arriba" de una recta que pasa por (cx,cy)
 * inclinada `ang` unidades de rk_sin8 (256 = vuelta entera; positivo baja
 * hacia la derecha, porque en pantalla la y crece hacia abajo). Con ang = 0
 * es "todo lo que está por encima de esta altura", que es un párpado. */
rk_forma_t rk_semiplano_arriba_q4(int32_t cx, int32_t cy, int ang);
/* Igual, cubriendo el lado de abajo. */
rk_forma_t rk_semiplano_abajo_q4(int32_t cx, int32_t cy, int ang);
/* Semiplano definido por una arista (a -> b): cubre el lado donde queda el
 * punto de referencia (ix, iy). Es la forma robusta de armar triángulos y
 * rombos: en vez de razonar qué lado cubre cada ángulo, se le dice "el
 * interior está de este lado" y listo. */
rk_forma_t rk_semiplano_arista_q4(int32_t ax, int32_t ay, int32_t bx, int32_t by,
                                  int32_t ix, int32_t iy);
/* Devuelve la misma forma invertida: cubre lo que antes no cubría. */
rk_forma_t rk_forma_invertida(rk_forma_t f);

/* Cobertura de una forma sobre el pixel cuyo centro es (px,py) en Q4.
 * 0 = afuera, 255 = adentro, intermedio en la franja del borde. */
uint8_t rk_forma_cobertura(const rk_forma_t *f, int32_t px, int32_t py);

/* Rectángulo en pixeles que contiene la forma, ya expandido un pixel por el
 * antialiasing. Devuelve false si la forma no tiene límite (semiplano, o una
 * forma invertida): esas no acotan nada y sólo sirven intersectadas. */
bool rk_forma_caja(const rk_forma_t *f, int *x0, int *y0, int *x1, int *y1);

/* Pinta `color` sobre la INTERSECCIÓN de `n` formas, recortado al
 * rectángulo [clip_x0, clip_x1) x [clip_y0, clip_y1) y a la caja de las
 * formas acotadas. `alfa` escala la opacidad total (255 = opaco), para
 * rubores y resplandores.
 *
 * Al menos una forma tiene que ser acotada o el recorte tiene que ser
 * explícito; si no, se pinta el framebuffer entero, que casi nunca es lo
 * que se quiere pero nunca escribe fuera de él. */
void rk_aa_pintar(rk_fb_t *fb, const rk_forma_t *formas, int n,
                  rk_color_t color, uint8_t alfa,
                  int clip_x0, int clip_y0, int clip_x1, int clip_y1);

/* Atajos para los casos de una sola forma, sin recorte extra. */
void rk_aa_elipse(rk_fb_t *fb, int32_t cx, int32_t cy, int32_t rx, int32_t ry,
                  rk_color_t c);
void rk_aa_circulo(rk_fb_t *fb, int32_t cx, int32_t cy, int32_t r,
                   rk_color_t c);
void rk_aa_capsula(rk_fb_t *fb, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                   int32_t r, rk_color_t c);

/* Triángulo relleno. El orden de los vértices no importa. */
void rk_aa_triangulo(rk_fb_t *fb, int32_t ax, int32_t ay, int32_t bx, int32_t by,
                     int32_t cx, int32_t cy, rk_color_t c, uint8_t alfa);
/* Rombo centrado en (cx,cy) con semiejes (hw, hh). Dos rombos cruzados son
 * un destello de cuatro puntas. */
void rk_aa_rombo(rk_fb_t *fb, int32_t cx, int32_t cy, int32_t hw, int32_t hh,
                 rk_color_t c, uint8_t alfa);

/* Arco grueso con puntas redondeadas: la mitad inferior (abajo = true, una
 * sonrisa o un ojo cerrado contento) o la superior de un anillo. Todo en Q4. */
void rk_aa_arco(rk_fb_t *fb, int32_t cx, int32_t cy, int32_t r, int32_t grosor,
                bool abajo, rk_color_t c);

/* Raíz cuadrada entera de 64 bits. Expuesta porque la usan los tests y el
 * rig de caras, y porque es la única operación cara de todo el módulo. */
uint32_t rk_isqrt64(uint64_t v);

#endif /* ROOTKIT_AA_H */
