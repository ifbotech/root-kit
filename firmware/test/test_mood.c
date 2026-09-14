/* Tests de la máquina de estados de ánimo.
 * Corre sin LVGL, sin SDL y sin hardware:  make test
 */
#include <stdio.h>
#include <stddef.h>
#include "../core/mood.h"

static int total  = 0;
static int fallas = 0;

static void check(const char *caso, rk_mood_t esperado, rk_verdict_t got)
{
    total++;
    if (esperado == got.mood) {
        printf("  ok     %-40s -> %-12s %s\n",
               caso, rk_mood_name(got.mood), got.reason);
    } else {
        fallas++;
        printf("  FALLA  %-40s -> esperaba %s, obtuvo %s\n",
               caso, rk_mood_name(esperado), rk_mood_name(got.mood));
    }
}

static void check_sev(const char *caso, rk_severity_t esperada, rk_verdict_t got)
{
    total++;
    if (esperada == got.severity) {
        printf("  ok     %-40s -> severidad %d\n", caso, (int)got.severity);
    } else {
        fallas++;
        printf("  FALLA  %-40s -> esperaba severidad %d, obtuvo %d\n",
               caso, (int)esperada, (int)got.severity);
    }
}

static rk_telemetry_t tel(uint8_t soil, int16_t temp_dc, uint8_t rh, uint32_t lux)
{
    rk_telemetry_t t;
    t.valid    = true;
    t.age_s    = 60;
    t.soil_pct = soil;
    t.temp_dc  = temp_dc;
    t.rh_pct   = rh;
    t.lux      = lux;
    t.batt_mv  = 3900;
    return t;
}

int main(void)
{
    const rk_species_t *m = rk_species_find("monstera");
    rk_mood_state_t st;
    rk_telemetry_t t;
    int i;

    if (m == NULL) {
        printf("no se encontró la especie de prueba\n");
        return 1;
    }
    printf("Especie: %s  (suelo %u-%u%%, %d,%d-%d,%d C, HR>=%u%%, %lu-%lu lux)\n\n",
           m->nombre, (unsigned)m->soil_min, (unsigned)m->soil_max,
           m->temp_min_dc / 10, m->temp_min_dc % 10,
           m->temp_max_dc / 10, m->temp_max_dc % 10,
           (unsigned)m->rh_min,
           (unsigned long)m->lux_min, (unsigned long)m->lux_max);

    printf("Lecturas aisladas\n");
    rk_mood_state_init(&st);
    t = tel(40, 240, 60, 5000);
    check("todo en rango", RK_MOOD_HAPPY, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(15, 240, 60, 5000);
    check("tierra muy seca", RK_MOOD_THIRSTY, rk_mood_eval(&st, m, &t));
    rk_mood_state_init(&st);
    check_sev("tierra muy seca es urgente", RK_SEV_URGENT, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(22, 240, 60, 5000);
    check("tierra apenas seca", RK_MOOD_THIRSTY, rk_mood_eval(&st, m, &t));
    rk_mood_state_init(&st);
    check_sev("tierra apenas seca es aviso", RK_SEV_WATCH, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(80, 240, 60, 5000);
    check("encharcada", RK_MOOD_DROWNING, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(40, 100, 60, 5000);
    check("10 C: frio", RK_MOOD_COLD, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(40, 360, 60, 5000);
    check("36 C: calor", RK_MOOD_HOT, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(40, 240, 60, 200);
    check("200 lux de dia: poca luz", RK_MOOD_DARK, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(40, 240, 60, 40000);
    check("40k lux: sol directo", RK_MOOD_SCORCHED, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(40, 240, 30, 5000);
    check("aire seco", RK_MOOD_PARCHED_AIR, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(40, 240, 60, 5000);
    t.age_s = 7200;
    check("telemetria vieja", RK_MOOD_OFFLINE, rk_mood_eval(&st, m, &t));

    printf("\nPrioridad: el agua le gana al resto\n");
    rk_mood_state_init(&st);
    t = tel(15, 100, 30, 200);
    check("seca + fria + oscura + aire seco", RK_MOOD_THIRSTY, rk_mood_eval(&st, m, &t));

    printf("\nNoche: oscuridad sostenida deja de ser queja\n");
    rk_mood_state_init(&st);
    t = tel(40, 240, 60, 0);
    /* Dejamos el contador en MUESTRAS_NOCHE-2 para que las dos evaluaciones
     * siguientes caigan justo a los lados del umbral. */
    for (i = 0; i < MUESTRAS_NOCHE - 2; i++) {
        rk_mood_eval(&st, m, &t);
    }
    check("septima muestra a oscuras", RK_MOOD_DARK, rk_mood_eval(&st, m, &t));
    check("octava muestra a oscuras", RK_MOOD_SLEEPING, rk_mood_eval(&st, m, &t));

    t = tel(15, 240, 60, 0);
    check("de noche pero sin agua: gana el agua", RK_MOOD_THIRSTY, rk_mood_eval(&st, m, &t));

    printf("\nHisteresis: no titila en el borde\n");
    rk_mood_state_init(&st);
    t = tel(24, 240, 60, 5000);
    check("24% arranca sediento", RK_MOOD_THIRSTY, rk_mood_eval(&st, m, &t));
    t = tel(27, 240, 60, 5000);
    check("27% sigue sediento (banda)", RK_MOOD_THIRSTY, rk_mood_eval(&st, m, &t));
    t = tel(32, 240, 60, 5000);
    check("32% ya sale del estado", RK_MOOD_HAPPY, rk_mood_eval(&st, m, &t));

    printf("\n%d comprobaciones, %d fallas\n", total, fallas);
    return fallas == 0 ? 0 : 1;
}
