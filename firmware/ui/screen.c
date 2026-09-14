#include "screen.h"
#include "../gfx/font.h"
#include "../art/tuga.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Bandas verticales del lienzo de 160x240. */
#define BAR_H      16                  /* barra superior            */
#define SCENE_Y    BAR_H
#define SCENE_H   132                  /* la escena del simbionte   */
#define SAY_Y     (SCENE_Y + SCENE_H)  /* el globo de diálogo       */
#define SAY_H      20
#define STATS_Y   (SAY_Y + SAY_H)
#define STATS_H    52
#define SEL_Y     (STATS_Y + STATS_H)
#define SEL_H     (RK_CANVAS_H - SEL_Y)

#define C_INK      RK_RGB(232, 240, 226)
#define C_INK_DIM  RK_RGB(138, 152, 136)
#define C_PANEL    RK_RGB( 18,  24,  20)
#define C_PANEL_2  RK_RGB( 28,  36,  30)
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

/* ------------------------------------------------------------ widgets --- */

/* Barra horizontal con la zona de confort marcada. Es el widget que hace
 * que un número suelto se vuelva interpretable de un vistazo: se ve si el
 * valor está dentro del rango, y cuán lejos del borde. */
static void gauge(rk_fb_t *fb, int x, int y, int w,
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

    rk_fill_rect(fb, x, y, w, 5, C_PANEL);
    rk_rect(fb, x, y, w, 5, C_LINE);

    /* zona cómoda */
    gx0 = x + (lo - vmin) * w / span;
    gx1 = x + (hi - vmin) * w / span;
    if (gx1 > gx0) {
        rk_fill_rect(fb, gx0, y + 1, gx1 - gx0, 3, rk_mix(C_PANEL, C_OK, 70));
    }

    /* valor */
    if (value < vmin) { value = vmin; }
    if (value > vmax) { value = vmax; }
    vx = x + (value - vmin) * w / span;
    if (vx > x + w - 2) { vx = x + w - 2; }
    rk_fill_rect(fb, vx, y - 1, 2, 7, c);
}

static void stat_row(rk_fb_t *fb, int y, const char *label, const char *val,
                     int value, int vmin, int vmax, int lo, int hi,
                     rk_color_t c)
{
    rk_text(fb, 6, y, label, C_INK_DIM, 1);
    rk_text(fb, 154 - rk_text_w(val, 1), y, val, C_INK, 1);
    gauge(fb, 6, y + 9, 148, value, vmin, vmax, lo, hi, c);
}

/* ------------------------------------------------------------ pantalla -- */

