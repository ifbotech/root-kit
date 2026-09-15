/* mood.h — el corazón de ROOTKIT: telemetría cruda -> estado de ánimo.
 *
 * Este módulo es C99 puro, sin LVGL y sin ESP-IDF. Se compila igual en el
 * simulador de escritorio, en los tests y en la Terminal, y es lo único que
 * decide qué cara pone el simbionte. Todo el resto es presentación.
 */
#ifndef ROOTKIT_MOOD_H
#define ROOTKIT_MOOD_H

#include "telemetry.h"
#include "species.h"

/* Detección de noche. Expuestas acá porque los tests y la UI las necesitan:
 * la Terminal quiere saber si el simbionte duerme o si de verdad falta luz. */
#define LUX_NOCHE        15u
#define MUESTRAS_NOCHE    8    /* a una muestra cada 15 min ~ 2 h */

typedef enum {
    RK_MOOD_UNKNOWN = 0,   /* no hay planta asignada o faltan datos       */
    RK_MOOD_OFFLINE,       /* el nodo dejó de reportar                      */
    RK_MOOD_SLEEPING,      /* es de noche: no se juzga luz ni aire        */
    RK_MOOD_HAPPY,
    RK_MOOD_THIRSTY,       /* tierra seca                                 */
    RK_MOOD_DROWNING,      /* exceso de agua en la raíz                   */
    RK_MOOD_COLD,
    RK_MOOD_HOT,
    RK_MOOD_SCORCHED,      /* demasiado sol directo                       */
    RK_MOOD_DARK,          /* poca luz, de día                            */
    RK_MOOD_PARCHED_AIR,   /* humedad relativa baja                       */
    RK_MOOD_COUNT
} rk_mood_t;

typedef enum {
    RK_SEV_OK = 0,
    RK_SEV_WATCH,
    RK_SEV_URGENT
} rk_severity_t;

typedef struct {
    rk_mood_t     mood;
    rk_severity_t severity;
    const char   *reason;   /* frase corta en castellano, lista para pantalla */
} rk_verdict_t;

/* Estado que hay que conservar entre lecturas, uno por planta:
 * la histéresis evita que el simbionte titile en el borde de un umbral,
 * y el conteo de muestras oscuras distingue "es de noche" de "le falta luz". */
typedef struct {
    rk_mood_t last_mood;
    uint16_t  dark_samples;
    uint16_t  light_samples;
} rk_mood_state_t;

void rk_mood_state_init(rk_mood_state_t *st);

/* Evalúa una lectura y actualiza el estado. Nunca devuelve NULL en reason. */
rk_verdict_t rk_mood_eval(rk_mood_state_t     *st,
                          const rk_species_t  *sp,
                          const rk_telemetry_t *t);

const char *rk_mood_name(rk_mood_t m);

/* Frase canónica del ánimo, en castellano y lista para pantalla. Es la misma
 * que devuelve rk_mood_eval; existe aparte para que la UI y las capturas
 * puedan pedirla sin tener que fabricar una telemetría que la provoque. */
const char *rk_mood_reason(rk_mood_t m);

#endif /* ROOTKIT_MOOD_H */
