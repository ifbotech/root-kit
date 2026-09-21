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

/* --------------------------------------------------------- rellenos --- */
/* Un relleno puede ser un color plano o un DEGRADADO, y esa es la diferencia
 * entre una forma y un dibujo: un iris con un degradado de ámbar a ocre se
 * lee como un ojo húmedo, y el mismo iris en color plano se lee como un
 * círculo. Cuesta una multiplicación por pixel.
 *
 *   RK_RELLENO_PLANO    un color, como siempre
 *   RK_RELLENO_LINEAL   de `a` en (x0,y0) a `b` en (x1,y1)
 *   RK_RELLENO_RADIAL   `a` en el centro y `b` a `r` de distancia
 */
typedef enum {
    RK_RELLENO_PLANO = 0,
    RK_RELLENO_LINEAL,
    RK_RELLENO_RADIAL
} rk_relleno_tipo_t;

typedef struct {
    rk_relleno_tipo_t tipo;
    rk_color_t a, b;
    int32_t x0, y0, x1, y1;   /* Q4: el eje del lineal, o el centro y el radio */
} rk_relleno_t;

static inline rk_relleno_t rk_plano(rk_color_t c)
{
    rk_relleno_t r;
    r.tipo = RK_RELLENO_PLANO; r.a = c; r.b = c;
    r.x0 = 0; r.y0 = 0; r.x1 = 0; r.y1 = 0;
    return r;
}

static inline rk_relleno_t rk_lineal(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                                     rk_color_t a, rk_color_t b)
{
    rk_relleno_t r;
    r.tipo = RK_RELLENO_LINEAL; r.a = a; r.b = b;
    r.x0 = x0; r.y0 = y0; r.x1 = x1; r.y1 = y1;
    return r;
}

static inline rk_relleno_t rk_radial(int32_t cx, int32_t cy, int32_t rad,
                                     rk_color_t centro, rk_color_t borde)
{
    rk_relleno_t r;
    r.tipo = RK_RELLENO_RADIAL; r.a = centro; r.b = borde;
    r.x0 = cx; r.y0 = cy; r.x1 = rad; r.y1 = 0;
    return r;
}

/* Igual que rk_aa_pintar, con un relleno en vez de un color. */
void rk_aa_pintar_relleno(rk_fb_t *fb, const rk_forma_t *formas, int n,
                          const rk_relleno_t *relleno, uint8_t alfa,
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
