#include "adulto.h"
#include "look.h"
#include <stddef.h>

#define COL_AGUA      RK_RGB(120, 200, 245)
#define COL_NIEVE     RK_RGB(225, 240, 255)
#define COL_SUDOR     RK_RGB(150, 210, 245)
#define COL_POLVO     RK_RGB(198, 176, 132)
#define COL_GLITCH    RK_RGB( 72, 214, 190)
#define COL_ZZZ       RK_RGB(170, 200, 230)

const rk_color_t *rk_companion_pal(const rk_companion_t *comp)
{
    int i = rk_companion_index(comp);
    if (i < 0 || i >= RK_PAL_COUNT) {
        i = 0;
    }
    return RK_PAL[i];
}

/* ------------------------------------------------------- escenografía ---- */
void rk_escena_colores(rk_mood_t mood, rk_color_t *top, rk_color_t *bottom)
{
    switch (mood) {
    case RK_MOOD_SLEEPING:
        *top = RK_RGB(14, 18, 40);  *bottom = RK_RGB(30, 34, 62);  break;
    case RK_MOOD_DARK:
        *top = RK_RGB(16, 20, 26);  *bottom = RK_RGB(28, 34, 40);  break;
    case RK_MOOD_DROWNING:
        *top = RK_RGB(18, 52, 92);  *bottom = RK_RGB(30, 92, 140); break;
    case RK_MOOD_COLD:
        *top = RK_RGB(32, 52, 86);  *bottom = RK_RGB(66, 96, 134); break;
    case RK_MOOD_HOT:
        *top = RK_RGB(96, 44, 30);  *bottom = RK_RGB(158, 82, 44); break;
    case RK_MOOD_SCORCHED:
        *top = RK_RGB(150, 96, 30); *bottom = RK_RGB(206, 158, 66); break;
    case RK_MOOD_THIRSTY:
        *top = RK_RGB(72, 56, 32);  *bottom = RK_RGB(116, 94, 54); break;
    case RK_MOOD_PARCHED_AIR:
        *top = RK_RGB(66, 62, 44);  *bottom = RK_RGB(112, 104, 72); break;
    case RK_MOOD_OFFLINE:
        *top = RK_RGB(20, 22, 24);  *bottom = RK_RGB(38, 42, 44);  break;
    default: /* HAPPY y el resto: invernadero */
        *top = RK_RGB(22, 54, 44);  *bottom = RK_RGB(52, 104, 74); break;
    }
}

/* ------------------------------------------------------------ efectos ---- */
/* Los efectos escalan con el tamaño de la banda que reciben, así que la
 * misma función sirve para la escena de 240x176 del Prime y para la de
 * 128x88 del Mini sin una sola constante duplicada. */
static void fx_particles(rk_fb_t *fb, int x, int y, int w, int h,
                         uint32_t t_ms, int n, int speed, bool up,
                         rk_color_t c, int size)
{
    int i;
    for (i = 0; i < n; i++) {
        uint16_t r  = rk_hash((uint16_t)(i * 2654 + 17));
        int      px = x + (int)(r % (unsigned)(w > 0 ? w : 1));
        int      travel = (int)((t_ms / 8 * speed / 10 + (r >> 5) % 512) % 512);
        int      py = up ? (y + h - travel * h / 512)
                         : (y + travel * h / 512);
        /* deriva lateral suave, distinta para cada partícula */
        px += rk_sin8((uint8_t)(t_ms / 12 + i * 31)) * 3 / 127;
        if (py < y || py >= y + h) {
            continue;
        }
        if (size <= 1) {
            rk_px(fb, px, py, c);
        } else {
            rk_fill_rect(fb, px, py, size, size, c);
        }
    }
}

static void fx_glitch(rk_fb_t *fb, int x, int y, int w, int h, uint32_t t_ms)
{
    int i;
    for (i = 0; i < 6; i++) {
        uint16_t r  = rk_hash((uint16_t)(i * 911 + t_ms / 120));
        int      by = y + (int)(r % (unsigned)(h > 0 ? h : 1));
        int      bh = 1 + (int)((r >> 4) % 3);
        rk_fill_rect(fb, x, by, w, bh, rk_mix(COL_GLITCH, 0x0000, 150));
        rk_hline(fb, x, by, w, COL_GLITCH);
    }
}

