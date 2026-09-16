/* link.h — el pegamento entre el kit y el protocolo.
 *
 * Acá viven las cuatro conversiones que hacen que la app y los aparatos se
 * entiendan, y ninguna otra. Es la capa que el ESP32 va a llamar desde sus
 * callbacks de radio, y es puro C99 sin dependencias, así que se testea
 * entera en el escritorio sin una placa.
 *
 *     app / concentrador                             aparato
 *     ------------------                             -------
 *     rk_link_config_from_node()  --- CONFIG --->    rk_link_apply_config()
 *     rk_link_ingest()            <-- TELEMETRY ---  rk_link_telemetry_from_node()
 *
 * LA REGLA QUE ESTE MÓDULO HACE CUMPLIR
 *
 * El ánimo se evalúa UNA vez, en el aparato dueño de la maceta, y viaja
 * resuelto. `rk_link_ingest` lo copia tal cual al roster y NO vuelve a
 * llamar a rk_mood_eval sobre esos datos. Si algún día alguien agrega esa
 * llamada "por las dudas", las dos puntas pueden discrepar —distinta
 * histéresis acumulada, distinto conteo de muestras oscuras— y el usuario ve
 * una cara en la maceta y otra en el teléfono sin forma de saber cuál le
 * miente. Hay un test que fija este comportamiento.
 */
#ifndef ROOTKIT_LINK_H
#define ROOTKIT_LINK_H

#include "proto.h"
#include "../core/node.h"
#include "../nodo/soil.h"

#define RK_PERSONA_NINGUNA 0xFFu

/* --------------------------------------------------- lado de la app ---- */

/* Arma la configuración que hay que mandarle a un nodo: sus umbrales de
 * especie, su calibración de suelo, su simbionte y su etapa. Devuelve false
 * si falta el nodo o la especie —sin especie no puede evaluarse solo y no
 * tiene sentido configurarlo todavía. */
bool rk_link_config_from_node(const rk_node_t *n, uint16_t interval_s,
                              const rk_soil_cal_t *cal, rk_config_pkt_t *out);

/* Aplica una telemetría recibida al roster. Devuelve el nodo actualizado, o
 * NULL si el id no está en el kit. `now_s` es el reloj de quien recibe, y de
 * él sale la antigüedad que después decide el estado del enlace. */
rk_node_t *rk_link_ingest(rk_roster_t *r, const rk_telemetry_pkt_t *p,
                          uint32_t now_s);

/* Envejece la telemetría de todos los nodos. Se llama una vez por segundo de
 * reloj: es lo que hace que un nodo que dejó de hablar termine en CAIDO sin
 * necesidad de un temporizador por nodo. */
void rk_link_envejecer(rk_roster_t *r, uint32_t delta_s);

/* ----------------------------------------------- lado del aparato ------ */

/* Reconstruye del paquete de configuración la especie con la que el aparato
 * va a evaluarse, y qué carcasa declaró el usuario que lleva puesta. `sp_out` es memoria del llamador: los umbrales viajan, los
 * textos no, así que id y nombre quedan vacíos —el evaluador no los usa. */
void rk_link_apply_config(const rk_config_pkt_t *cfg, rk_species_t *sp_out,
                          const rk_persona_t **persona_out,
                          rk_stage_t *etapa_out);

/* Arma la telemetría que el aparato emite, con su ánimo ya evaluado. */
void rk_link_telemetry_from_node(const rk_node_t *n, uint16_t seq,
                                 uint16_t soil_raw, uint8_t flags,
                                 rk_telemetry_pkt_t *out);

#endif /* ROOTKIT_LINK_H */
