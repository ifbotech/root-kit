#include "cara.h"
#include "../gfx/font.h"
#include "../art/face.h"
#include "../nodo/power.h"
#include <stddef.h>
#include <stdio.h>

/* Debajo de este porcentaje aparece el aviso. Es lo único que la app no
 * puede resolver sola: para cambiar la celda hay que ir hasta la maceta, y
 * conviene que el aparato lo pida antes de apagarse. */
#define BATT_AVISO 15

void rk_cara_draw(rk_fb_t *fb, const rk_node_t *n, uint32_t t_ms)
{
    uint8_t adornos;
    int pct;

    if (fb == NULL) {
        return;
    }
    if (n == NULL) {
        rk_face_draw(fb, NULL, RK_MOOD_UNKNOWN, RK_SEV_OK, 0u, t_ms);
        return;
    }

    /* Lo que el crecimiento desbloqueó: el modelo te tocó por azar, pero el
     * aura y la corona se ganan cuidando la planta. */
    adornos = rk_face_adornos_etapa((int)rk_stage_from_bond(&n->bond));

    rk_face_draw(fb, n->persona, n->verdict.mood, n->verdict.severity,
                 adornos, t_ms);

    /* ---- aviso de batería, sólo cuando importa ------------------------- */
    pct = rk_batt_pct(n->tel.batt_mv);
    if (n->tel.valid && pct < BATT_AVISO) {
        int w = fb->w / 5;
        int h = fb->h / 22;
        int x = fb->w / 24;
        int y = fb->h / 24;
        rk_color_t c = (pct < 6) ? RK_RGB(255, 90, 80) : RK_RGB(250, 190, 90);

        /* Late despacio: en una pantalla sin interfaz, un elemento fijo se
         * vuelve parte del dibujo y deja de avisar. */
        if (((t_ms / 900u) % 2u) == 0u) {
            rk_rect(fb, x, y, w, h, c);
            rk_fill_rect(fb, x + w, y + h / 3, 2, h / 3, c);
            rk_fill_rect(fb, x + 2, y + 2, (w - 4) * pct / 100, h - 4, c);
        }
    }
}

void rk_cara_emparejar(rk_fb_t *fb, const uint8_t id[6],
                       const rk_persona_t *p, uint32_t t_ms)
{
    char code[8];
    int cx, y;

    if (fb == NULL) {
        return;
    }
    if (p == NULL) {
        p = rk_persona_at(0);
    }
    cx = fb->w / 2;

    /* La cara ya está, dormida, desde antes de tener planta: el objeto es
     * una criatura apenas se enciende, no un aparato esperando configuración.
     * Ese primer encendido es medio producto. */
    rk_face_draw(fb, p, RK_MOOD_SLEEPING, RK_SEV_OK, 0u, t_ms);

    /* Código corto de emparejamiento sobre una banda oscura, abajo, donde no
     * tapa los ojos. Seis caracteres alcanzan y entran a escala 2, que es lo
     * que hace falta para tipearlo sin agacharse hasta la maceta. */
    y = fb->h - fb->h / 5;
    rk_fill_rect(fb, 0, y, fb->w, fb->h - y, rk_dim(p->fondo, 120));
    rk_hline(fb, 0, y, fb->w, p->acento);

    if (id != NULL) {
        snprintf(code, sizeof code, "%02X%02X%02X",
                 (unsigned)id[3], (unsigned)id[4], (unsigned)id[5]);
    } else {
        snprintf(code, sizeof code, "------");
    }
    rk_text_center(fb, cx, y + fb->h / 40, code, p->acento, 2);

    if ((t_ms / 700u) % 2u == 0u) {
        rk_text_center(fb, cx, y + fb->h / 40 + 18, "CARGALO EN LA APP",
                       rk_mix(p->acento, p->fondo, 120), 1);
    }
}