static void fx_rays(rk_fb_t *fb, int x, int y, int w, int h, uint32_t t_ms)
{
    int i;
    for (i = 0; i < 7; i++) {
        int rx = x + w * i / 7 + rk_sin8((uint8_t)(t_ms / 24 + i * 36)) * 4 / 127;
        rk_color_t c = rk_mix(RK_RGB(255, 230, 150), RK_RGB(206, 158, 66), 120);
        int j;
        for (j = 0; j < h; j += 3) {
            rk_px(fb, rx + j / 4, y + j, c);
            rk_px(fb, rx + j / 4, y + j + 1, c);
        }
    }
}

static void fx_zzz(rk_fb_t *fb, int cx, int cy, int esc, uint32_t t_ms)
{
    int i;
    for (i = 0; i < 3; i++) {
        uint32_t ph = (t_ms / 10 + (uint32_t)i * 110) % 330;
        int  ry  = cy - (int)ph / 9 * esc / 2;
        int  rx  = cx + 16 * esc / 2 + (int)ph / 22;
        int  sz  = (1 + (int)ph / 160) * esc / 2;
        if (ph > 300 || sz < 1) {
            continue;
        }
        /* una Z dibujada a mano, que escala mejor que la fuente a este tamaño */
        rk_hline(fb, rx, ry, 4 * sz, COL_ZZZ);
        rk_hline(fb, rx, ry + 3 * sz, 4 * sz, COL_ZZZ);
        rk_px(fb, rx + 2 * sz, ry + sz, COL_ZZZ);
        rk_px(fb, rx + sz, ry + 2 * sz, COL_ZZZ);
    }
}

void rk_adulto_fx_back(rk_fb_t *fb, int x, int y, int w, int h,
                       rk_mood_t mood, uint32_t t_ms)
{
    switch (mood) {
    case RK_MOOD_SCORCHED: fx_rays(fb, x, y, w, h, t_ms); break;
    case RK_MOOD_DARK:
        /* pocas motas de polvo flotando: hace que el vacío no parezca un bug */
        fx_particles(fb, x, y, w, h, t_ms, 10, 2, true,
                     rk_mix(COL_POLVO, 0x0000, 170), 1);
        break;
    default: break;
    }
}

void rk_adulto_fx_front(rk_fb_t *fb, int x, int y, int w, int h,
                        rk_mood_t mood, uint32_t t_ms)
{
    /* La densidad de partículas se escala con el área: la escena del Prime
     * es cuatro veces la del Mini, y con un número fijo la lluvia del Mini
     * se vería como una tormenta. */
    int dens = (w * h) / 6000;
    if (dens < 1) { dens = 1; }
    if (dens > 4) { dens = 4; }

    switch (mood) {
    case RK_MOOD_DROWNING:
        fx_particles(fb, x, y, w, h, t_ms, 6 * dens, 14, true, COL_AGUA,
                     dens > 2 ? 2 : 1);
        break;
    case RK_MOOD_COLD:
        fx_particles(fb, x, y, w, h, t_ms, 7 * dens, 7, false, COL_NIEVE, 1);
        break;
    case RK_MOOD_HOT:
        fx_particles(fb, x + w / 4, y, w / 2, h / 2, t_ms, 2 * dens, 20, false,
                     COL_SUDOR, dens > 2 ? 2 : 1);
        break;
    case RK_MOOD_PARCHED_AIR:
        fx_particles(fb, x, y, w, h, t_ms, 4 * dens, 4, false, COL_POLVO, 1);
        break;
    case RK_MOOD_SLEEPING:
        fx_zzz(fb, x + w / 2, y + h / 2, dens > 2 ? 2 : 1, t_ms);
        break;
    case RK_MOOD_OFFLINE:
        fx_glitch(fb, x, y, w, h, t_ms);
        break;
    default: break;
    }
}

