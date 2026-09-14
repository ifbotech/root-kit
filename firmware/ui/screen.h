/* screen.h — composición de la pantalla de la Terminal.
 *
 * Una sola función pura: dado el estado del sistema y un instante, escribe
 * el cuadro. Sin estado propio, sin efectos secundarios. Eso hace que el
 * simulador y la Terminal rendericen exactamente lo mismo, y que se pueda
 * capturar cualquier cuadro para revisarlo.
 */
#ifndef ROOTKIT_SCREEN_H
#define ROOTKIT_SCREEN_H

#include "../gfx/fb.h"
#include "../core/mood.h"

#define RK_MAX_PLANTS 6

typedef struct {
    char                 nombre[18];    /* como la bautizó el usuario     */
    const rk_species_t  *sp;
    rk_telemetry_t       tel;
    rk_mood_state_t      mst;
    rk_verdict_t         verdict;
} rk_plant_t;

typedef struct {
    rk_plant_t plants[RK_MAX_PLANTS];
    int        count;
    int        selected;
    bool       wifi;
    uint8_t    hub_batt_pct;   /* batería del Spore seleccionado */
} rk_state_t;

void rk_screen_draw(rk_fb_t *fb, const rk_state_t *st, uint32_t t_ms);

/* Devuelve el índice de planta que corresponde al toque, o -1 si el toque
 * no cayó sobre la tira selectora. Coordenadas en pixeles lógicos. */
int rk_screen_hit(const rk_state_t *st, int x, int y);

#endif /* ROOTKIT_SCREEN_H */
