#include "brote.h"
#include "adulto.h"
#include "look.h"
#include <stddef.h>

/* Hojas por etapa. ESPORA no tiene ninguna —todavía no salió del cascarón—
 * y de ahí en adelante sale una por etapa hasta cuatro. */
static const int HOJAS[RK_ETAPA_COUNT] = { 0, 1, 2, 3, 4 };

int rk_brote_hojas(rk_stage_t etapa)
{
    if ((int)etapa < 0 || etapa >= RK_ETAPA_COUNT) {
        return 0;
    }
    return HOJAS[etapa];
}

bool rk_brote_aura(rk_stage_t etapa)
{
    return etapa >= RK_ETAPA_MADURO;
}

/* La pala de una hoja: un rombo. A 32x32 un rombo se lee como hoja y un
 * disco se lee como pelota, y la diferencia son cuatro pixeles. */
static void pala(rk_fb_t *fb, int cx, int cy, int r,
                 rk_color_t carne, rk_color_t punta)
{
    int j;
    for (j = -r; j <= r; j++) {
        int w = r - (j < 0 ? -j : j);
        rk_hline(fb, cx - w, cy + j, w * 2 + 1, carne);
    }
    rk_px(fb, cx, cy - r, punta);
}

/* Una hoja de crecimiento: tallo desde el cuerpo y pala en la punta.
 *
 * Van por FUERA de la silueta a propósito. Una hoja pegada al cuerpo se
 * confunde con el caparazón y el crecimiento deja de verse; saliendo del
 * contorno, cada hoja nueva ensancha la silueta, que es lo único que se
 * percibe desde un metro de distancia. */
static void hoja(rk_fb_t *fb, int x0, int y0, int x1, int y1, int esc,
                 rk_color_t tallo, rk_color_t carne, rk_color_t punta)
{
    int i, dx = x1 - x0, dy = y1 - y0;
    int pasos = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);

    if (pasos <= 0) {
        return;
    }
    for (i = 0; i <= pasos; i++) {
        rk_fill_rect(fb, x0 + dx * i / pasos, y0 + dy * i / pasos,
                     esc, esc, tallo);
    }
    pala(fb, x1, y1, 2 * esc + 1, carne, punta);
}

/* El cascarón de la etapa ESPORA: el brote todavía adentro, sólo los ojos
 * asomando por encima del borde. Es la única etapa que tapa al bicho, y eso
 * es deliberado —da algo que esperar durante los primeros siete días sanos. */
static void cascaron(rk_fb_t *fb, int cx, int cy, int esc,
                     const rk_color_t *pal)
{
    int w = RK_BROTE_W * esc * 3 / 4;
    int h = RK_BROTE_H * esc / 2;
    int x = cx - w / 2;
    int y = cy + RK_BROTE_H * esc / 2 - h;
    int i;

    rk_fill_round(fb, x, y, w, h, esc * 3, pal[2]);
    rk_fill_round(fb, x + esc, y + esc, w - esc * 2, h - esc * 2, esc * 3, pal[3]);
    /* La grieta: tres trazos en zigzag por el frente. */
    for (i = 0; i < 4; i++) {
        int gx = x + w / 4 + (i % 2) * esc * 2;
        rk_fill_rect(fb, gx, y + esc * 2 + i * esc * 2, esc * 3, esc, pal[1]);
    }
}

