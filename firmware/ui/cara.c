#include "cara.h"
#include "../art/face.h"
#include "../nodo/power.h"
#include <stddef.h>

/* Un personaje sin personaje: ojos redondos, sin cejas ni boca, en grises
 * tibios. Es lo que se ve entre el vínculo y el cofre. */
const rk_persona_t rk_persona_incognita = {
    "incognito", "?", "", "Todavía no sabe quién es.",
    RK_RAR_COMUN,
    RK_OJOS_REDONDOS, 15, 15, 21, 0,
    RK_CEJA_NINGUNA, 0, 0,
    RK_BOCA_NINGUNA, 0,
    0u,
    RK_RGB( 38,  40,  46), RK_RGB( 30,  32,  38),
    RK_RGB(150, 154, 166), RK_RGB(210, 214, 222),
    RK_RGB(120, 124, 136), RK_RGB(210, 214, 222)
};

void rk_cara_draw(rk_fb_t *fb, const rk_node_t *n, uint32_t t_ms)
{
    uint8_t adornos;
    rk_mood_t mood;

    if (fb == NULL) {
        return;
    }
    if (n == NULL) {
        rk_face_draw(fb, NULL, RK_MOOD_UNKNOWN, RK_SEV_OK, 0u, t_ms);
        return;
    }

    /* Lo que el crecimiento desbloqueó: el modelo te tocó en el cofre, pero
     * el aura y la corona se ganan cuidando la planta. */
    adornos = rk_face_adornos_etapa((int)rk_stage_from_bond(&n->bond));
    mood = n->verdict.mood;

    /* Batería crítica: si la planta está bien, la cara se duerme. Es la
     * única pista de batería en la pantalla, y es a propósito una cara y no
     * un ícono. El aviso con palabras llega por la app. */
    if (n->tel.valid && n->tel.batt_mv > 0u && rk_batt_is_critical(n->tel.batt_mv)
        && mood == RK_MOOD_HAPPY) {
        mood = RK_MOOD_SLEEPING;
    }

    rk_face_draw(fb, n->persona, mood, n->verdict.severity, adornos, t_ms);
}

void rk_cara_dormida(rk_fb_t *fb, uint32_t t_ms)
{
    if (fb == NULL) {
        return;
    }
    rk_face_draw(fb, &rk_persona_incognita, RK_MOOD_SLEEPING, RK_SEV_OK, 0u, t_ms);
}