void rk_screen_draw(rk_fb_t *fb, const rk_state_t *st, uint32_t t_ms)
{
    const rk_plant_t *p;
    rk_color_t top, bottom;
    char buf[24];
    int i, cxp;

    if (st == NULL || st->count == 0) {
        rk_fb_clear(fb, C_PANEL);
        rk_text_center(fb, RK_CANVAS_W / 2, 110, "SIN PLANTAS", C_INK_DIM, 1);
        rk_text_center(fb, RK_CANVAS_W / 2, 124, "REGISTRA UNA EN EL HUB",
                       C_INK_DIM, 1);
        return;
    }
    p = &st->plants[st->selected];

    /* ---- escena: el fondo también expresa el ánimo ---------------------- */
    rk_tuga_scene_colors(p->verdict.mood, &top, &bottom);
    rk_vgradient(fb, 0, SCENE_Y, RK_CANVAS_W, SCENE_H, top, bottom);

    /* línea de horizonte, para que el bicho se apoye en algo */
    rk_hline(fb, 0, SCENE_Y + SCENE_H - 22, RK_CANVAS_W,
             rk_mix(bottom, 0x0000, 60));
    rk_fill_rect(fb, 0, SCENE_Y + SCENE_H - 21, RK_CANVAS_W, 21,
                 rk_mix(bottom, 0x0000, 90));

    rk_tuga_fx_back(fb, 0, SCENE_Y, RK_CANVAS_W, SCENE_H,
                    p->verdict.mood, t_ms);
    rk_tuga_draw(fb, RK_CANVAS_W / 2, SCENE_Y + SCENE_H / 2 - 4,
                 p->verdict.mood, p->verdict.severity, t_ms);
    rk_tuga_fx_front(fb, 0, SCENE_Y, RK_CANVAS_W, SCENE_H,
                     p->verdict.mood, t_ms);

    /* ---- barra superior -------------------------------------------------- */
    rk_fill_rect(fb, 0, 0, RK_CANVAS_W, BAR_H, C_PANEL);
    rk_hline(fb, 0, BAR_H - 1, RK_CANVAS_W, C_LINE);
    rk_text(fb, 5, 5, p->nombre, C_INK, 1);

    /* wifi como tres barritas, batería como una pila con relleno */
    for (i = 0; i < 3; i++) {
        rk_color_t c = st->wifi ? C_ACCENT : C_LINE;
        rk_fill_rect(fb, 118 + i * 4, 10 - i * 3, 2, 3 + i * 3, c);
    }
    {
        int pct = st->hub_batt_pct > 100 ? 100 : st->hub_batt_pct;
        rk_rect(fb, 136, 4, 17, 8, C_INK_DIM);
        rk_fill_rect(fb, 153, 6, 2, 4, C_INK_DIM);
        rk_fill_rect(fb, 138, 6, 13 * pct / 100, 4,
                     pct < 20 ? C_CRIT : (pct < 40 ? C_WARN : C_OK));
    }

    /* ---- globo de diálogo: el bicho habla, no muestra una gráfica -------- */
    rk_fill_rect(fb, 0, SAY_Y, RK_CANVAS_W, SAY_H, C_PANEL_2);
    rk_hline(fb, 0, SAY_Y, RK_CANVAS_W, C_LINE);
    rk_fill_rect(fb, 0, SAY_Y, 3, SAY_H, sev_color(p->verdict.severity));
    /* Las frases están pensadas para entrar en un renglón de 160 px (unos
     * 25 caracteres). Si alguna crece, se achica antes de cortarse. */
    {
        const char *say = p->verdict.reason;
        int sc = rk_text_w(say, 1) > RK_CANVAS_W - 10 ? 0 : 1;
        if (sc == 0) {
            rk_text(fb, 6, SAY_Y + 7, say, C_INK, 1);   /* al ras, sin centrar */
        } else {
            rk_text_center(fb, RK_CANVAS_W / 2 + 2, SAY_Y + 7, say, C_INK, 1);
        }
    }

    /* ---- panel de datos --------------------------------------------------- */
    rk_fill_rect(fb, 0, STATS_Y, RK_CANVAS_W, STATS_H, C_PANEL);

    snprintf(buf, sizeof buf, "%u%%", (unsigned)p->tel.soil_pct);
    stat_row(fb, STATS_Y + 4, "TIERRA", buf,
             p->tel.soil_pct, 0, 100, p->sp->soil_min, p->sp->soil_max,
             (p->verdict.mood == RK_MOOD_THIRSTY ||
              p->verdict.mood == RK_MOOD_DROWNING)
                 ? sev_color(p->verdict.severity) : C_ACCENT);

    snprintf(buf, sizeof buf, "%d.%d~C",
             p->tel.temp_dc / 10, (p->tel.temp_dc % 10 + 10) % 10);
    stat_row(fb, STATS_Y + 22, "CLIMA", buf,
             p->tel.temp_dc, 0, 450, p->sp->temp_min_dc, p->sp->temp_max_dc,
             (p->verdict.mood == RK_MOOD_COLD || p->verdict.mood == RK_MOOD_HOT)
                 ? sev_color(p->verdict.severity) : C_ACCENT);

    {
        /* La luz se comprime: de 0 a 40k lineal sería ilegible en 148 px. */
        unsigned lux = p->tel.lux;
        int shown    = (int)(lux > 40000u ? 40000u : lux) / 400;
        int lo       = (int)(p->sp->lux_min / 400);
        int hi       = (int)((p->sp->lux_max > 40000u ? 40000u
                                                      : p->sp->lux_max) / 400);
        if (lux >= 1000u) {
            snprintf(buf, sizeof buf, "%uK LUX", lux / 1000u);
        } else {
            snprintf(buf, sizeof buf, "%u LUX", lux);
        }
        stat_row(fb, STATS_Y + 40 - 4, "LUZ", buf, shown, 0, 100, lo, hi,
                 (p->verdict.mood == RK_MOOD_DARK ||
                  p->verdict.mood == RK_MOOD_SCORCHED)
                     ? sev_color(p->verdict.severity) : C_ACCENT);
    }

    /* ---- selector de plantas ---------------------------------------------- */
    rk_fill_rect(fb, 0, SEL_Y, RK_CANVAS_W, SEL_H, C_PANEL_2);
    rk_hline(fb, 0, SEL_Y, RK_CANVAS_W, C_LINE);

    for (i = 0; i < st->count && i < RK_MAX_PLANTS; i++) {
        const rk_plant_t *q = &st->plants[i];
        int w  = RK_CANVAS_W / st->count;
        int x  = i * w;
        rk_color_t c = sev_color(q->verdict.severity);

        if (i == st->selected) {
            rk_fill_rect(fb, x + 1, SEL_Y + 2, w - 2, SEL_H - 3, C_PANEL);
            rk_hline(fb, x + 1, SEL_Y + 1, w - 2, C_ACCENT);
        }
        /* Punto de estado por planta: se ve de un vistazo cuál reclama algo
         * sin tener que ir planta por planta. */
        rk_fill_rect(fb, x + w / 2 - 2, SEL_Y + 6, 4, 4, c);
        cxp = x + w / 2;
        buf[0] = q->nombre[0];
        buf[1] = q->nombre[1] ? q->nombre[1] : '\0';
        buf[2] = '\0';
        rk_text_center(fb, cxp, SEL_Y + 13, buf,
                       i == st->selected ? C_INK : C_INK_DIM, 1);
    }
}

int rk_screen_hit(const rk_state_t *st, int x, int y)
{
    int w;
    if (st == NULL || st->count <= 0 || y < SEL_Y) {
        return -1;
    }
    w = RK_CANVAS_W / st->count;
    if (w <= 0) {
        return -1;
    }
    if (x < 0 || x >= RK_CANVAS_W) {
        return -1;
    }
    return (x / w) < st->count ? (x / w) : -1;
}