void rk_brote_draw(rk_fb_t *fb, int cx, int cy, const rk_companion_t *comp,
                   rk_mood_t mood, rk_severity_t sev, rk_stage_t etapa,
                   int esc, uint32_t t_ms)
{
    const rk_look_t  *lk  = rk_look(mood);
    const rk_color_t *pal = rk_companion_pal(comp);
    const rk_shape_t *cuerpo_sh, *ojo, *boca;
    rk_sprite_t cuerpo, s_ojo, s_boca;
    int idx, bx, by, bob, jitter, hx, hy, n, i;
    uint8_t tint_amt;

    if (fb == NULL || esc < 1) {
        return;
    }
    idx = rk_companion_index(comp);
    if (idx < 0 || idx >= RK_PAL_COUNT) {
        idx = 0;
    }
    if ((int)etapa < 0 || etapa >= RK_ETAPA_COUNT) {
        etapa = RK_ETAPA_ESPORA;
    }
    cuerpo_sh = &rk_brote_body[idx];

    bob = lk->bob_amp
        ? rk_sin8((uint8_t)(t_ms * lk->bob_speed / 1000)) * lk->bob_amp / 127
        : 0;
    jitter = lk->shiver ? (((t_ms / 60) % 2) ? lk->shiver : -lk->shiver) : 0;

    bx = cx - RK_BROTE_W * esc / 2 + jitter * esc;
    by = cy - RK_BROTE_H * esc / 2 + bob * esc;

    /* ---- aura de las etapas altas, detrás de todo ----------------------- */
    /* Los discos se pintan de mayor a menor y cada uno tapa al anterior, así
     * que para que el resplandor se apague HACIA AFUERA la mezcla tiene que
     * crecer a medida que el radio baja. Al revés queda un anillo brillante
     * con el centro apagado, que se lee como dona y no como aura. */
    if (rk_brote_aura(etapa)) {
        int r0 = RK_BROTE_W * esc / 2 + esc * 3;
        int pulso = rk_sin8((uint8_t)(t_ms / 14)) * esc / 127;
        int pasos = esc * 4;
        for (i = 0; i < pasos; i++) {
            int r = r0 + pulso - i;
            uint8_t t = (uint8_t)(4 + i * 44 / pasos);
            if (r <= 0) {
                break;
            }
            rk_disc(fb, cx, cy, r, rk_mix(RK_RGB(8, 10, 12), pal[6], t));
        }
    }

    /* ---- hojas de crecimiento, detrás del cuerpo ------------------------ */
    /* Coordenadas en pixeles de ARTE respecto del centro del bicho, para que
     * el abanico sea el mismo a escala 1 (la ficha del Prime) y a escala 2
     * (la maceta). Alternan lado y suben de a pares: cuatro hojas forman un
     * abanico y no un arbusto. */
    {
        static const int8_t HOJA[4][4] = {
            /* base x, base y, punta x, punta y */
            { -11,  10, -19,   1 },
            {  11,  10,  19,   1 },
            { -12,   4, -22,  -8 },
            {  12,   4,  22,  -8 },
        };
        n = rk_brote_hojas(etapa);
        for (i = 0; i < n && i < 4; i++) {
            int mece = rk_sin8((uint8_t)(t_ms / 20 + i * 50)) * esc * 2 / 127;
            hoja(fb,
                 cx + HOJA[i][0] * esc, cy + HOJA[i][1] * esc,
                 cx + HOJA[i][2] * esc + mece, cy + HOJA[i][3] * esc,
                 esc, pal[7], pal[8], pal[6]);
        }
    }

    /* ---- el cuerpo ------------------------------------------------------ */
    tint_amt = lk->tint_amt;
    if (sev == RK_SEV_URGENT) {
        int pulse = rk_sin8((uint8_t)(t_ms / 4));
        int amt   = (int)tint_amt + pulse * 45 / 127;
        if (amt < 0)   { amt = 0; }
        if (amt > 255) { amt = 255; }
        tint_amt = (uint8_t)amt;
    }

    cuerpo = rk_shape_sprite(cuerpo_sh, pal);
    if (lk->dim > 0) {
        rk_blit_scaled_tint(fb, &cuerpo, bx, by, esc,
                            tint_amt > 0 ? lk->tint : RK_RGB(8, 10, 18),
                            tint_amt > lk->dim ? tint_amt : lk->dim);
    } else {
        rk_blit_scaled_tint(fb, &cuerpo, bx, by, esc, lk->tint, tint_amt);
    }

    /* ---- la cara -------------------------------------------------------- */
    ojo  = RK_BR_OJO[rk_look_parpadea(lk, t_ms) ? RK_OJO_BLINK : lk->ojo];
    boca = RK_BR_BOCA[lk->boca];
    s_ojo  = rk_shape_sprite(ojo, pal);
    s_boca = rk_shape_sprite(boca, pal);

    hx = bx + RK_BR_HEAD_CX * esc;
    hy = by + RK_BR_HEAD_CY * esc;

    {
        int ey = hy - (RK_BR_EYE_DY + ojo->h / 2) * esc;
        int my = hy + (RK_BR_MOUTH_DY - boca->h / 2) * esc;
        rk_color_t d = RK_RGB(8, 10, 18);
        uint8_t    a = lk->dim;
        rk_blit_scaled_tint(fb, &s_ojo,
                            hx - (RK_BR_EYE_DX + ojo->w / 2) * esc, ey, esc, d, a);
        rk_blit_scaled_tint(fb, &s_ojo,
                            hx + (RK_BR_EYE_DX - ojo->w / 2) * esc, ey, esc, d, a);
        /* La boca sólo en la mitad inferior de la cara, y sólo si la etapa ya
         * la sacó del cascarón. En ESPORA la expresión es toda ojos. */
        if (etapa > RK_ETAPA_ESPORA) {
            rk_blit_scaled_tint(fb, &s_boca,
                                hx - boca->w / 2 * esc, my, esc, d, a);
        }
    }

    /* ---- el cascarón, adelante, sólo en la primera etapa ---------------- */
    if (etapa == RK_ETAPA_ESPORA) {
        cascaron(fb, cx, cy, esc, pal);
    }
}
