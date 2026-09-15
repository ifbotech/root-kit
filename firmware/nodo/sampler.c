#include "sampler.h"
#include <stddef.h>
#include <string.h>

const rk_sampler_cfg_t RK_SAMPLER_DEFAULT = {
    300u,      /* mide cada 5 min                                          */
    120u,      /* hasta cada 2 min si hay movimiento                       */
    1800u,     /* hasta cada 30 min si no pasa nada                        */
    7200u,     /* latido cada 2 h: el Prime da por caído a las 6 h         */
    3u,        /* 3 puntos de humedad de suelo                             */
    5u,        /* 5 puntos de humedad relativa                             */
    8,         /* 0,8 grados                                               */
    25u,       /* 25% de cambio relativo de luz                            */
    5u         /* "cerca de un umbral" = 5 unidades                        */
};

void rk_sampler_init(rk_sampler_t *s, const rk_sampler_cfg_t *cfg)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof *s);
    s->cfg        = (cfg != NULL) ? *cfg : RK_SAMPLER_DEFAULT;
    s->interval_s = s->cfg.base_interval_s;
    s->have_last  = false;
    s->seq        = 0;
}

static uint16_t abs_diff_u8(uint8_t a, uint8_t b)
{
    return (uint16_t)(a > b ? a - b : b - a);
}

static int16_t abs_diff_i16(int16_t a, int16_t b)
{
    int32_t d = (int32_t)a - (int32_t)b;
    return (int16_t)(d < 0 ? -d : d);
}

/* Cambio relativo de luz. En lineal no sirve: pasar de 100 a 200 lux importa
 * (el doble) y pasar de 20.000 a 20.100 no, aunque la diferencia absoluta
 * sea la misma. */
static bool lux_changed(uint32_t a, uint32_t b, uint8_t pct)
{
    uint32_t hi = a > b ? a : b;
    uint32_t lo = a > b ? b : a;
    uint32_t delta;

    if (hi < 20u) {
        return false;             /* de noche todo es ruido */
    }
    delta = hi - lo;
    return (delta * 100u) > (hi * (uint32_t)pct);
}

/* ¿Alguna magnitud está a punto de cruzar un límite de la especie? */
static bool near_threshold(const rk_telemetry_pkt_t *t,
                           const rk_species_t *sp, uint8_t margin)
{
    if (sp == NULL) {
        return false;
    }
    if (t->soil_pct <= sp->soil_min + margin ||
        (t->soil_pct + margin) >= sp->soil_max) {
        return true;
    }
    if (t->temp_dc <= (int16_t)(sp->temp_min_dc + margin * 10) ||
        t->temp_dc >= (int16_t)(sp->temp_max_dc - margin * 10)) {
        return true;
    }
    return false;
}

/* ¿La lectura nueva cae de otro lado de un umbral que la anterior? Eso
 * significa que el simbionte va a cambiar de cara, y eso siempre se
 * transmite aunque el movimiento numérico sea de un solo punto. */
static bool crossed_threshold(const rk_telemetry_pkt_t *a,
                              const rk_telemetry_pkt_t *b,
                              const rk_species_t *sp)
{
    if (sp == NULL) {
        return false;
    }
    if ((a->soil_pct < sp->soil_min) != (b->soil_pct < sp->soil_min)) { return true; }
    if ((a->soil_pct > sp->soil_max) != (b->soil_pct > sp->soil_max)) { return true; }
    if ((a->temp_dc < sp->temp_min_dc) != (b->temp_dc < sp->temp_min_dc)) { return true; }
    if ((a->temp_dc > sp->temp_max_dc) != (b->temp_dc > sp->temp_max_dc)) { return true; }
    if ((a->rh_pct  < sp->rh_min)      != (b->rh_pct  < sp->rh_min))      { return true; }
    if ((a->lux < sp->lux_min) != (b->lux < sp->lux_min)) { return true; }
    if ((a->lux > sp->lux_max) != (b->lux > sp->lux_max)) { return true; }
    return false;
}

