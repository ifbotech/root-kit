#include "golden_util.h"
#include "../core/species.h"

static rk_color_t g_prime[RK_PRIME_PX];
static rk_color_t g_mini[RK_MINI_PX];

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

void rk_golden_kit(rk_roster_t *r, int nodo, rk_mood_t mood)
{
    static const struct {
        const char *nom; const char *sp; rk_role_t rol;
        uint8_t soil; int16_t temp; uint8_t rh; uint32_t lux; int dias;
    } SEED[4] = {
        { "MONSTERA", "monstera", RK_ROLE_PRIME, 38, 232, 55,  4800,  95 },
        { "POTUS",    "pothos",   RK_ROLE_MINI,  51, 241, 48,  7900,  34 },
        { "CACTUS",   "cactus",   RK_ROLE_MINI,  14, 250, 41, 11000,   9 },
        { "BONSAI",   "bonsai",   RK_ROLE_MINI,  42, 228, 60,  6100, 200 },
    };
    int i, d;

    rk_roster_init(r);
    r->wifi = true;

    for (i = 0; i < 4; i++) {
        int k = rk_roster_add(r, SEED[i].nom, SEED[i].rol,
                              rk_species_find(SEED[i].sp));
        rk_node_t *n = &r->nodes[k];
        for (d = 0; d < SEED[i].dias; d++) {
            rk_bond_dia(&n->bond, true);
        }
        n->tel.valid    = true;
        n->tel.age_s    = 60u;
        n->tel.soil_pct = SEED[i].soil;
        n->tel.temp_dc  = SEED[i].temp;
        n->tel.rh_pct   = SEED[i].rh;
        n->tel.lux      = SEED[i].lux;
        n->tel.batt_mv  = (uint16_t)(3880 - i * 90);

        n->verdict.mood     = (i == nodo) ? mood : RK_MOOD_HAPPY;
        n->verdict.severity = (i == nodo && mood == RK_MOOD_THIRSTY)
                              ? RK_SEV_URGENT : RK_SEV_WATCH;
        n->verdict.reason   = rk_mood_reason(n->verdict.mood);
    }
    r->selected = (nodo >= 0 && nodo < r->count) ? nodo : 0;
}

uint32_t rk_golden_prime(rk_mood_t mood, uint32_t t_ms)
{
    rk_roster_t kit;
    rk_fb_t     fb;

    rk_fb_init(&fb, g_prime, RK_PRIME_W, RK_PRIME_H);
    rk_golden_kit(&kit, 0, mood);
    rk_prime_draw(&fb, &kit, t_ms);
    return rk_frame_hash(g_prime, RK_PRIME_PX);
}

uint32_t rk_golden_mini(rk_mood_t mood, uint32_t t_ms)
{
    rk_roster_t kit;
    rk_fb_t     fb;

    rk_fb_init(&fb, g_mini, RK_MINI_W, RK_MINI_H);
    rk_golden_kit(&kit, 1, mood);
    rk_mini_draw(&fb, &kit.nodes[1], t_ms);
    return rk_frame_hash(g_mini, RK_MINI_PX);
}