/* ------------------------------------------------------------- el bicho -- */
void rk_adulto_draw(rk_fb_t *fb, int cx, int cy, const rk_companion_t *comp,
                    rk_mood_t mood, rk_severity_t sev, int esc, uint32_t t_ms)
{
    const rk_look_t  *lk  = rk_look(mood);
    const rk_color_t *pal = rk_companion_pal(comp);
    const rk_shape_t *ojo, *boca;
    rk_sprite_t cuerpo, s_ojo, s_boca;
    int bx, by, bob, jitter, hx, hy;
    uint8_t tint_amt;

    if (fb == NULL || esc < 1) {
        return;
    }

    /* Respiración: una senoidal lenta sobre el eje Y. Es el detalle más
     * barato que separa "un dibujo" de "algo vivo". */
    bob = lk->bob_amp
        ? rk_sin8((uint8_t)(t_ms * lk->bob_speed / 1000)) * lk->bob_amp / 127
        : 0;
    jitter = lk->shiver ? (((t_ms / 60) % 2) ? lk->shiver : -lk->shiver) : 0;

    bx = cx - RK_ADULTO_W * esc / 2 + jitter * esc;
    by = cy - RK_ADULTO_H * esc / 2 + bob * esc;

    /* Sombra en el piso: deliberadamente NO sigue al bob, y eso es lo que
     * hace leer la respiración como altura en vez de como deslizamiento. */
    {
        int i;
        for (i = 0; i < 3 * esc; i++) {
            rk_hline(fb, cx - 20 * esc + i * 3, cy + RK_ADULTO_H * esc / 2 - 3 * esc + i,
                     40 * esc - i * 6, RK_RGB(16, 22, 18));
        }
    }

    /* Una alerta urgente hace latir el tinte: imposible de ignorar de reojo,
     * pero sin ser un parpadeo agresivo. */
    tint_amt = lk->tint_amt;
    if (sev == RK_SEV_URGENT) {
        int pulse = rk_sin8((uint8_t)(t_ms / 4));
        int amt   = (int)tint_amt + pulse * 45 / 127;
        if (amt < 0)   { amt = 0; }
        if (amt > 255) { amt = 255; }
        tint_amt = (uint8_t)amt;
    }

    cuerpo = rk_shape_sprite(&rk_adulto_body, pal);
    if (lk->dim > 0) {
        /* En penumbra el simbionte se apaga con la escena. Si no, queda
         * flotando a pleno brillo sobre un fondo oscuro y rompe la ilusión. */
        rk_blit_scaled_tint(fb, &cuerpo, bx, by, esc,
                            tint_amt > 0 ? lk->tint : RK_RGB(8, 10, 18),
                            tint_amt > lk->dim ? tint_amt : lk->dim);
    } else {
        rk_blit_scaled_tint(fb, &cuerpo, bx, by, esc, lk->tint, tint_amt);
    }

    ojo  = RK_AD_OJO[rk_look_parpadea(lk, t_ms) ? RK_OJO_BLINK : lk->ojo];
    boca = RK_AD_BOCA[lk->boca];
    s_ojo  = rk_shape_sprite(ojo, pal);
    s_boca = rk_shape_sprite(boca, pal);

    hx = bx + RK_AD_HEAD_CX * esc;
    hy = by + RK_AD_HEAD_CY * esc;

    {
        int ey = hy - (RK_AD_EYE_DY + ojo->h / 2) * esc;
        int my = hy + (RK_AD_MOUTH_DY - boca->h / 2) * esc;
        rk_color_t d = RK_RGB(8, 10, 18);
        uint8_t    a = lk->dim;
        rk_blit_scaled_tint(fb, &s_ojo,
                            hx - (RK_AD_EYE_DX + ojo->w / 2) * esc, ey, esc, d, a);
        rk_blit_scaled_tint(fb, &s_ojo,
                            hx + (RK_AD_EYE_DX - ojo->w / 2) * esc, ey, esc, d, a);
        rk_blit_scaled_tint(fb, &s_boca,
                            hx - boca->w / 2 * esc, my, esc, d, a);
    }
}
