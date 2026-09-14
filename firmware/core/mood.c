#include "mood.h"
#include <stddef.h>

/* Márgenes de histéresis. Para SALIR de un estado hay que cruzar el umbral
 * por este margen extra; si no, el simbionte titila entre feliz y sediento
 * cada vez que la lectura oscila un punto sobre el límite. */
#define HYST_SOIL_PCT     4
#define HYST_TEMP_DC     15    /* 1,5 °C */
#define HYST_RH_PCT       5
#define HYST_LUX_DIV      5    /* 20 % */

/* Telemetría más vieja que esto: damos el Spore por caído (tres ciclos). */
#define OFFLINE_S      5400u

/* Cuánto hay que pasarse del umbral para que la alerta sea urgente. */
#define URGENTE_SOIL_PCT  8
#define URGENTE_TEMP_DC  50    /* 5,0 °C */

void rk_mood_state_init(rk_mood_state_t *st)
{
    if (st == NULL) {
        return;
    }
    st->last_mood     = RK_MOOD_UNKNOWN;
    st->dark_samples  = 0;
    st->light_samples = 0;
}

/* ¿El estado ya estaba activo en la lectura anterior? Si sí, aplicamos el
 * margen de histéresis antes de dejarlo salir. */
static bool sticky(const rk_mood_state_t *st, rk_mood_t m)
{
    return st->last_mood == m;
}

static rk_verdict_t mk(rk_verdict_t *out, rk_mood_state_t *st,
                       rk_mood_t m, rk_severity_t s, const char *r)
{
    if (st != NULL) {
        st->last_mood = m;
    }
    out->mood     = m;
    out->severity = s;
    out->reason   = r;
    return *out;
}

rk_verdict_t rk_mood_eval(rk_mood_state_t      *st,
                          const rk_species_t   *sp,
                          const rk_telemetry_t *t)
{
    rk_verdict_t v;
    bool     es_noche;
    uint8_t  seco, mojado, rh_min;
    int16_t  frio, calor;
    uint32_t lux_min, lux_max;

    v.mood = RK_MOOD_UNKNOWN;
    v.severity = RK_SEV_OK;
    v.reason = "sin datos";

    if (st == NULL || sp == NULL || t == NULL) {
        return v;
    }

    if (!t->valid || t->age_s > OFFLINE_S) {
        return mk(&v, st, RK_MOOD_OFFLINE, RK_SEV_WATCH, "el Spore no reporta");
    }

    /* ---- Ciclo día / noche ------------------------------------------- */
    if (t->lux < LUX_NOCHE) {
        if (st->dark_samples < 0xFFFFu) {
            st->dark_samples++;
        }
        st->light_samples = 0;
    } else {
        if (st->light_samples < 0xFFFFu) {
            st->light_samples++;
        }
        st->dark_samples = 0;
    }
    es_noche = (st->dark_samples >= MUESTRAS_NOCHE);

    /* ---- 1. Agua: lo más urgente, y lo que mata más rápido ------------ */
    seco   = sp->soil_min;
    mojado = sp->soil_max;
    if (sticky(st, RK_MOOD_THIRSTY)) {
        seco = (uint8_t)(seco + HYST_SOIL_PCT);
    }
    if (sticky(st, RK_MOOD_DROWNING) && mojado > HYST_SOIL_PCT) {
        mojado = (uint8_t)(mojado - HYST_SOIL_PCT);
    }

    if (t->soil_pct < seco) {
        rk_severity_t s = (t->soil_pct + URGENTE_SOIL_PCT < sp->soil_min)
                          ? RK_SEV_URGENT : RK_SEV_WATCH;
        return mk(&v, st, RK_MOOD_THIRSTY, s, "la tierra está seca");
    }
    if (t->soil_pct > mojado) {
        rk_severity_t s = (t->soil_pct > sp->soil_max + 12)
                          ? RK_SEV_URGENT : RK_SEV_WATCH;
        return mk(&v, st, RK_MOOD_DROWNING, s, "exceso de agua en la raíz");
    }

    /* ---- 2. Temperatura ----------------------------------------------- */
    frio  = sp->temp_min_dc;
    calor = sp->temp_max_dc;
    if (sticky(st, RK_MOOD_COLD)) {
        frio = (int16_t)(frio + HYST_TEMP_DC);
    }
    if (sticky(st, RK_MOOD_HOT)) {
        calor = (int16_t)(calor - HYST_TEMP_DC);
    }

    if (t->temp_dc < frio) {
        rk_severity_t s = (t->temp_dc < sp->temp_min_dc - URGENTE_TEMP_DC)
                          ? RK_SEV_URGENT : RK_SEV_WATCH;
        return mk(&v, st, RK_MOOD_COLD, s, "hace frío para esta especie");
    }
    if (t->temp_dc > calor) {
        rk_severity_t s = (t->temp_dc > sp->temp_max_dc + URGENTE_TEMP_DC)
                          ? RK_SEV_URGENT : RK_SEV_WATCH;
        return mk(&v, st, RK_MOOD_HOT, s, "hace calor para esta especie");
    }

    /* ---- 3. De noche no se juzga la luz ni la humedad del aire -------- */
    if (es_noche) {
        return mk(&v, st, RK_MOOD_SLEEPING, RK_SEV_OK, "durmiendo");
    }

    /* ---- 4. Luz -------------------------------------------------------- */
    lux_min = sp->lux_min;
    lux_max = sp->lux_max;
    if (sticky(st, RK_MOOD_DARK)) {
        lux_min += lux_min / HYST_LUX_DIV;
    }
    if (sticky(st, RK_MOOD_SCORCHED)) {
        lux_max -= lux_max / HYST_LUX_DIV;
    }

    if (t->lux > lux_max) {
        return mk(&v, st, RK_MOOD_SCORCHED, RK_SEV_WATCH, "demasiado sol directo");
    }
    if (t->lux < lux_min) {
        return mk(&v, st, RK_MOOD_DARK, RK_SEV_WATCH, "le falta luz");
    }

    /* ---- 5. Humedad del aire: el problema más lento, va último -------- */
    rh_min = sp->rh_min;
    if (sticky(st, RK_MOOD_PARCHED_AIR)) {
        rh_min = (uint8_t)(rh_min + HYST_RH_PCT);
    }
    if (t->rh_pct < rh_min) {
        return mk(&v, st, RK_MOOD_PARCHED_AIR, RK_SEV_WATCH, "el aire está muy seco");
    }

    return mk(&v, st, RK_MOOD_HAPPY, RK_SEV_OK, "todo en rango");
}

const char *rk_mood_name(rk_mood_t m)
{
    switch (m) {
    case RK_MOOD_UNKNOWN:     return "UNKNOWN";
    case RK_MOOD_OFFLINE:     return "OFFLINE";
    case RK_MOOD_SLEEPING:    return "SLEEPING";
    case RK_MOOD_HAPPY:       return "HAPPY";
    case RK_MOOD_THIRSTY:     return "THIRSTY";
    case RK_MOOD_DROWNING:    return "DROWNING";
    case RK_MOOD_COLD:        return "COLD";
    case RK_MOOD_HOT:         return "HOT";
    case RK_MOOD_SCORCHED:    return "SCORCHED";
    case RK_MOOD_DARK:        return "DARK";
    case RK_MOOD_PARCHED_AIR: return "PARCHED_AIR";
    default:                  return "??";
    }
}
