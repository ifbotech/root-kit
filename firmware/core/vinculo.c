#include "vinculo.h"
#include <stddef.h>

/* ------------------------------------------------------------ vinculo ---- */
/* Días SANOS acumulados que hacen falta para entrar en cada etapa. El umbral
 * es de días sanos y no de días transcurridos: una planta abandonada tiene un
 * simbionte que no crece, y ahí está toda la mecánica de vínculo. */
static const uint16_t UMBRAL[RK_ETAPA_COUNT] = { 0u, 7u, 30u, 90u, 180u };

void rk_bond_init(rk_bond_t *b)
{
    if (b == NULL) {
        return;
    }
    b->dias_vividos = 0;
    b->dias_sanos   = 0;
    b->racha        = 0;
    b->mejor_racha  = 0;
}

void rk_bond_dia(rk_bond_t *b, bool sano)
{
    if (b == NULL) {
        return;
    }
    if (b->dias_vividos < 0xFFFFu) {
        b->dias_vividos++;
    }
    if (sano) {
        if (b->dias_sanos < 0xFFFFu) {
            b->dias_sanos++;
        }
        if (b->racha < 0xFFFFu) {
            b->racha++;
        }
        if (b->racha > b->mejor_racha) {
            b->mejor_racha = b->racha;
        }
    } else {
        /* La racha se corta, pero los días sanos acumulados NO se pierden.
         * Castigar el olvido borrando meses de cuidado convierte un mal día
         * en motivo para abandonar el producto. */
        b->racha = 0;
    }
}

rk_stage_t rk_stage_from_bond(const rk_bond_t *b)
{
    int i;

    if (b == NULL) {
        return RK_ETAPA_ESPORA;
    }
    for (i = RK_ETAPA_COUNT - 1; i > 0; i--) {
        if (b->dias_sanos >= UMBRAL[i]) {
            return (rk_stage_t)i;
        }
    }
    return RK_ETAPA_ESPORA;
}

const char *rk_stage_name(rk_stage_t e)
{
    switch (e) {
    case RK_ETAPA_ESPORA:     return "ESPORA";
    case RK_ETAPA_BROTE:      return "BROTE";
    case RK_ETAPA_JOVEN:      return "JOVEN";
    case RK_ETAPA_MADURO:     return "MADURO";
    case RK_ETAPA_ANCESTRAL:  return "ANCESTRAL";
    default:                  return "?";
    }
}

uint16_t rk_stage_faltan(const rk_bond_t *b)
{
    rk_stage_t e;

    if (b == NULL) {
        return UMBRAL[RK_ETAPA_BROTE];
    }
    e = rk_stage_from_bond(b);
    if (e >= RK_ETAPA_COUNT - 1) {
        return 0;
    }
    return (uint16_t)(UMBRAL[e + 1] - b->dias_sanos);
}

uint8_t rk_stage_progreso(const rk_bond_t *b)
{
    rk_stage_t e;
    uint16_t desde, hasta, span;

    if (b == NULL) {
        return 0;
    }
    e = rk_stage_from_bond(b);
    if (e >= RK_ETAPA_COUNT - 1) {
        return 100;
    }
    desde = UMBRAL[e];
    hasta = UMBRAL[e + 1];
    span  = (uint16_t)(hasta - desde);
    if (span == 0u) {
        return 100;
    }
    return (uint8_t)(((uint32_t)(b->dias_sanos - desde) * 100u) / span);
}
