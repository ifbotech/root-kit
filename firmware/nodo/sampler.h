/* sampler.h — cuándo medir y cuándo transmitir.
 *
 * La idea central, y la razón de que este módulo exista:
 *
 *   medir cuesta ~250 uAh, transmitir cuesta ~28.000 uAh
 *
 * Transmitir es unas 110 veces más caro que medir. Un firmware que mide y
 * transmite en el mismo ciclo desperdicia casi toda su batería mandando
 * lecturas idénticas a la anterior. Así que se desacoplan las dos cadencias:
 * se mide seguido, que es barato, y se transmite sólo cuando hay algo que
 * contar.
 *
 * "Algo que contar" es cualquiera de estas cuatro:
 *   - una magnitud se movió más que su banda muerta,
 *   - se cruzó un umbral de la especie (cambia el ánimo del simbionte),
 *   - cambió el estado de batería baja,
 *   - o venció el latido, para que el Prime no marque el nodo como caído.
 *
 * Además el período de medición se adapta: si todo está estable y lejos de
 * los bordes se estira, y si algo se acerca a un umbral se acorta. Una
 * planta que está por pedir agua se mira más seguido que una recién regada.
 */
#ifndef ROOTKIT_SAMPLER_H
#define ROOTKIT_SAMPLER_H

#include <stdint.h>
#include <stdbool.h>
#include "../net/proto.h"
#include "../core/species.h"

typedef struct {
    uint16_t base_interval_s;   /* período de medición al arrancar        */
    uint16_t min_interval_s;    /* piso cuando hay actividad              */
    uint16_t max_interval_s;    /* techo cuando todo está quieto          */
    uint16_t heartbeat_s;       /* silencio máximo tolerado               */
    uint8_t  soil_deadband;     /* puntos porcentuales                    */
    uint8_t  rh_deadband;
    int16_t  temp_deadband_dc;
    uint8_t  lux_deadband_pct;  /* cambio relativo, en porcentaje         */
    uint8_t  near_margin;       /* qué tan cerca de un umbral es "cerca"  */
} rk_sampler_cfg_t;

/* Configuración por defecto, pensada para una 18650 y una maceta de interior. */
extern const rk_sampler_cfg_t RK_SAMPLER_DEFAULT;

typedef enum {
    RK_TX_NO = 0,
    RK_TX_CAMBIO,       /* una magnitud se movió                          */
    RK_TX_UMBRAL,       /* se cruzó un límite de la especie               */
    RK_TX_BATERIA,      /* cambió el estado de batería                    */
    RK_TX_LATIDO,       /* venció el silencio máximo                      */
    RK_TX_PRIMERA       /* primera lectura tras el arranque               */
} rk_tx_reason_t;

typedef struct {
    rk_sampler_cfg_t   cfg;
    uint16_t           interval_s;
    uint32_t           last_tx_s;
    rk_telemetry_pkt_t last_sent;
    bool               have_last;
    bool               last_low_batt;
    uint16_t           seq;
    uint32_t           tx_count;
    uint32_t           measure_count;
} rk_sampler_t;

typedef struct {
    bool           transmit;
    rk_tx_reason_t reason;
    uint16_t       sleep_s;     /* cuánto dormir hasta la próxima medición */
    uint16_t       seq;         /* secuencia asignada si se transmite      */
} rk_sampler_decision_t;

void rk_sampler_init(rk_sampler_t *s, const rk_sampler_cfg_t *cfg);

/* Un ciclo completo: se llama después de medir, con la lectura fresca y el
 * tiempo de encendido acumulado. Decide si transmitir y cuánto dormir.
 * `sp` puede ser NULL si el nodo todavía no tiene especie asignada. */
rk_sampler_decision_t rk_sampler_step(rk_sampler_t *s,
                                      const rk_telemetry_pkt_t *now,
                                      uint32_t uptime_s,
                                      const rk_species_t *sp);

const char *rk_tx_reason_name(rk_tx_reason_t r);

/* Transmisiones por día que resultarían de este estado, para alimentar el
 * modelo de consumo con datos reales en vez de una estimación. */
uint32_t rk_sampler_tx_per_day(const rk_sampler_t *s, uint32_t uptime_s);

#endif /* ROOTKIT_SAMPLER_H */
