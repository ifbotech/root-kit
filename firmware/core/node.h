/* node.h — el kit: un Prime y sus Minis.
 *
 * Un ROOTKIT es un Prime enchufado más entre cero y cinco Minis a batería.
 * Cada nodo —el Prime incluido— tiene su propia maceta, su propia especie y
 * su propio simbionte. Este módulo es la tabla de todos ellos, y es lo que
 * el Prime dibuja y el Hub sirve.
 *
 * TRES REGLAS QUE ORDENAN EL SISTEMA
 *
 * 1. CADA NODO EVALÚA SU PROPIA MACETA.
 *    El Mini corre el mismo core/mood.c que el Prime, con los umbrales de su
 *    especie que le llegaron en la trama de configuración, y manda el ánimo
 *    ya resuelto. No es duplicación: es el mismo archivo compilado dos
 *    veces. Y es lo que permite que el Mini dibuje una cara correcta aunque
 *    el Prime esté apagado o fuera de alcance.
 *
 *    Esto invierte la regla vieja del "Spore tonto", y la razón es concreta:
 *    desde que el nodo tiene pantalla, tiene que saber qué cara poner sin
 *    preguntarle a nadie. Un nodo que necesita la red para expresarse se
 *    queda mudo justo cuando más importa.
 *
 * 2. EL PRIME ES DONDE EL SIMBIONTE ES ADULTO.
 *    El mismo bicho que en la maceta del Mini se ve como brote de 13 mm, al
 *    seleccionarlo en el Prime se ve adulto a 22 mm, con su nombre, su rareza
 *    y su vínculo. Esa es la razón de producto por la que el Prime existe:
 *    no es una pantalla más grande, es donde tus criaturas están grandes.
 *
 * 3. LA CEREMONIA PASA EN EL PRIME, SIEMPRE.
 *    Registrar una planta —la del Prime o la de cualquier Mini— abre la
 *    cápsula en el Prime. El Mini recién entonces recibe a quién le tocó.
 */
#ifndef ROOTKIT_NODE_H
#define ROOTKIT_NODE_H

#include <stdint.h>
#include <stdbool.h>
#include "mood.h"
#include "species.h"
#include "companion.h"

/* Un Prime y hasta cinco Minis. El tope no es arbitrario: son los nodos que
 * entran en la tira selectora del Prime sin que las fichas bajen de 38 px,
 * que es el ancho mínimo para un toque cómodo en un panel de 36,5 mm. */
#define RK_MAX_NODES 6

typedef enum {
    RK_ROLE_PRIME = 0,
    RK_ROLE_MINI,
    RK_ROLE_COUNT
} rk_role_t;

/* Salud del enlace, derivada de la antigüedad de la última trama. Es
 * distinto del ánimo: un nodo puede estar callado y la planta perfecta. */
typedef enum {
    RK_LINK_NUNCA = 0,   /* nunca habló: recién dado de alta        */
    RK_LINK_VIVO,        /* dentro del latido esperado              */
    RK_LINK_TIBIO,       /* se pasó del latido pero todavía no cae  */
    RK_LINK_CAIDO        /* se lo da por desconectado               */
} rk_link_t;

/* Umbrales del enlace, en segundos. El latido del Mini es de 7.200 s en el
 * perfil adaptativo, así que TIBIO empieza al doble y CAIDO al triple: da
 * margen para una transmisión perdida sin volver el indicador nervioso. */
#define RK_LINK_TIBIO_S  14400u
#define RK_LINK_CAIDO_S  21600u

typedef struct {
    char                  nombre[18];   /* como la bautizó el usuario     */
    rk_role_t             role;
    uint8_t               id[6];        /* derivado de la MAC             */
    const rk_species_t   *sp;
    const rk_companion_t *comp;
    rk_bond_t             bond;
    rk_telemetry_t        tel;
    rk_mood_state_t       mst;
    rk_verdict_t          verdict;
} rk_node_t;

typedef struct {
    rk_node_t nodes[RK_MAX_NODES];
    int       count;
    int       selected;
    bool      wifi;
} rk_roster_t;

void rk_roster_init(rk_roster_t *r);

/* Da de alta un nodo. Devuelve su índice, o -1 si no hay lugar, si falta el
 * nombre o si ya hay un Prime y se pide otro: el kit tiene exactamente uno.
 * El simbionte sale de la especie, y con especie NULL queda sin asignar
 * hasta que la identificación por foto termine. */
int  rk_roster_add(rk_roster_t *r, const char *nombre, rk_role_t role,
                   const rk_species_t *sp);

/* Búsqueda por identificador de radio. Devuelve NULL si no está. */
rk_node_t *rk_roster_find(rk_roster_t *r, const uint8_t id[6]);

/* Índice del Prime, o -1 si el kit todavía no tiene uno. */
int  rk_roster_prime(const rk_roster_t *r);

/* Reevalúa el ánimo de todos los nodos a partir de su telemetría. Lo corre
 * el Prime sobre lo que recibe; cada Mini corre lo mismo sobre lo suyo. */
void rk_roster_eval(rk_roster_t *r);

rk_link_t   rk_node_link(const rk_node_t *n);
const char *rk_link_name(rk_link_t l);
const char *rk_role_name(rk_role_t r);

/* El nodo que más reclama atención: primero por severidad, y a igualdad de
 * severidad el que lleva más tiempo así. Es lo que el Prime muestra cuando
 * nadie tocó nada, y lo que decide qué planta va primero en el Hub. */
int  rk_roster_peor(const rk_roster_t *r);

/* Cierra el día de todos los nodos y avanza sus vínculos. Un día es "sano"
 * si terminó sin severidad urgente y con el nodo reportando. */
void rk_roster_cerrar_dia(rk_roster_t *r);

#endif /* ROOTKIT_NODE_H */
