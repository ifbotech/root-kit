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
    RK_ACC_NINGUNO,
    RK_RGB( 38,  40,  46), RK_RGB( 30,  32,  38),
    RK_RGB(150, 154, 166), RK_RGB(210, 214, 222),
    RK_RGB(120, 124, 136), RK_RGB(210, 214, 222)
};

rk_mood_t rk_cara_animo_de(const rk_node_t *n)
{
    rk_mood_t mood;

    if (n == NULL) {
        return RK_MOOD_UNKNOWN;
    }
    mood = n->verdict.mood;

    /* Batería crítica: si la planta está bien, la cara se duerme. Es la
     * única pista de batería en la pantalla, y es a propósito una cara y no
     * un ícono. El aviso con palabras llega por la app. */
    if (n->tel.valid && n->tel.batt_mv > 0u && rk_batt_is_critical(n->tel.batt_mv)
        && mood == RK_MOOD_HAPPY) {
        mood = RK_MOOD_SLEEPING;
    }
    return mood;
}

/* Lo que el crecimiento desbloqueó: el modelo te tocó en el cofre, pero el
 * aura y la corona se ganan cuidando la planta. */
static uint8_t adornos_de(const rk_node_t *n)
{
    return rk_face_adornos_etapa((int)rk_stage_from_bond(&n->bond));
}

void rk_cara_draw(rk_fb_t *fb, const rk_node_t *n, uint32_t t_ms)
{
    if (fb == NULL) {
        return;
    }
    if (n == NULL) {
        rk_face_draw(fb, NULL, RK_MOOD_UNKNOWN, RK_SEV_OK, 0u, t_ms);
        return;
    }
    rk_face_draw(fb, n->persona, rk_cara_animo_de(n), n->verdict.severity,
                 adornos_de(n), t_ms);
}

void rk_cara_dormida(rk_fb_t *fb, uint32_t t_ms)
{
    if (fb == NULL) {
        return;
    }
    rk_face_draw(fb, &rk_persona_incognita, RK_MOOD_SLEEPING, RK_SEV_OK, 0u, t_ms);
}

/* ------------------------------------------------------- la transición --- */
void rk_cara_anim_iniciar(rk_cara_anim_t *a, rk_mood_t mood, uint32_t t_ms)
{
    if (a == NULL) {
        return;
    }
    a->desde = (uint8_t)mood;
    a->hacia = (uint8_t)mood;
    a->t0_ms = t_ms;
    a->iniciada = true;
}

void rk_cara_anim_animo(rk_cara_anim_t *a, rk_mood_t mood, uint32_t t_ms)
{
    if (a == NULL) {
        return;
    }
    if (!a->iniciada) {
        rk_cara_anim_iniciar(a, mood, t_ms);
        return;
    }
    if ((uint8_t)mood == a->hacia) {
        return;
    }
    /* Si interrumpe una transición a medio camino, arranca desde el ánimo
     * que dominaba en pantalla: el destino si ya había pasado la mitad, el
     * origen si no. */
    if (rk_cara_anim_pct(a, t_ms) >= 50u) {
        a->desde = a->hacia;
    }
    a->hacia = (uint8_t)mood;
    a->t0_ms = t_ms;
}

uint8_t rk_cara_anim_pct(const rk_cara_anim_t *a, uint32_t t_ms)
{
    uint32_t pasado;

    if (a == NULL || !a->iniciada || a->desde == a->hacia) {
        return 100u;
    }
    pasado = t_ms - a->t0_ms;           /* módulo 2^32: aguanta el desborde */
    if (pasado >= RK_CARA_TRANSICION_MS) {
        return 100u;
    }
    return rk_face_ease((uint8_t)(pasado * 100u / RK_CARA_TRANSICION_MS));
}

bool rk_cara_anim_en_curso(const rk_cara_anim_t *a, uint32_t t_ms)
{
    return rk_cara_anim_pct(a, t_ms) < 100u;
}

void rk_cara_anim_draw(rk_fb_t *fb, const rk_persona_t *p, const rk_cara_anim_t *a,
                       rk_severity_t sev, uint8_t adornos_extra, uint8_t cierre,
                       uint32_t t_ms)
{
    if (fb == NULL) {
        return;
    }
    if (a == NULL || !a->iniciada) {
        rk_face_draw_cierre(fb, p, RK_MOOD_UNKNOWN, sev, adornos_extra, cierre, t_ms);
        return;
    }
    rk_face_draw_mezcla(fb, p, (rk_mood_t)a->desde, (rk_mood_t)a->hacia,
                        rk_cara_anim_pct(a, t_ms), sev, adornos_extra, cierre, t_ms);
}

void rk_cara_draw_anim(rk_fb_t *fb, const rk_node_t *n, rk_cara_anim_t *a,
                       uint8_t cierre, uint32_t t_ms)
{
    if (fb == NULL) {
        return;
    }
    if (n == NULL) {
        rk_face_draw_cierre(fb, NULL, RK_MOOD_UNKNOWN, RK_SEV_OK, 0u, cierre, t_ms);
        return;
    }
    rk_cara_anim_animo(a, rk_cara_animo_de(n), t_ms);
    rk_cara_anim_draw(fb, n->persona, a, n->verdict.severity, adornos_de(n), cierre, t_ms);
}
