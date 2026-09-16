#include "revelado.h"
#include "../gfx/font.h"
#include "../art/face.h"
#include <stddef.h>

rk_rev_fase_t rk_revelado_fase(uint32_t t_ms)
{
    if (t_ms < RK_REV_NEGRO)   { return RK_REV_FASE_NEGRO; }
    if (t_ms < RK_REV_OJOS)    { return RK_REV_FASE_OJOS; }
    if (t_ms < RK_REV_RASGOS)  { return RK_REV_FASE_RASGOS; }
    return RK_REV_FASE_NOMBRE;
}

bool rk_revelado_termino(uint32_t t_ms)
{
    return t_ms >= RK_REV_NOMBRE + 1600u;
}

/* Destellos del color de la rareza. Son los únicos que quedaron de la
 * ceremonia vieja, y ahora dicen algo distinto: no "qué suerte tuviste" sino
 * "esta es la rareza de lo que ya tenés en la mano". */
static void destellos(rk_fb_t *fb, int cx, int cy, int n, int avance,
                      rk_color_t tono)
{
    int i;

    for (i = 0; i < n; i++) {
        uint16_t h   = rk_hash((uint16_t)(i * 1597u + 7u));
        int      ang = (int)(h % 256u);
        int      vel = 60 + (int)((h >> 4) % 70u);
        int      d   = avance * vel / 100;
        int      x   = cx + rk_sin8((uint8_t)(ang + 64)) * d / 127;
        int      y   = cy + rk_sin8((uint8_t)ang) * d / 127;
        int      sz  = (avance < 130) ? 3 : (avance < 170 ? 2 : 1);
        uint8_t  fade = (avance < 130) ? 0u
                        : (uint8_t)((avance - 130) * 255 / 70);

        rk_fill_rect(fb, x - sz / 2, y - sz / 2, sz, sz,
                     rk_mix(tono, RK_RGB(6, 8, 10), fade));
    }
}

void rk_revelado_draw(rk_fb_t *fb, const rk_persona_t *p, uint32_t t_ms)
{
    rk_color_t tono;
    int cx, cy, W, H;
    uint8_t oscuro;

    if (fb == NULL) {
        return;
    }
    if (p == NULL) {
        p = rk_persona_at(0);
    }
    W = fb->w;
    H = fb->h;
    cx = W / 2;
    cy = H * 44 / 100;
    tono = rk_rarity_color(p->rareza);

    if (t_ms < RK_REV_NEGRO) {
        /* Oscuridad con un latido: el aparato despertando. */
        int pulso = rk_sin8((uint8_t)(t_ms / 3));
        rk_fb_clear(fb, rk_mix(RK_RGB(4, 5, 6), p->fondo,
                               (uint8_t)(20 + pulso / 6)));
        return;
    }

    /* El fondo de la persona entra desde el negro. */
    oscuro = (t_ms < RK_REV_OJOS)
             ? (uint8_t)(255u - (t_ms - RK_REV_NEGRO) * 255u /
                         (RK_REV_OJOS - RK_REV_NEGRO))
             : 0u;
    rk_vgradient(fb, 0, 0, W, H,
                 rk_dim(p->fondo, oscuro), rk_dim(p->fondo2, oscuro));

    if (t_ms < RK_REV_OJOS) {
        /* Sólo los ojos, abriéndose. Se dibuja la cara completa y se tapa
         * todo salvo la banda de los ojos: así el rig es uno solo y no hay
         * una segunda implementación de "ojos" que se pueda desincronizar. */
        int banda = H * 18 / 100;
        int y0 = cy - banda / 2;
        uint32_t p01 = (t_ms - RK_REV_NEGRO) * 100u
                     / (RK_REV_OJOS - RK_REV_NEGRO);
        int abre = (int)(banda * p01 / 100);

        rk_face_rasgos(fb, cx, cy, W, p, RK_MOOD_SLEEPING, 0u, t_ms);
        rk_fill_rect(fb, 0, 0, W, y0 + (banda - abre) / 2,
                     rk_dim(p->fondo, oscuro));
        rk_fill_rect(fb, 0, y0 + (banda + abre) / 2, W, H,
                     rk_dim(p->fondo2, oscuro));
        return;
    }

    if (t_ms < RK_REV_RASGOS) {
        /* Los rasgos aparecen y los destellos salen desde el centro. */
        uint32_t p01 = (t_ms - RK_REV_OJOS) * 100u
                     / (RK_REV_RASGOS - RK_REV_OJOS);
        destellos(fb, cx, cy, rk_rarity_sparkles(p->rareza),
                  (int)p01 * 2, tono);
        rk_face_rasgos(fb, cx, cy, W, p, RK_MOOD_HAPPY, 0u, t_ms);
        return;
    }

    /* Reposo: la cara despierta y su nombre. */
    rk_face_rasgos(fb, cx, cy, W, p, RK_MOOD_HAPPY, 0u, t_ms);

    {
        const char *rn = rk_rarity_name(p->rareza);
        int yr = H * 74 / 100;
        int w  = rk_text_w(rn, 1) + 12;

        rk_fill_rect(fb, cx - w / 2, yr, w, 13, rk_mix(p->fondo, tono, 45));
        rk_rect(fb, cx - w / 2, yr, w, 13, tono);
        rk_text_center(fb, cx, yr + 3, rn, tono, 1);

        rk_text_center(fb, cx, yr + 20, p->nombre,
                       RK_RGB(240, 244, 238), (W >= 200) ? 3 : 2);

        if (rk_revelado_termino(t_ms) && ((t_ms / 600u) % 2u) == 0u) {
            rk_text_center(fb, cx, H - 14, "TOCA PARA SEGUIR",
                           rk_mix(tono, p->fondo, 120), 1);
        }
    }
}
