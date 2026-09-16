/* golden_util.h — escenario fijo y hash de cuadro, compartidos entre el test
 * de regresión visual y el generador de golden.h.
 *
 * Viven aparte para romper la dependencia circular: gen_golden necesita
 * renderizar sin que golden.h exista todavía.
 *
 * La tabla de referencia es una matriz de MODELOS x ÁNIMOS. No alcanza con
 * fijar los once ánimos de un modelo: el rig es procedural y cada familia de
 * ojos toma un camino distinto, así que un cambio puede romper el visor sin
 * tocar al ciclope. Sesenta y seis hashes suenan a mucho hasta que uno se
 * acuerda de que son sesenta y seis caras distintas.
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

/* Un nodo determinista con el modelo `persona` y el ánimo forzado. */
void rk_golden_nodo(rk_node_t *n, int persona, rk_mood_t mood);

/* Renderiza la pantalla completa de ese nodo y devuelve el hash. */
uint32_t rk_golden_cara(int persona, rk_mood_t mood, uint32_t t_ms);

/* Instante de animación fijo, para que el hash no dependa de un reloj. */
#define RK_GOLDEN_T_MS 1200u

#endif /* ROOTKIT_GOLDEN_UTIL_H */
