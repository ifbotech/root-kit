/* tuga.h — Tuga.exe, el primer simbionte de la colección.
 *
 * No es un flipbook de cuadros dibujados: es un rig. Un cuerpo, un juego de
 * ojos, un juego de bocas y una capa de efectos que se combinan según el
 * estado de ánimo. Eso da muchas más expresiones que cuadros dibujados, y
 * sobre todo permite que el próximo simbionte reutilice todo el sistema
 * cambiando sólo el cuerpo.
 */
#ifndef ROOTKIT_TUGA_H
#define ROOTKIT_TUGA_H

#include "../gfx/fb.h"
#include "../core/mood.h"

/* Dibuja a Tuga centrada en (cx, cy) del framebuffer.
 * t_ms es el tiempo de animación; la función es pura respecto de él, así que
 * el mismo instante siempre produce el mismo cuadro. */
void rk_tuga_draw(rk_fb_t *fb, int cx, int cy,
                  rk_mood_t mood, rk_severity_t sev, uint32_t t_ms);

/* Los efectos ambientales (burbujas, nieve, rayos, glitch) se dibujan sobre
 * toda la escena, no sólo sobre el bicho. Se llama antes de rk_tuga_draw
 * para lo que va detrás y después para lo que va adelante. */
void rk_tuga_fx_back(rk_fb_t *fb, int x, int y, int w, int h,
                     rk_mood_t mood, uint32_t t_ms);
void rk_tuga_fx_front(rk_fb_t *fb, int x, int y, int w, int h,
                      rk_mood_t mood, uint32_t t_ms);

/* Color ambiente de la escena para cada ánimo: el fondo también actúa. */
void rk_tuga_scene_colors(rk_mood_t mood, rk_color_t *top, rk_color_t *bottom);

#endif /* ROOTKIT_TUGA_H */