rk_sampler_decision_t rk_sampler_step(rk_sampler_t *s,
                                      const rk_telemetry_pkt_t *now,
                                      uint32_t uptime_s,
                                      const rk_species_t *sp)
{
    rk_sampler_decision_t d;
    bool low, moved = false;

    d.transmit = false;
    d.reason   = RK_TX_NO;
    d.sleep_s  = (s != NULL) ? s->interval_s : 300u;
    d.seq      = 0;

    if (s == NULL || now == NULL) {
        return d;
    }
    s->measure_count++;
    low = (now->flags & RK_FLAG_LOW_BATT) != 0u;

    /* ---- ¿hay algo que contar? ---------------------------------------- */
    if (!s->have_last) {
        d.reason = RK_TX_PRIMERA;
    } else {
        moved =
            abs_diff_u8(now->soil_pct, s->last_sent.soil_pct) >= s->cfg.soil_deadband ||
            abs_diff_u8(now->rh_pct,   s->last_sent.rh_pct)   >= s->cfg.rh_deadband   ||
            abs_diff_i16(now->temp_dc, s->last_sent.temp_dc)  >= s->cfg.temp_deadband_dc ||
            lux_changed(now->lux, s->last_sent.lux, s->cfg.lux_deadband_pct);

        if (crossed_threshold(&s->last_sent, now, sp)) {
            d.reason = RK_TX_UMBRAL;        /* prioridad: cambia el ánimo   */
        } else if (low != s->last_low_batt) {
            d.reason = RK_TX_BATERIA;
        } else if (moved) {
            d.reason = RK_TX_CAMBIO;
        } else if (uptime_s - s->last_tx_s >= (uint32_t)s->cfg.heartbeat_s) {
            d.reason = RK_TX_LATIDO;
        }
    }
    d.transmit = (d.reason != RK_TX_NO);

    /* ---- adaptar el período de medición -------------------------------- */
    /* Cerca de un umbral o con la planta moviéndose, acortamos hasta el piso.
     * Con todo quieto, estiramos de a poco: subir rápido y bajar despacio
     * haría que el nodo reaccione tarde justo cuando importa. */
    if (near_threshold(now, sp, s->cfg.near_margin) ||
        d.reason == RK_TX_UMBRAL || moved) {
        s->interval_s = s->cfg.min_interval_s;
    } else if (d.reason == RK_TX_NO || d.reason == RK_TX_LATIDO) {
        uint32_t next = (uint32_t)s->interval_s + s->interval_s / 4u;
        if (next > (uint32_t)s->cfg.max_interval_s) {
            next = s->cfg.max_interval_s;
        }
        s->interval_s = (uint16_t)next;
    }
    d.sleep_s = s->interval_s;

    /* ---- registrar la transmisión --------------------------------------- */
    if (d.transmit) {
        s->seq++;
        d.seq            = s->seq;
        s->last_sent     = *now;
        s->last_sent.seq = s->seq;
        s->last_tx_s     = uptime_s;
        s->last_low_batt = low;
        s->have_last     = true;
        s->tx_count++;
    }
    return d;
}

const char *rk_tx_reason_name(rk_tx_reason_t r)
{
    switch (r) {
    case RK_TX_NO:      return "sin novedad";
    case RK_TX_CAMBIO:  return "cambio de lectura";
    case RK_TX_UMBRAL:  return "cruce de umbral";
    case RK_TX_BATERIA: return "estado de bateria";
    case RK_TX_LATIDO:  return "latido";
    case RK_TX_PRIMERA: return "primera lectura";
    default:            return "?";
    }
}

uint32_t rk_sampler_tx_per_day(const rk_sampler_t *s, uint32_t uptime_s)
{
    if (s == NULL || uptime_s == 0u) {
        return 0;
    }
    return (s->tx_count * 86400u) / uptime_s;
}
