#include "mini.h"
#include "../gfx/font.h"
#include "../art/adulto.h"
#include "../art/brote.h"
#include "../nodo/power.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Bandas del panel de 128x128. */
#define BAR_H      14
#define SCENE_Y    BAR_H
#define SCENE_H    88
#define SAY_Y     (SCENE_Y + SCENE_H)
#define SAY_H      16
#define TIRA_Y    (SAY_Y + SAY_H)
#define TIRA_H    (RK_MINI_H - TIRA_Y)

#define C_INK      RK_RGB(232, 240, 226)
#define C_INK_DIM  RK_RGB(138, 152, 136)
#define C_PANEL    RK_RGB( 18,  24,  20)
#define C_LINE     RK_RGB( 46,  58,  48)
#define C_ACCENT   RK_RGB( 72, 214, 190)
#define C_WARN     RK_RGB(227, 165,  74)
#define C_CRIT     RK_RGB(220, 108,  86)
#define C_OK       RK_RGB( 92, 200, 140)

static rk_color_t sev_color(rk_severity_t s)
{
    if (s == RK_SEV_URGENT) { return C_CRIT; }
    if (s == RK_SEV_WATCH)  { return C_WARN; }
    return C_OK;
}

/* Una barrita de la tira inferior: 38 px de ancho, la zona cómoda marcada en
 * verde apagado y el valor como un tick. Sin número y sin etiqueta: la
 * posición en la tira ya dice cuál es. */
static void barrita(rk_fb_t *fb, int x, int y, int w,
                    int value, int vmin, int vmax, int lo, int hi,
                    rk_color_t c)
{
    int span = vmax - vmin;
    int gx0, gx1, vx;

    if (span <= 0) {
        return;
    }
    if (lo < vmin) { lo = vmin; }
    if (hi > vmax) { hi = vmax; }

    rk_fill_rect(fb, x, y, w, 5, RK_RGB(10, 14, 12));
    gx0 = x + (lo - vmin) * w / span;
    gx1 = x + (hi - vmin) * w / span;
    if (gx1 > gx0) {
        rk_fill_rect(fb, gx0, y + 1, gx1 - gx0, 3, RK_RGB(32, 62, 46));
    }
    if (value < vmin) { value = vmin; }
    if (value > vmax) { value = vmax; }
    vx = x + (value - vmin) * w / span;
    if (vx > x + w - 2) { vx = x + w - 2; }
    rk_fill_rect(fb, vx, y - 1, 2, 7, c);
}

