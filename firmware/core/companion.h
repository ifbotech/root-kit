/* companion.h — la colección: simbiontes, rarezas y el vínculo que crece.
 *
 * Tres ideas sostienen este módulo.
 *
 * 1. LA RAREZA ES MÉRITO, NO SUERTE.
 *    Sale de la dificultad hortícola de la especie, no de un dado. Un potus
 *    da un simbionte común porque el potus perdona todo; un bonsái da uno
 *    legendario porque mantenerlo vivo es trabajo de verdad. Así la rareza
 *    significa algo cuando la mostrás, y de paso el producto queda afuera
 *    del terreno de las cajas de botín, que Bélgica y Países Bajos ya
 *    restringen y la UE mira de cerca.
 *
 *    La ceremonia de apertura se conserva entera: el cofre, los destellos
 *    graduados por rareza, la revelación. Lo que no hay es azar en el
 *    resultado. Es mejor diseño de juego además de más prolijo legalmente:
 *    el jugador que quiere un legendario sabe exactamente qué hacer.
 *
 * 2. EL SIMBIONTE CRECE CON LA PLANTA, NO CON EL RELOJ.
 *    Las etapas avanzan con días SANOS acumulados, no con días transcurridos.
 *    Una planta abandonada tiene un simbionte que no evoluciona, y esa es
 *    toda la mecánica de vínculo: el tiempo solo no alcanza.
 *
 * 3. UNA ESPECIE, UN SIMBIONTE.
 *    El mapeo es total y biyectivo, y hay un test que lo verifica. Registrar
 *    una especie nueva siempre revela exactamente un habitante nuevo.
 */
#ifndef ROOTKIT_COMPANION_H
#define ROOTKIT_COMPANION_H

#include <stdint.h>
#include <stdbool.h>
#include "species.h"

typedef enum {
    RK_RAR_COMUN = 0,
    RK_RAR_RARO,
    RK_RAR_EPICO,
    RK_RAR_LEGENDARIO,
    RK_RAR_COUNT
} rk_rarity_t;

/* Etapas de crecimiento. En el Prime el adulto cambia de porte y de aura;
 * en el Mini el brote va sacando hojas alrededor. */
typedef enum {
    RK_ETAPA_ESPORA = 0,   /*   0 días sanos */
    RK_ETAPA_BROTE,        /*   7 */
    RK_ETAPA_JOVEN,        /*  30 */
    RK_ETAPA_MADURO,       /*  90 */
    RK_ETAPA_ANCESTRAL,    /* 180 */
    RK_ETAPA_COUNT
} rk_stage_t;

typedef struct {
    const char *id;
    const char *nombre;        /* "Tuga.exe"                              */
    const char *especie;       /* id de la especie que lo revela          */
    const char *lema;          /* una línea de personalidad               */
} rk_companion_t;

/* El vínculo con una planta. Vive en NVS, uno por planta registrada. */
typedef struct {
    uint16_t dias_vividos;     /* desde el alta                            */
    uint16_t dias_sanos;       /* los que terminaron bien: mueven la etapa */
    uint16_t racha;            /* días sanos consecutivos                  */
    uint16_t mejor_racha;
} rk_bond_t;

extern const rk_companion_t rk_companion_table[];
extern const int            rk_companion_count;

const rk_companion_t *rk_companion_find(const char *id);

/* Posición en la tabla, o -1. El índice es la clave con la que el arte
 * elige paleta y cuerpo del brote: tools/gen_art.py genera las doce paletas
 * en ESTE orden, y hay un test que verifica que sigan alineadas. */
int rk_companion_index(const rk_companion_t *c);

/* El simbionte que revela una especie. Determinista y total: para cualquier
 * especie del catálogo devuelve siempre el mismo, y nunca NULL. */
const rk_companion_t *rk_companion_for_species(const char *especie);

rk_rarity_t rk_rarity_of(const rk_companion_t *c);
rk_rarity_t rk_rarity_from_difficulty(uint8_t dificultad);

const char *rk_rarity_name(rk_rarity_t r);
/* Cuántos destellos y de qué intensidad merece cada rareza en la ceremonia.
 * Vive acá y no en la animación para que la escala sea una sola. */
int         rk_rarity_sparkles(rk_rarity_t r);

/* --------------------------------------------------------------- vínculo - */
void       rk_bond_init(rk_bond_t *b);
/* Cierra un día. `sano` es true si la planta lo terminó en un estado bueno. */
void       rk_bond_dia(rk_bond_t *b, bool sano);
rk_stage_t rk_stage_from_bond(const rk_bond_t *b);
const char *rk_stage_name(rk_stage_t e);
/* Días sanos que faltan para la próxima etapa; 0 si ya está en la última. */
uint16_t   rk_stage_faltan(const rk_bond_t *b);
/* Progreso hacia la próxima etapa, de 0 a 100. */
uint8_t    rk_stage_progreso(const rk_bond_t *b);

#endif /* ROOTKIT_COMPANION_H */
