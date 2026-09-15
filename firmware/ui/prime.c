#include "prime.h"
#include "../gfx/font.h"
#include "../art/adulto.h"
#include "../art/brote.h"
#include "../ui/gacha.h"
/* La curva de descarga vive en nodo/power.c y se usa desde acá en vez de
 * reimplementarla: una sola curva, un solo lugar donde corregirla. */
#include "../nodo/power.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Bandas verticales del panel de 240x320. */
#define BAR_H      34
#define SCENE_Y    BAR_H
#define SCENE_H   176
#define SAY_Y     (SCENE_Y + SCENE_H)
#define SAY_H      32
#define STATS_Y   (SAY_Y + SAY_H)
#define STATS_H    42
#define SEL_Y     (STATS_Y + STATS_H)
#define SEL_H     (RK_PRIME_H - SEL_Y)

#define T          RK_PRIME_TEXT   /* escala base del texto: 2 */

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

static rk_color_t link_color(rk_link_t l)
{
    switch (l) {
    case RK_LINK_VIVO:  return C_OK;
    case RK_LINK_TIBIO: return C_WARN;
    case RK_LINK_CAIDO: return C_CRIT;
    default:            return C_INK_DIM;
    }
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

    rk_fill_rect(fb, x, y, w, 6, C_PANEL);
    rk_rect(fb, x, y, w, 6, C_LINE);

    gx0 = x + (lo - vmin) * w / span;
    gx1 = x + (hi - vmin) * w / span;
    if (gx1 > gx0) {
        rk_fill_rect(fb, gx0, y + 1, gx1 - gx0, 4, rk_mix(C_PANEL, C_OK, 70));
    }

    if (value < vmin) { value = vmin; }
    if (value > vmax) { value = vmax; }
    vx = x + (value - vmin) * w / span;
    if (vx > x + w - 3) { vx = x + w - 3; }
    rk_fill_rect(fb, vx, y - 2, 3, 10, c);
}

/* Una fila de dato: etiqueta chica a la izquierda, valor grande a la
 * derecha y el medidor debajo, ocupando el ancho completo. */
static void stat_row(rk_fb_t *fb, int y, const char *label, const char *val,
                     int value, int vmin, int vmax, int lo, int hi,
                     rk_color_t c)
{
    rk_text(fb, 8, y, label, C_INK_DIM, 1);
    rk_text(fb, RK_PRIME_W - 8 - rk_text_w(val, T), y - 3, val, C_INK, T);
    gauge(fb, 8, y + 11, RK_PRIME_W - 16, value, vmin, vmax, lo, hi, c);
}

/* ------------------------------------------------------------ pantalla -- */
static void pantalla_vacia(rk_fb_t *fb, uint32_t t_ms)
{
    int cx = RK_PRIME_W / 2;

    rk_fb_clear(fb, C_PANEL);
    /* El bicho igual aparece, dormido: una pantalla de bienvenida sin
     * criatura no vende lo que el producto es. */
    rk_adulto_draw(fb, cx, 130, NULL, RK_MOOD_SLEEPING, RK_SEV_OK,
                   RK_PRIME_ART, t_ms);
    rk_text_center(fb, cx, 210, "ROOTKIT", C_ACCENT, 3);
    rk_text_center(fb, cx, 244, "SIN MACETAS", C_INK_DIM, T);
    rk_text_center(fb, cx, 268, "REGISTRA UNA", C_INK_DIM, 1);
    rk_text_center(fb, cx, 280, "EN EL HUB", C_INK_DIM, 1);
}

