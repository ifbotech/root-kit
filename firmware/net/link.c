#include "link.h"
#include <stddef.h>
#include <string.h>

/* --------------------------------------------------- lado de la app ---- */
bool rk_link_config_from_node(const rk_node_t *n, uint16_t interval_s,
                              const rk_soil_cal_t *cal, rk_config_pkt_t *out)
{
    int idx;

    if (n == NULL || out == NULL || n->sp == NULL) {
        return false;
    }
    memset(out, 0, sizeof *out);
    memcpy(out->id, n->id, 6);
    out->interval_s   = interval_s;

    if (cal != NULL && rk_soil_cal_valid(cal)) {
        out->soil_dry_raw = cal->dry_raw;
        out->soil_wet_raw = cal->wet_raw;
        out->flags        = RK_FLAG_CALIBRATED;
    } else {
        out->soil_dry_raw = RK_SOIL_CAL_DEFAULT.dry_raw;
        out->soil_wet_raw = RK_SOIL_CAL_DEFAULT.wet_raw;
        out->flags        = 0u;
    }

    out->soil_min    = n->sp->soil_min;
    out->soil_max    = n->sp->soil_max;
    out->temp_min_dc = n->sp->temp_min_dc;
    out->temp_max_dc = n->sp->temp_max_dc;
    out->rh_min      = n->sp->rh_min;
    out->lux_min     = n->sp->lux_min;
    out->lux_max     = n->sp->lux_max;

    idx = rk_persona_index(n->persona);
    out->persona_idx = (idx >= 0 && idx < 255) ? (uint8_t)idx
                                               : RK_PERSONA_NINGUNA;
    out->etapa    = (uint8_t)rk_stage_from_bond(&n->bond);
    return true;
}

rk_node_t *rk_link_ingest(rk_roster_t *r, const rk_telemetry_pkt_t *p,
                          uint32_t now_s)
{
    rk_node_t *n;

    (void)now_s;
    if (r == NULL || p == NULL) {
        return NULL;
    }
    n = rk_roster_find(r, p->id);
    if (n == NULL) {
        return NULL;
    }

    n->tel.valid    = true;
    n->tel.age_s    = 0u;          /* recién llegada */
    n->tel.soil_pct = p->soil_pct;
    n->tel.temp_dc  = p->temp_dc;
    n->tel.rh_pct   = p->rh_pct;
    n->tel.lux      = p->lux;
    n->tel.batt_mv  = p->batt_mv;

    /* El ánimo llega resuelto desde el aparato y se copia tal cual. NO se
     * vuelve a llamar a rk_mood_eval acá: ver la nota de link.h. */
    n->verdict.mood     = (rk_mood_t)p->mood;
    n->verdict.severity = (rk_severity_t)p->severity;
    n->verdict.reason   = rk_mood_reason((rk_mood_t)p->mood);

    return n;
}

void rk_link_envejecer(rk_roster_t *r, uint32_t delta_s)
{
    int i;

    if (r == NULL) {
        return;
    }
    for (i = 0; i < r->count && i < RK_MAX_NODES; i++) {
        rk_node_t *n = &r->nodes[i];
        if (!n->tel.valid) {
            continue;
        }
        /* Saturación en vez de envolvimiento: un nodo silencioso durante
         * 136 años tiene que seguir leyéndose como caído, no como recién
         * llegado. */
        if (n->tel.age_s > 0xFFFFFFFFu - delta_s) {
            n->tel.age_s = 0xFFFFFFFFu;
        } else {
            n->tel.age_s += delta_s;
        }
    }
}

/* ----------------------------------------------- lado del aparato ------ */
void rk_link_apply_config(const rk_config_pkt_t *cfg, rk_species_t *sp_out,
                          const rk_persona_t **persona_out,
                          rk_stage_t *etapa_out)
{
    if (cfg == NULL) {
        return;
    }
    if (sp_out != NULL) {
        memset(sp_out, 0, sizeof *sp_out);
        sp_out->id          = "";
        sp_out->nombre      = "";
        sp_out->soil_min    = cfg->soil_min;
        sp_out->soil_max    = cfg->soil_max;
        sp_out->temp_min_dc = cfg->temp_min_dc;
        sp_out->temp_max_dc = cfg->temp_max_dc;
        sp_out->rh_min      = cfg->rh_min;
        sp_out->lux_min     = cfg->lux_min;
        sp_out->lux_max     = cfg->lux_max;
        /* La dificultad de la especie ya no viaja porque ya no significa
         * nada: la rareza se mudó a la caja física. El campo queda en la
         * struct para la app, que sí la muestra al elegir una planta. */
        sp_out->dificultad  = 0u;
    }
    if (persona_out != NULL) {
        *persona_out = rk_persona_at((int)cfg->persona_idx);
    }
    if (etapa_out != NULL) {
        *etapa_out = (cfg->etapa < (uint8_t)RK_ETAPA_COUNT)
                     ? (rk_stage_t)cfg->etapa : RK_ETAPA_ESPORA;
    }
}

void rk_link_telemetry_from_node(const rk_node_t *n, uint16_t seq,
                                 uint16_t soil_raw, uint8_t flags,
                                 rk_telemetry_pkt_t *out)
{
    if (n == NULL || out == NULL) {
        return;
    }
    memset(out, 0, sizeof *out);
    memcpy(out->id, n->id, 6);
    out->seq      = seq;
    out->flags    = flags;
    out->soil_pct = n->tel.soil_pct;
    out->temp_dc  = n->tel.temp_dc;
    out->rh_pct   = n->tel.rh_pct;
    out->lux      = n->tel.lux;
    out->batt_mv  = n->tel.batt_mv;
    out->soil_raw = soil_raw;
    out->mood     = (uint8_t)n->verdict.mood;
    out->severity = (uint8_t)n->verdict.severity;
    out->etapa    = (uint8_t)rk_stage_from_bond(&n->bond);
}
