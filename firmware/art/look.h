/* look.h — cómo se ve cada estado de ánimo. El carácter del simbionte.
 *
 * Esta tabla es la razón de que el Prime y el Mini muestren el MISMO bicho.
 * El adulto de 96x72 y el brote de 32x32 tienen arte distinto, pero la
 * decisión de qué ojo, qué boca, qué tinte y cuánto respira para cada ánimo
 * se toma una sola vez, acá. Si viviera duplicada en los dos rigs, el día
 * que alguien tocara uno el Mini y el Prime empezarían a decir cosas
 * distintas sobre la misma planta, que es exactamente el bug que el usuario
 * no puede diagnosticar.
 *
 * Agregar un ánimo es agregar una fila. Agregar un simbionte no toca nada de
 * esto: la paleta cambia, el carácter no.
 */
#ifndef ROOTKIT_LOOK_H
#define ROOTKIT_LOOK_H

#include "../gfx/fb.h"
#include "../core/mood.h"
#include "sprites.h"

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

/* Las dos tablas de arte. Mismo orden que los enums, y hay un test que
 * verifica que estén completas y sin huecos en las dos escalas. */
extern const rk_shape_t *const RK_AD_OJO[RK_OJO_COUNT];
extern const rk_shape_t *const RK_AD_BOCA[RK_BOCA_COUNT];
extern const rk_shape_t *const RK_BR_OJO[RK_OJO_COUNT];
extern const rk_shape_t *const RK_BR_BOCA[RK_BOCA_COUNT];

#endif /* ROOTKIT_LOOK_H */
