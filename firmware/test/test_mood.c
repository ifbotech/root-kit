/* Tests de la máquina de estados de ánimo: prioridad entre necesidades,
 * ciclo día/noche, histéresis en los bordes y detección de nodo caído. */
#include <stddef.h>
#include "rk_test.h"
#include "../core/mood.h"

static void check(const char *caso, rk_mood_t esperado, rk_verdict_t got)
{
    CHECK_STR(caso, rk_mood_name(esperado), rk_mood_name(got.mood));
}

static void check_sev(const char *caso, rk_severity_t esperada, rk_verdict_t got)
{
    CHECK_INT(caso, (int)esperada, (int)got.severity);
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

void suite_mood(void)
{
    const rk_species_t *m = rk_species_find("monstera");
    rk_mood_state_t st;
    rk_telemetry_t t;
    int i;

    RK_SUITE("animo");

    if (m == NULL) {
        rk_t_fail("especie de prueba", "no se encontro monstera");
        RK_SUITE_END();
        return;
    }

    /* ---- lecturas aisladas -------------------------------------------- */
    rk_mood_state_init(&st);
    t = tel(40, 240, 60, 5000);
    check("todo en rango", RK_MOOD_HAPPY, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(15, 240, 60, 5000);
    check("tierra muy seca", RK_MOOD_THIRSTY, rk_mood_eval(&st, m, &t));
    rk_mood_state_init(&st);
    check_sev("tierra muy seca es urgente", RK_SEV_URGENT,
              rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(22, 240, 60, 5000);
    check("tierra apenas seca", RK_MOOD_THIRSTY, rk_mood_eval(&st, m, &t));
    rk_mood_state_init(&st);
    check_sev("tierra apenas seca es aviso", RK_SEV_WATCH,
              rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(80, 240, 60, 5000);
    check("encharcada", RK_MOOD_DROWNING, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(40, 100, 60, 5000);
    check("10 grados: frio", RK_MOOD_COLD, rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(40, 360, 60, 5000);
    check("36 grados: calor", RK_MOOD_HOT, rk_mood_eval(&st, m, &t));

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

    rk_mood_state_init(&st);
    t = tel(40, 240, 60, 5000);
    t.valid = false;
    check("telemetria invalida", RK_MOOD_OFFLINE, rk_mood_eval(&st, m, &t));

    check("especie NULL no explota", RK_MOOD_UNKNOWN,
          rk_mood_eval(&st, NULL, &t));
    check("telemetria NULL no explota", RK_MOOD_UNKNOWN,
          rk_mood_eval(&st, m, NULL));

    /* ---- prioridad: el agua le gana al resto --------------------------- */
    rk_mood_state_init(&st);
    t = tel(15, 100, 30, 200);
    check("seca + fria + oscura gana el agua", RK_MOOD_THIRSTY,
          rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(40, 100, 30, 200);
    check("sin problema de agua gana la temperatura", RK_MOOD_COLD,
          rk_mood_eval(&st, m, &t));

    rk_mood_state_init(&st);
    t = tel(40, 240, 30, 200);
    check("sin agua ni temperatura gana la luz", RK_MOOD_DARK,
          rk_mood_eval(&st, m, &t));

    /* ---- noche: la oscuridad sostenida deja de ser queja ---------------- */
    rk_mood_state_init(&st);
    t = tel(40, 240, 60, 0);
    for (i = 0; i < MUESTRAS_NOCHE - 2; i++) {
        rk_mood_eval(&st, m, &t);
    }
    check("septima muestra a oscuras todavia se queja", RK_MOOD_DARK,
          rk_mood_eval(&st, m, &t));
    check("octava muestra a oscuras ya duerme", RK_MOOD_SLEEPING,
          rk_mood_eval(&st, m, &t));

    t = tel(15, 240, 60, 0);
    check("de noche pero sin agua gana el agua", RK_MOOD_THIRSTY,
          rk_mood_eval(&st, m, &t));

    /* Amanecer: una sola muestra con luz reinicia el contador de noche. */
    rk_mood_state_init(&st);
    t = tel(40, 240, 60, 0);
    for (i = 0; i < MUESTRAS_NOCHE + 2; i++) {
        rk_mood_eval(&st, m, &t);
    }
    t = tel(40, 240, 60, 5000);
    check("al amanecer vuelve a estar contenta", RK_MOOD_HAPPY,
          rk_mood_eval(&st, m, &t));

    /* ---- histéresis: no titila en el borde ------------------------------ */
    rk_mood_state_init(&st);
    t = tel(24, 240, 60, 5000);
    check("24% arranca sediento", RK_MOOD_THIRSTY, rk_mood_eval(&st, m, &t));
    t = tel(27, 240, 60, 5000);
    check("27% sigue sediento por la banda", RK_MOOD_THIRSTY,
          rk_mood_eval(&st, m, &t));
    t = tel(32, 240, 60, 5000);
    check("32% ya sale del estado", RK_MOOD_HAPPY, rk_mood_eval(&st, m, &t));

    /* El mismo mecanismo del otro lado, en temperatura. */
    rk_mood_state_init(&st);
    t = tel(40, 175, 60, 5000);
    check("17,5 grados entra en frio", RK_MOOD_COLD, rk_mood_eval(&st, m, &t));
    t = tel(40, 188, 60, 5000);
    check("18,8 grados sigue en frio por la banda", RK_MOOD_COLD,
          rk_mood_eval(&st, m, &t));
    t = tel(40, 210, 60, 5000);
    check("21 grados ya sale", RK_MOOD_HAPPY, rk_mood_eval(&st, m, &t));

    RK_SUITE_END();
}
