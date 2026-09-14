#include "golden_util.h"
#include "../core/species.h"
#include <string.h>
#include <stdio.h>

static rk_color_t g_px[RK_CANVAS_W * RK_CANVAS_H];

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

void rk_golden_state(rk_state_t *st, rk_mood_t mood)
{
    static const char *NOM[3]    = { "MONSTERA", "POTUS", "CACTUS" };
    static const char *SP[3]     = { "monstera", "pothos", "cactus" };
    static const uint8_t SOIL[3] = { 38, 51, 14 };
    int i;

    memset(st, 0, sizeof *st);
    st->count        = 3;
    st->selected     = 0;
    st->wifi         = true;
    st->hub_batt_pct = 64;

    for (i = 0; i < 3; i++) {
        rk_plant_t *p = &st->plants[i];
        snprintf(p->nombre, sizeof p->nombre, "%s", NOM[i]);
        p->sp = rk_species_find(SP[i]);
        rk_mood_state_init(&p->mst);
        p->tel.valid    = true;
        p->tel.age_s    = 60;
        p->tel.soil_pct = SOIL[i];
        p->tel.temp_dc  = (int16_t)(232 + i * 9);
        p->tel.rh_pct   = (uint8_t)(55 - i * 7);
        p->tel.lux      = (uint32_t)(4800 + i * 3100);
        p->tel.batt_mv  = 3880;
        p->verdict.mood     = (i == 0) ? mood : RK_MOOD_HAPPY;
        p->verdict.severity = (i == 0 && mood == RK_MOOD_THIRSTY)
                              ? RK_SEV_URGENT : RK_SEV_WATCH;
        p->verdict.reason   = rk_mood_reason(p->verdict.mood);
    }
}

uint32_t rk_golden_render(rk_mood_t mood, uint32_t t_ms)
{
    rk_state_t st;
    rk_fb_t    fb;

    rk_fb_init(&fb, g_px, RK_CANVAS_W, RK_CANVAS_H);
    rk_golden_state(&st, mood);
    rk_screen_draw(&fb, &st, t_ms);
    return rk_frame_hash(g_px, RK_CANVAS_W * RK_CANVAS_H);
}
