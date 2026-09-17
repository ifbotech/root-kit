/* golden_util.h — escenario fijo y hash de cuadro, compartidos entre el test
 * de regresión visual y el generador de golden.h.
 *
 * Viven aparte para romper la dependencia circular: gen_golden necesita
 * renderizar sin que golden.h exista todavía.
 *
 * La tabla de referencia es una matriz de ROOTIES x PIELES x ÁNIMOS. No
 * alcanza con fijar los once ánimos de uno: el rig es procedural y cada
 * familia de ojos, cada brillo y cada boca toma un camino distinto, y cada
 * piel mezcla colores distintos. Ciento sesenta y cinco hashes suenan a
 * mucho hasta que uno se acuerda de que son ciento sesenta y cinco caras.
 */
#ifndef ROOTKIT_GOLDEN_UTIL_H
#define ROOTKIT_GOLDEN_UTIL_H

#include <stdint.h>
#include "../gfx/fb.h"
#include "../gfx/panel.h"
#include "../ui/cara.h"
#include "../core/mood.h"
#include "../core/node.h"
#include "../core/persona.h"

/* FNV-1a de 32 bits sobre el framebuffer. */
uint32_t rk_frame_hash(const rk_color_t *px, int n);

/* Un nodo determinista con el Rooti `persona`, la piel `rareza` y el ánimo
 * forzado. */
void rk_golden_nodo(rk_node_t *n, int persona, int rareza, rk_mood_t mood);

/* Renderiza la pantalla completa de ese nodo y devuelve el hash. */
uint32_t rk_golden_cara(int persona, int rareza, rk_mood_t mood, uint32_t t_ms);

/* Instante de animación fijo, para que el hash no dependa de un reloj. */
#define RK_GOLDEN_T_MS 1200u

#endif /* ROOTKIT_GOLDEN_UTIL_H */
