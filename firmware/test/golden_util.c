#include "golden_util.h"
#include "../core/species.h"
#include <string.h>
#include <stdio.h>

static rk_color_t g_px[RK_MINI_PX];

uint32_t rk_frame_hash(const rk_color_t *px, int n)
{
    uint32_t h = 2166136261u;      /* FNV-1a de 32 bits */
    int i;

    for (i = 0; i < n; i++) {
        h ^= (uint32_t)(px[i] & 0xFFu);
        h *= 16777619u;
        h ^= (uint32_t)((px[i] >> 8) & 0xFFu);
        h *= 16777619u;
    }
    return h;
}

void rk_golden_nodo(rk_node_t *n, int persona, rk_mood_t mood)
{
    int d;

    memset(n, 0, sizeof *n);
    snprintf(n->nombre, sizeof n->nombre, "MACETA");
    n->id[0]   = 0x52u;
    n->id[5]   = (uint8_t)persona;
    n->sp      = rk_species_find("monstera");
    n->persona = rk_persona_at(persona);

    rk_mood_state_init(&n->mst);
    rk_bond_init(&n->bond);
    /* 34 días sanos: etapa JOVEN. Se eligió una etapa intermedia a propósito
     * para que el escenario ejercite los adornos de crecimiento sin estar en
     * el extremo donde aparecen todos. */
    for (d = 0; d < 34; d++) {
        rk_bond_dia(&n->bond, true);
    }

    n->tel.valid    = true;
    n->tel.age_s    = 60u;
    n->tel.soil_pct = 38;
    n->tel.temp_dc  = 232;
    n->tel.rh_pct   = 55;
    n->tel.lux      = 4800;
    n->tel.batt_mv  = 3880;

    n->verdict.mood     = mood;
    n->verdict.severity = (mood == RK_MOOD_THIRSTY) ? RK_SEV_URGENT
                        : (mood == RK_MOOD_HAPPY || mood == RK_MOOD_SLEEPING)
                          ? RK_SEV_OK : RK_SEV_WATCH;
    n->verdict.reason   = rk_mood_reason(mood);
}

uint32_t rk_golden_cara(int persona, rk_mood_t mood, uint32_t t_ms)
{
    rk_node_t n;
    rk_fb_t   fb;

    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);
    rk_golden_nodo(&n, persona, mood);
    rk_cara_draw(&fb, &n, t_ms);
    return rk_frame_hash(g_px, RK_MINI_PX);
}