void rk_mini_draw(rk_fb_t *fb, const rk_node_t *n, uint32_t t_ms)
{
    rk_color_t top, bottom, c;
    rk_mood_t  mood;
    rk_stage_t etapa;
    int x;

    if (fb == NULL) {
        return;
    }
    if (n == NULL) {
        rk_fb_clear(fb, C_PANEL);
        rk_text_center(fb, RK_MINI_W / 2, 60, "SIN PLANTA", C_INK_DIM, 1);
        return;
    }
    mood  = n->verdict.mood;
    etapa = rk_stage_from_bond(&n->bond);
    c     = sev_color(n->verdict.severity);

    /* ---- escena ---------------------------------------------------------- */
    rk_escena_colores(mood, &top, &bottom);
    rk_vgradient(fb, 0, SCENE_Y, RK_MINI_W, SCENE_H, top, bottom);
    rk_hline(fb, 0, SCENE_Y + SCENE_H - 16, RK_MINI_W,
             rk_mix(bottom, 0x0000, 60));
    rk_fill_rect(fb, 0, SCENE_Y + SCENE_H - 15, RK_MINI_W, 15,
                 rk_mix(bottom, 0x0000, 90));

    rk_adulto_fx_back(fb, 0, SCENE_Y, RK_MINI_W, SCENE_H, mood, t_ms);
    rk_brote_draw(fb, RK_MINI_W / 2, SCENE_Y + SCENE_H / 2 - 2, n->comp,
                  mood, n->verdict.severity, etapa, RK_MINI_ART, t_ms);
    rk_adulto_fx_front(fb, 0, SCENE_Y, RK_MINI_W, SCENE_H, mood, t_ms);

    /* ---- barra superior --------------------------------------------------- */
    rk_fill_rect(fb, 0, 0, RK_MINI_W, BAR_H, C_PANEL);
    rk_hline(fb, 0, BAR_H - 1, RK_MINI_W, C_LINE);
    rk_text(fb, 4, 4, n->nombre, C_INK, 1);

    {
        /* Pila de 16 px. En un nodo a batería este es el segundo dato más
         * importante de la pantalla, después de la cara. */
        int pct = rk_batt_pct(n->tel.batt_mv);
        rk_rect(fb, RK_MINI_W - 20, 4, 15, 7, C_INK_DIM);
        rk_fill_rect(fb, RK_MINI_W - 5, 6, 2, 3, C_INK_DIM);
        rk_fill_rect(fb, RK_MINI_W - 18, 6, 11 * pct / 100, 3,
                     pct < 20 ? C_CRIT : (pct < 40 ? C_WARN : C_OK));
    }

    /* ---- frase ------------------------------------------------------------ */
    rk_fill_rect(fb, 0, SAY_Y, RK_MINI_W, SAY_H, RK_RGB(24, 32, 27));
    rk_hline(fb, 0, SAY_Y, RK_MINI_W, C_LINE);
    rk_fill_rect(fb, 0, SAY_Y, 3, SAY_H, c);
    /* A escala 1 entran 21 caracteres en 128 px, y la frase más larga del
     * catálogo tiene 20. El test de la suite "mini" lo verifica para todas,
     * porque una frase que no entre se corta y deja al bicho balbuceando. */
    rk_text_center(fb, RK_MINI_W / 2 + 2, SAY_Y + 5, n->verdict.reason,
                   C_INK, 1);

    /* ---- tira de datos ----------------------------------------------------- */
    rk_fill_rect(fb, 0, TIRA_Y, RK_MINI_W, TIRA_H, C_PANEL);
    if (n->sp == NULL) {
        return;
    }
    x = 4;
    barrita(fb, x, TIRA_Y + 3, 38, n->tel.soil_pct, 0, 100,
            n->sp->soil_min, n->sp->soil_max,
            (mood == RK_MOOD_THIRSTY || mood == RK_MOOD_DROWNING)
                ? c : C_ACCENT);
    x += 42;
    barrita(fb, x, TIRA_Y + 3, 38, n->tel.temp_dc, 0, 450,
            n->sp->temp_min_dc, n->sp->temp_max_dc,
            (mood == RK_MOOD_COLD || mood == RK_MOOD_HOT) ? c : C_ACCENT);
    x += 42;
    {
        unsigned lux = n->tel.lux;
        int shown = (int)(lux > 40000u ? 40000u : lux) / 400;
        int lo    = (int)(n->sp->lux_min / 400);
        int hi    = (int)((n->sp->lux_max > 40000u ? 40000u
                                                   : n->sp->lux_max) / 400);
        barrita(fb, x, TIRA_Y + 3, 38, shown, 0, 100, lo, hi,
                (mood == RK_MOOD_DARK || mood == RK_MOOD_SCORCHED)
                    ? c : C_ACCENT);
    }
}

void rk_mini_emparejar(rk_fb_t *fb, const uint8_t id[6], uint32_t t_ms)
{
    char code[8];
    int  cx = RK_MINI_W / 2;

    if (fb == NULL) {
        return;
    }
    rk_fb_clear(fb, C_PANEL);

    /* Un brote genérico dormido: incluso sin planta el objeto ya es una
     * criatura, y eso es medio producto. */
    rk_brote_draw(fb, cx, 46, NULL, RK_MOOD_SLEEPING, RK_SEV_OK,
                  RK_ETAPA_ESPORA, RK_MINI_ART, t_ms);

    rk_text_center(fb, cx, 78, "SIN PLANTA", C_INK_DIM, 1);

    /* Código corto de emparejamiento: los tres últimos bytes del id en hexa.
     * Seis caracteres son suficientes para distinguir seis nodos y entran a
     * escala 2, que es lo que hace falta para poder tipearlo del otro lado
     * de la habitación sin agacharse. */
    if (id != NULL) {
        snprintf(code, sizeof code, "%02X%02X%02X",
                 (unsigned)id[3], (unsigned)id[4], (unsigned)id[5]);
    } else {
        snprintf(code, sizeof code, "------");
    }
    rk_fill_rect(fb, cx - 42, 90, 84, 22, RK_RGB(24, 32, 27));
    rk_rect(fb, cx - 42, 90, 84, 22, C_ACCENT);
    rk_text_center(fb, cx, 94, code, C_ACCENT, 2);

    if ((t_ms / 600u) % 2u == 0u) {
        rk_text_center(fb, cx, 118, "CARGALO EN EL HUB", C_INK_DIM, 1);
    }
}
