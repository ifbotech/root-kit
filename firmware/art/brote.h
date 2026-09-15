/* brote.h — el simbionte cría, que vive en cada Mini.
 *
 * El brote no es el adulto reducido: invierte sus proporciones. Donde el
 * adulto es caparazón con una cabeza asomando, el brote es cabeza con un
 * caparazón asomando. Cabeza grande, ojos grandes y bajos, cuerpo chico —el
 * esquema infantil— es lo que hace que 32x32 se lean como "la cría del mismo
 * bicho" y no como "otro bicho más chico".
 *
 * A escala 2 sobre el panel de 128x128 mide 64 px, o sea 12,9 mm: la mitad
 * larga del adulto del Prime. Esa diferencia de porte es la que cuenta la
 * jerarquía del producto sin una palabra de interfaz.
 *
 * CÓMO SE MUESTRA EL CRECIMIENTO
 *
 * El cuerpo NO cambia con la etapa. Doce simbiontes por cinco etapas serían
 * sesenta sprites, y sobre todo el crecimiento quedaría invisible: a 32x32
 * un cuerpo 15% más grande no se nota. En cambio crece lo que rodea al
 * brote: le van saliendo hojas alrededor, una por etapa, y en las dos
 * últimas se le enciende un aura del color de su acento. Eso sí se ve de
 * reojo desde el otro lado de la habitación, que es el único lugar desde el
 * que alguien mira una maceta.
 */
#ifndef ROOTKIT_BROTE_H
#define ROOTKIT_BROTE_H

#include "../gfx/fb.h"
#include "../core/mood.h"
#include "../core/companion.h"
#include "sprites.h"

/* Dibuja el brote centrado en (cx, cy) a escala `esc`.
 *
 * `comp` puede ser NULL: sale el cuerpo del primer simbionte, que es lo que
 * corresponde mientras la identificación por foto está en curso.
 *
 * Función pura de (comp, mood, sev, etapa, esc, t_ms). */
void rk_brote_draw(rk_fb_t *fb, int cx, int cy, const rk_companion_t *comp,
                   rk_mood_t mood, rk_severity_t sev, rk_stage_t etapa,
                   int esc, uint32_t t_ms);

/* Cuántas hojas de crecimiento le corresponden a una etapa. Expuesto porque
 * lo usan los tests y la ficha de la colección en el Prime. */
int  rk_brote_hojas(rk_stage_t etapa);
/* ¿La etapa enciende aura? A partir de MADURO. */
bool rk_brote_aura(rk_stage_t etapa);

#endif /* ROOTKIT_BROTE_H */
