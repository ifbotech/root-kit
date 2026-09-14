#include "tuga.h"
#include "tuga_data.h"
#include <stddef.h>

/* ------------------------------------------------------------ tintes ----- */
#define TINT_FRIO     RK_RGB( 90, 150, 235)
#define TINT_CALOR    RK_RGB(240, 110,  70)
#define TINT_SOL      RK_RGB(255, 210, 120)
#define TINT_SED      RK_RGB(190, 150,  90)
#define TINT_AGUA     RK_RGB( 60, 120, 210)
#define TINT_MUERTO   RK_RGB(110, 120, 130)

#define COL_AGUA      RK_RGB(120, 200, 245)
#define COL_NIEVE     RK_RGB(225, 240, 255)
#define COL_SUDOR     RK_RGB(150, 210, 245)
#define COL_POLVO     RK_RGB(198, 176, 132)
#define COL_GLITCH    RK_RGB( 72, 214, 190)
#define COL_ZZZ       RK_RGB(170, 200, 230)

/* Cómo se ve la cara en cada estado. Toda la personalidad del bicho está
 * concentrada en esta tabla, que es justamente la idea: agregar un ánimo
 * nuevo es agregar una fila, no escribir código. */
typedef struct {
    const rk_sprite_t *const *eye;
    const rk_sprite_t *const *mouth;
    rk_color_t tint;
    uint8_t    tint_amt;
    uint8_t    bob_amp;     /* amplitud de la respiración, en pixeles      */
    uint8_t    bob_speed;   /* fase por segundo                            */
    uint8_t    shiver;      /* jitter horizontal (tiritar)                 */
    uint8_t    dim;         /* penumbra: el bicho también se apaga          */
    bool       blinks;
} look_t;

static const rk_sprite_t *const E_OPEN   = &rk_eye_open;
static const rk_sprite_t *const E_BLINK  = &rk_eye_blink;
static const rk_sprite_t *const E_HAPPY  = &rk_eye_happy;
static const rk_sprite_t *const E_WIDE   = &rk_eye_wide;
static const rk_sprite_t *const E_SLEEPY = &rk_eye_sleepy;
static const rk_sprite_t *const E_DEAD   = &rk_eye_dead;
static const rk_sprite_t *const E_DIZZY  = &rk_eye_dizzy;
static const rk_sprite_t *const E_GLITCH = &rk_eye_glitch;

static const rk_sprite_t *const M_SMILE = &rk_mouth_smile;
static const rk_sprite_t *const M_FLAT  = &rk_mouth_flat;
static const rk_sprite_t *const M_FROWN = &rk_mouth_frown;
static const rk_sprite_t *const M_OPEN  = &rk_mouth_open;
static const rk_sprite_t *const M_WAVY  = &rk_mouth_wavy;
static const rk_sprite_t *const M_PANT  = &rk_mouth_pant;

static const look_t LOOKS[RK_MOOD_COUNT] = {
/* UNKNOWN     */ { &E_DIZZY,  &M_FLAT,  0,            0,   1, 30, 0,   0, false },
/* OFFLINE     */ { &E_GLITCH, &M_FLAT,  TINT_MUERTO, 120,  0,  0, 0,  60, false },
/* SLEEPING    */ { &E_BLINK,  &M_FLAT,  0,             0,  3, 14, 0,  95, false },
/* HAPPY       */ { &E_HAPPY,  &M_SMILE, 0,             0,  2, 42, 0,   0, true  },
/* THIRSTY     */ { &E_SLEEPY, &M_FROWN, TINT_SED,     70,  1, 22, 0,   0, true  },
/* DROWNING    */ { &E_WIDE,   &M_OPEN,  TINT_AGUA,    85,  3, 64, 0,  25, false },
/* COLD        */ { &E_SLEEPY, &M_WAVY,  TINT_FRIO,    90,  1, 26, 1,   0, false },
/* HOT         */ { &E_SLEEPY, &M_PANT,  TINT_CALOR,   75,  2, 70, 0,   0, false },
/* SCORCHED    */ { &E_DEAD,   &M_FROWN, TINT_SOL,    100,  1, 18, 0,   0, false },
/* DARK        */ { &E_WIDE,   &M_FLAT,  0,             0,  1, 20, 0, 110, true  },
/* PARCHED_AIR */ { &E_OPEN,   &M_WAVY,  TINT_SED,     40,  2, 34, 0,   0, true  },
};

/* ------------------------------------------------------- escenografía ---- */
void rk_tuga_scene_colors(rk_mood_t mood, rk_color_t *top, rk_color_t *bottom)
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

static void fx_zzz(rk_fb_t *fb, int cx, int cy, uint32_t t_ms)
{
    int i;
    for (i = 0; i < 3; i++) {
        uint32_t ph = (t_ms / 10 + (uint32_t)i * 110) % 330;
        int  ry  = cy - (int)ph / 9;
        int  rx  = cx + 16 + (int)ph / 22;
        int  sz  = 1 + (int)ph / 160;
        if (ph > 300) {
            continue;
        }
        /* una Z dibujada a mano, que escala mejor que la fuente a este tamaño */
        rk_hline(fb, rx, ry, 4 * sz, COL_ZZZ);
        rk_hline(fb, rx, ry + 3 * sz, 4 * sz, COL_ZZZ);
        rk_px(fb, rx + 2 * sz, ry + sz, COL_ZZZ);
        rk_px(fb, rx + sz, ry + 2 * sz, COL_ZZZ);
    }
}

