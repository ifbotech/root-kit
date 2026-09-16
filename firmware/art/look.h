/* look.h — qué expresión pide cada estado de ánimo.
 *
 * Es la mitad semántica del rig: dice QUÉ siente el aparato, en términos
 * abstractos —ojo entrecerrado, boca hacia abajo, tiritando— sin decir cómo
 * se dibuja eso. El CÓMO lo pone la carcasa, en core/persona.c, y los cruza
 * art/face.c.
 *
 * Esa separación es lo que permite que seis modelos compartan once ánimos
 * sin escribir sesenta y seis caras a mano. Agregar un ánimo es agregar una
 * fila acá y no tocar ningún modelo; agregar un modelo es agregar una fila
 * allá y no tocar ningún ánimo.
 */
#ifndef ROOTKIT_LOOK_H
#define ROOTKIT_LOOK_H

#include "../gfx/fb.h"
#include "../core/mood.h"

typedef enum {
    RK_OJO_OPEN = 0,
    RK_OJO_BLINK,
    RK_OJO_HAPPY,
    RK_OJO_WIDE,
    RK_OJO_SLEEPY,
    RK_OJO_DEAD,
    RK_OJO_DIZZY,
    RK_OJO_GLITCH,
    RK_OJO_COUNT
} rk_ojo_t;

typedef enum {
    RK_BOCA_SMILE = 0,
    RK_BOCA_FLAT,
    RK_BOCA_FROWN,
    RK_BOCA_OPEN,
    RK_BOCA_WAVY,
    RK_BOCA_PANT,
    RK_BOCA_COUNT
} rk_boca_t;

typedef struct {
    uint8_t    ojo;         /* rk_ojo_t                                   */
    uint8_t    boca;        /* rk_boca_t                                  */
    rk_color_t tint;
    uint8_t    tint_amt;
    /* Amplitud de la respiración, en pixeles de ARTE (no de panel). Por
     * debajo de 3 el movimiento se pierde en el redondeo entero y el bicho
     * parece congelado: lo detectó el test de regresión visual, que encontró
     * dos instantes distintos produciendo exactamente el mismo cuadro. */
    uint8_t    bob_amp;
    uint8_t    bob_speed;   /* fase por segundo                            */
    uint8_t    shiver;      /* jitter horizontal (tiritar)                 */
    uint8_t    dim;         /* penumbra: el bicho también se apaga          */
    bool       blinks;
} rk_look_t;

/* Nunca devuelve NULL: un ánimo fuera de rango cae en UNKNOWN. */
const rk_look_t *rk_look(rk_mood_t mood);

/* ¿Está parpadeando en este instante? Abierto casi siempre, cerrado 120 ms
 * cada 3,4 s. El período es primo respecto del de la respiración para que no
 * se sincronicen y el bicho no quede con un tic mecánico. */
bool rk_look_parpadea(const rk_look_t *lk, uint32_t t_ms);

#endif /* ROOTKIT_LOOK_H */