void rk_prime_draw(rk_fb_t *fb, const rk_roster_t *r, uint32_t t_ms)
{
    const rk_node_t *n;
    rk_color_t top, bottom;
    rk_link_t  link;
    char buf[24];
    int i, sel;

    if (fb == NULL) {
        return;
    }
    if (r == NULL || r->count == 0) {
        pantalla_vacia(fb, t_ms);
        return;
    }
    sel = r->selected;
    if (sel < 0 || sel >= r->count) {
        sel = 0;
    }
    n = &r->nodes[sel];
    link = rk_node_link(n);

    /* ---- escena: el fondo también expresa el ánimo ---------------------- */
    rk_escena_colores(n->verdict.mood, &top, &bottom);
    rk_vgradient(fb, 0, SCENE_Y, RK_PRIME_W, SCENE_H, top, bottom);

    /* línea de horizonte, para que el bicho se apoye en algo */
    rk_hline(fb, 0, SCENE_Y + SCENE_H - 30, RK_PRIME_W,
             rk_mix(bottom, 0x0000, 60));
    rk_fill_rect(fb, 0, SCENE_Y + SCENE_H - 29, RK_PRIME_W, 29,
                 rk_mix(bottom, 0x0000, 90));

    rk_adulto_fx_back(fb, 0, SCENE_Y, RK_PRIME_W, SCENE_H,
                      n->verdict.mood, t_ms);
    rk_adulto_draw(fb, RK_PRIME_W / 2, SCENE_Y + SCENE_H / 2 - 6, n->comp,
                   n->verdict.mood, n->verdict.severity, RK_PRIME_ART, t_ms);
    rk_adulto_fx_front(fb, 0, SCENE_Y, RK_PRIME_W, SCENE_H,
                       n->verdict.mood, t_ms);

    /* Etiqueta del simbionte, abajo a la izquierda de la escena: el nombre y
     * la etapa del vínculo. Es lo que convierte "un dibujo" en "mi bicho". */
    if (n->comp != NULL) {
        rk_rarity_t rar = rk_rarity_of(n->comp);
        rk_stage_t  et  = rk_stage_from_bond(&n->bond);
        rk_color_t  rc  = rk_rarity_color(rar);
        int         ly  = SCENE_Y + SCENE_H - 24;

        rk_text_shadow(fb, 8, ly, n->comp->nombre, rc, RK_RGB(8, 10, 9), 1);
        rk_text_shadow(fb, 8, ly + 11, rk_stage_name(et), C_INK_DIM,
                       RK_RGB(8, 10, 9), 1);

        /* Progreso hacia la próxima etapa: una barra fina que sólo aparece
         * si falta algo. En la última etapa no hay barra, hay un punto. */
        {
            int px = RK_PRIME_W - 8 - 60;
            if (rk_stage_faltan(&n->bond) > 0u) {
                rk_fill_rect(fb, px, ly + 3, 60, 4, rk_mix(bottom, 0x0000, 140));
                rk_fill_rect(fb, px, ly + 3,
                             60 * rk_stage_progreso(&n->bond) / 100, 4, rc);
            } else {
                rk_disc(fb, px + 56, ly + 5, 3, rc);
            }
        }
    }

    /* ---- barra superior -------------------------------------------------- */
    rk_fill_rect(fb, 0, 0, RK_PRIME_W, BAR_H, C_PANEL);
    rk_hline(fb, 0, BAR_H - 1, RK_PRIME_W, C_LINE);
    rk_text(fb, 8, 9, n->nombre, C_INK, T);

    /* Rol del nodo: el Prime dice PRIME, y eso ubica al usuario cuando el
     * kit tiene varias macetas y todas se parecen. */
    rk_text(fb, 8, 2, rk_role_name(n->role), C_ACCENT, 1);

    /* Punto de enlace, wifi como tres barritas, batería como una pila. */
    rk_disc(fb, 152, 17, 4, link_color(link));
    for (i = 0; i < 3; i++) {
        rk_color_t c = (r->wifi) ? C_ACCENT : C_LINE;
        rk_fill_rect(fb, 166 + i * 6, 22 - i * 5, 4, 5 + i * 5, c);
    }
    {
        /* El Prime va enchufado: donde iría el porcentaje va un enchufe. En
         * los Minis la misma esquina muestra la celda. */
        if (n->role == RK_ROLE_PRIME) {
            rk_fill_rect(fb, 200, 10, 22, 14, C_PANEL_2);
            rk_rect(fb, 200, 10, 22, 14, C_INK_DIM);
            rk_fill_rect(fb, 208, 6, 3, 5, C_INK_DIM);
            rk_fill_rect(fb, 214, 6, 3, 5, C_INK_DIM);
            rk_fill_rect(fb, 205, 15, 12, 4, C_OK);
        } else {
            int pct = rk_batt_pct(n->tel.batt_mv);
            rk_rect(fb, 200, 10, 26, 14, C_INK_DIM);
            rk_fill_rect(fb, 226, 14, 3, 6, C_INK_DIM);
            rk_fill_rect(fb, 203, 13, 20 * pct / 100, 8,
                         pct < 20 ? C_CRIT : (pct < 40 ? C_WARN : C_OK));
        }
    }

    /* ---- globo de diálogo: el bicho habla, no muestra una gráfica -------- */
    rk_fill_rect(fb, 0, SAY_Y, RK_PRIME_W, SAY_H, C_PANEL_2);
    rk_hline(fb, 0, SAY_Y, RK_PRIME_W, C_LINE);
    rk_fill_rect(fb, 0, SAY_Y, 5, SAY_H, sev_color(n->verdict.severity));
    {
        /* Las frases están pensadas para entrar en un renglón a escala 2
         * (unos 19 caracteres). Si alguna crece, se achica antes de
         * cortarse: una frase truncada es peor que una chica. */
        const char *say = (link == RK_LINK_CAIDO)
                          ? rk_mood_reason(RK_MOOD_OFFLINE)
                          : n->verdict.reason;
        int esc = rk_text_w(say, T) > RK_PRIME_W - 20 ? 1 : T;
        rk_text_center(fb, RK_PRIME_W / 2 + 3, SAY_Y + (SAY_H - 7 * esc) / 2,
                       say, C_INK, esc);
    }

    /* ---- panel de datos --------------------------------------------------- */
    rk_fill_rect(fb, 0, STATS_Y, RK_PRIME_W, STATS_H, C_PANEL);

    if (n->sp == NULL) {
        rk_text_center(fb, RK_PRIME_W / 2, STATS_Y + 16, "SIN ESPECIE",
                       C_INK_DIM, T);
    } else {
        /* Sólo un dato por vez, rotando: a 42 px de alto entran tres filas
         * apretadas e ilegibles o una cómoda. Rota cada cuatro segundos, y
         * la que tiene el problema se queda fija hasta que se resuelva. */
        int cual = (int)((t_ms / 4000u) % 3u);
        rk_mood_t m = n->verdict.mood;

        if (m == RK_MOOD_THIRSTY || m == RK_MOOD_DROWNING)      { cual = 0; }
        else if (m == RK_MOOD_COLD || m == RK_MOOD_HOT)         { cual = 1; }
        else if (m == RK_MOOD_DARK || m == RK_MOOD_SCORCHED)    { cual = 2; }

        if (cual == 0) {
            snprintf(buf, sizeof buf, "%u%%", (unsigned)n->tel.soil_pct);
            stat_row(fb, STATS_Y + 12, "TIERRA", buf,
                     n->tel.soil_pct, 0, 100, n->sp->soil_min, n->sp->soil_max,
                     (m == RK_MOOD_THIRSTY || m == RK_MOOD_DROWNING)
                         ? sev_color(n->verdict.severity) : C_ACCENT);
        } else if (cual == 1) {
            snprintf(buf, sizeof buf, "%d.%d~C",
                     n->tel.temp_dc / 10, (n->tel.temp_dc % 10 + 10) % 10);
            stat_row(fb, STATS_Y + 12, "CLIMA", buf,
                     n->tel.temp_dc, 0, 450,
                     n->sp->temp_min_dc, n->sp->temp_max_dc,
                     (m == RK_MOOD_COLD || m == RK_MOOD_HOT)
                         ? sev_color(n->verdict.severity) : C_ACCENT);
        } else {
            /* La luz se comprime: de 0 a 40k lineal sería ilegible. */
            unsigned lux = n->tel.lux;
            int shown = (int)(lux > 40000u ? 40000u : lux) / 400;
            int lo    = (int)(n->sp->lux_min / 400);
            int hi    = (int)((n->sp->lux_max > 40000u ? 40000u
                                                      : n->sp->lux_max) / 400);
            if (lux >= 1000u) {
                snprintf(buf, sizeof buf, "%uK LUX", lux / 1000u);
            } else {
                snprintf(buf, sizeof buf, "%u LUX", lux);
            }
            stat_row(fb, STATS_Y + 12, "LUZ", buf, shown, 0, 100, lo, hi,
                     (m == RK_MOOD_DARK || m == RK_MOOD_SCORCHED)
                         ? sev_color(n->verdict.severity) : C_ACCENT);
        }
    }

    /* ---- selector de nodos ------------------------------------------------ */
    /* Cada ficha muestra el BROTE del simbionte de ese nodo, chiquito. Es la
     * pieza que ata las dos pantallas: lo que ves en la maceta del Mini es
     * exactamente lo que ves en su ficha del Prime. */
    rk_fill_rect(fb, 0, SEL_Y, RK_PRIME_W, SEL_H, C_PANEL_2);
    rk_hline(fb, 0, SEL_Y, RK_PRIME_W, C_LINE);

    for (i = 0; i < r->count && i < RK_MAX_NODES; i++) {
        const rk_node_t *q = &r->nodes[i];
        int w  = RK_PRIME_W / r->count;
        int x  = i * w;
        int cxp = x + w / 2;
        rk_color_t c = sev_color(q->verdict.severity);

        if (i == sel) {
            rk_fill_rect(fb, x + 1, SEL_Y + 2, w - 2, SEL_H - 2, C_PANEL);
            rk_fill_rect(fb, x + 1, SEL_Y + 1, w - 2, 2, C_ACCENT);
        }
        rk_brote_draw(fb, cxp, SEL_Y + 17, q->comp, q->verdict.mood,
                      RK_SEV_OK, rk_stage_from_bond(&q->bond), 1, t_ms);
        /* Punto de estado: se ve de un vistazo cuál reclama algo sin tener
         * que ir maceta por maceta. */
        rk_fill_rect(fb, cxp + 11, SEL_Y + 6, 5, 5, c);
        if (q->role == RK_ROLE_PRIME) {
            rk_fill_rect(fb, x + w / 2 - 15, SEL_Y + 6, 4, 4, C_ACCENT);
        }
    }
}

int rk_prime_hit(const rk_roster_t *r, int x, int y)
{
    int w;

    if (r == NULL || r->count <= 0 || y < SEL_Y || y >= RK_PRIME_H) {
        return -1;
    }
    w = RK_PRIME_W / r->count;
    if (w <= 0 || x < 0 || x >= RK_PRIME_W) {
        return -1;
    }
    return (x / w) < r->count ? (x / w) : -1;
}