void rk_tuga_fx_back(rk_fb_t *fb, int x, int y, int w, int h,
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

void rk_tuga_fx_front(rk_fb_t *fb, int x, int y, int w, int h,
                      rk_mood_t mood, uint32_t t_ms)
{
    switch (mood) {
    case RK_MOOD_DROWNING:
        fx_particles(fb, x, y, w, h, t_ms, 22, 14, true, COL_AGUA, 2);
        break;
    case RK_MOOD_COLD:
        fx_particles(fb, x, y, w, h, t_ms, 26, 7, false, COL_NIEVE, 1);
        break;
    case RK_MOOD_HOT:
        fx_particles(fb, x + w / 4, y, w / 2, h / 2, t_ms, 6, 20, false,
                     COL_SUDOR, 2);
        break;
    case RK_MOOD_PARCHED_AIR:
        fx_particles(fb, x, y, w, h, t_ms, 14, 4, false, COL_POLVO, 1);
        break;
    case RK_MOOD_SLEEPING:
        fx_zzz(fb, x + w / 2, y + h / 2, t_ms);
        break;
    case RK_MOOD_OFFLINE:
        fx_glitch(fb, x, y, w, h, t_ms);
        break;
    default: break;
    }
}

/* ------------------------------------------------------------- el bicho -- */
void rk_tuga_draw(rk_fb_t *fb, int cx, int cy,
                  rk_mood_t mood, rk_severity_t sev, uint32_t t_ms)
{
    const look_t *lk;
    const rk_sprite_t *eye, *mouth;
    int bx, by, bob, jitter, hx, hy;
    uint8_t tint_amt;

    if ((int)mood < 0 || mood >= RK_MOOD_COUNT) {
        mood = RK_MOOD_UNKNOWN;
    }
    lk = &LOOKS[mood];

    /* Respiración: una senoidal lenta sobre el eje Y. Es el detalle más
     * barato que separa "un dibujo" de "algo vivo". */
    bob = lk->bob_amp
        ? rk_sin8((uint8_t)(t_ms * lk->bob_speed / 1000)) * lk->bob_amp / 127
        : 0;
    jitter = lk->shiver ? (((t_ms / 60) % 2) ? lk->shiver : -lk->shiver) : 0;

    bx = cx - RK_TUGA_W / 2 + jitter;
    by = cy - RK_TUGA_H / 2 + bob;

    /* Sombra en el piso: deliberadamente NO sigue al bob, y eso es lo que
     * hace leer la respiración como altura en vez de como deslizamiento. */
    {
        int i;
        for (i = 0; i < 3; i++) {
            rk_hline(fb, cx - 20 + i * 3, cy + RK_TUGA_H / 2 - 3 + i,
                     40 - i * 6, RK_RGB(16, 22, 18));
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

    if (lk->dim > 0) {
        /* En penumbra el simbionte se apaga con la escena. Si no, queda
         * flotando a pleno brillo sobre un fondo oscuro y rompe la ilusión. */
        rk_blit_tint(fb, &rk_tuga_body, bx, by,
                     tint_amt > 0 ? lk->tint : RK_RGB(8, 10, 18),
                     tint_amt > lk->dim ? tint_amt : lk->dim);
    } else if (tint_amt > 0) {
        rk_blit_tint(fb, &rk_tuga_body, bx, by, lk->tint, tint_amt);
    } else {
        rk_blit(fb, &rk_tuga_body, bx, by);
    }

    /* Parpadeo: abierto casi siempre, cerrado 120 ms cada ~3,4 s. El período
     * es primo respecto del de la respiración para que no se sincronicen. */
    eye = *lk->eye;
    if (lk->blinks && (t_ms % 3400) < 120) {
        eye = &rk_eye_blink;
    }
    mouth = *lk->mouth;

    hx = bx + RK_HEAD_CX;
    hy = by + RK_HEAD_CY;

    {
        int ey = hy - RK_EYE_DY - eye->h / 2;
        int my = hy + RK_MOUTH_DY - mouth->h / 2;
        if (lk->dim > 0) {
            rk_color_t d = RK_RGB(8, 10, 18);
            rk_blit_tint(fb, eye,   hx - RK_EYE_DX - eye->w / 2, ey, d, lk->dim);
            rk_blit_tint(fb, eye,   hx + RK_EYE_DX - eye->w / 2, ey, d, lk->dim);
            rk_blit_tint(fb, mouth, hx - mouth->w / 2,           my, d, lk->dim);
        } else {
            rk_blit(fb, eye,   hx - RK_EYE_DX - eye->w / 2, ey);
            rk_blit(fb, eye,   hx + RK_EYE_DX - eye->w / 2, ey);
            rk_blit(fb, mouth, hx - mouth->w / 2,           my);
        }
    }
}
