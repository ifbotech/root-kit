/* adulto.h — el simbionte adulto, que vive en el Prime.
 *
 * No es un flipbook de cuadros dibujados: es un rig. Un cuerpo, un juego de
 * ojos, un juego de bocas y una capa de efectos que se combinan según el
 * estado de ánimo. Eso da muchas más expresiones que cuadros dibujados, y
 * sobre todo permite que los doce simbiontes compartan todo el sistema
 * cambiando sólo la paleta.
 *
 * El arte mide 96x72 y se dibuja a escala 2 sobre el panel de 240x320: 144
 * pixeles de alto, 21,9 mm de criatura. Ese número no es casual, es el que
 * separa "icono de estado" de "personaje" a distancia de escritorio.
 */
#ifndef ROOTKIT_ADULTO_H
#define ROOTKIT_ADULTO_H

#include "../gfx/fb.h"
#include "../core/mood.h"
#include "../core/companion.h"
#include "sprites.h"

/* Dibuja al adulto centrado en (cx, cy), a escala `esc`.
 *
 * `comp` puede ser NULL: sale con la paleta del primer simbionte, que es lo
 * que corresponde mientras la identificación por foto todavía está en curso.
 *
 * t_ms es el tiempo de animación; la función es pura respecto de él, así que
 * el mismo instante siempre produce el mismo cuadro. */
void rk_adulto_draw(rk_fb_t *fb, int cx, int cy, const rk_companion_t *comp,
                    rk_mood_t mood, rk_severity_t sev, int esc, uint32_t t_ms);

/* Los efectos ambientales (burbujas, nieve, rayos, glitch) se dibujan sobre
 * toda la escena, no sólo sobre el bicho. Se llama antes de rk_adulto_draw
 * para lo que va detrás y después para lo que va adelante. */
void rk_adulto_fx_back(rk_fb_t *fb, int x, int y, int w, int h,
                       rk_mood_t mood, uint32_t t_ms);
void rk_adulto_fx_front(rk_fb_t *fb, int x, int y, int w, int h,
                        rk_mood_t mood, uint32_t t_ms);

/* Color ambiente de la escena para cada ánimo: el fondo también actúa. */
void rk_escena_colores(rk_mood_t mood, rk_color_t *top, rk_color_t *bottom);

/* La paleta de un simbionte. Nunca devuelve NULL: con `comp` NULL o
 * desconocido cae en la paleta 0. */
const rk_color_t *rk_companion_pal(const rk_companion_t *comp);

#endif /* ROOTKIT_ADULTO_H */
