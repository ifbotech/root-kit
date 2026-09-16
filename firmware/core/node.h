/* node.h — el kit: los ROOTKIT que tiene una casa.
 *
 * Un ROOTKIT es un aparato en una maceta: una placa, sus sensores y una
 * carcasa impresa que le da la cara. Este módulo es la tabla de los que hay,
 * y es lo que la app muestra.
 *
 * TRES REGLAS QUE ORDENAN EL SISTEMA
 *
 * 1. CADA NODO EVALÚA SU PROPIA MACETA.
 *    Corre core/mood.c con los umbrales de su especie, que le llegaron en la
 *    trama de configuración, y muestra el ánimo sin preguntarle a nadie. Un
 *    aparato que necesita la red para saber qué cara poner se queda mudo
 *    justo cuando más importa.
 *
 * 2. LA ESPECIE DECIDE LOS UMBRALES; LA CARCASA DECIDE LA CARA.
 *    Son dos ejes independientes y esa independencia es el producto: el
 *    mismo modelo cuidando dos plantas distintas se comporta distinto, y dos
 *    modelos cuidando la misma planta se ven distintos.
 *
 * 3. LAS MÉTRICAS VIVEN EN LA APP.
 *    Este módulo es lo que la app sirve. El aparato sólo dibuja una cara.
 */
#ifndef ROOTKIT_NODE_H
#define ROOTKIT_NODE_H

#include <stdint.h>
#include <stdbool.h>
#include "mood.h"
#include "species.h"
#include "persona.h"
#include "vinculo.h"

/* Cuántos aparatos maneja una app. El tope existe para que las estructuras
 * sean estáticas y quepan en RAM; la app puede paginar si alguna vez hace
 * falta más. */
#define RK_MAX_NODES 8

/* Salud del enlace, derivada de la antigüedad de la última trama. Es
 * distinto del ánimo: un nodo puede estar callado y la planta perfecta. */
typedef enum {
    RK_LINK_NUNCA = 0,   /* nunca habló: recién dado de alta        */
    RK_LINK_VIVO,        /* dentro del latido esperado              */
    RK_LINK_TIBIO,       /* se pasó del latido pero todavía no cae  */
    RK_LINK_CAIDO        /* se lo da por desconectado               */
} rk_link_t;

/* Umbrales del enlace, en segundos. El latido es de 7.200 s en el perfil
 * adaptativo, así que TIBIO empieza al doble y CAIDO al triple: da margen
 * para una transmisión perdida sin volver el indicador nervioso. */
#define RK_LINK_TIBIO_S  14400u
#define RK_LINK_CAIDO_S  21600u

typedef struct {
    char                  nombre[18];   /* como la bautizó el usuario     */
    uint8_t               id[6];        /* derivado de la MAC             */
    const rk_species_t   *sp;
    /* Qué carcasa lleva puesta. Sale de la caja ciega y la carga el usuario
     * en la app, no de la planta: la especie decide los umbrales, la carcasa
     * decide la cara. */
    const rk_persona_t   *persona;
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

/* Da de alta un nodo. Devuelve su índice, o -1 si no hay lugar o falta el
 * nombre. La especie puede ser NULL hasta que la identificación por foto
 * termine, y la persona hasta que el usuario diga qué carcasa le tocó. */
int  rk_roster_add(rk_roster_t *r, const char *nombre,
                   const rk_species_t *sp, const rk_persona_t *persona);

/* Búsqueda por identificador de radio. Devuelve NULL si no está. */
rk_node_t *rk_roster_find(rk_roster_t *r, const uint8_t id[6]);

/* Reevalúa el ánimo de todos los nodos a partir de su telemetría. Lo corre
 * la app sobre lo que recibe; cada aparato corre lo mismo sobre lo suyo. */
void rk_roster_eval(rk_roster_t *r);

rk_link_t   rk_node_link(const rk_node_t *n);
const char *rk_link_name(rk_link_t l);

/* El nodo que más reclama atención: primero por severidad, y a igualdad de
 * severidad el que lleva más tiempo así. Es lo que decide qué maceta va
 * primero en la lista de la app. */
int  rk_roster_peor(const rk_roster_t *r);

/* Cierra el día de todos los nodos y avanza sus vínculos. Un día es "sano"
 * si terminó sin severidad urgente y con el nodo reportando. */
void rk_roster_cerrar_dia(rk_roster_t *r);

#endif /* ROOTKIT_NODE_H */
