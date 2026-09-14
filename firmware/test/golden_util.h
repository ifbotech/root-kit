/* golden_util.h — escenario fijo y hash de cuadro, compartidos entre el test
 * de regresión visual y el generador de golden.h.
 *
 * Viven aparte para romper la dependencia circular: gen_golden necesita
 * renderizar sin que golden.h exista todavía.
 */
#ifndef ROOTKIT_GOLDEN_UTIL_H
#define ROOTKIT_GOLDEN_UTIL_H

#include <stdint.h>
#include "../gfx/fb.h"
#include "../ui/screen.h"
#include "../core/mood.h"

/* FNV-1a de 32 bits sobre el framebuffer. */
uint32_t rk_frame_hash(const rk_color_t *px, int n);

/* Escenario determinista: tres plantas con datos fijos, la primera forzada
 * al ánimo pedido. La única fuente de variación es el código de dibujo. */
void rk_golden_state(rk_state_t *st, rk_mood_t mood);

/* Renderiza y devuelve el hash. */
uint32_t rk_golden_render(rk_mood_t mood, uint32_t t_ms);

/* Instante de animación usado para cada cuadro de referencia. Fijo, para que
 * el hash no dependa de un reloj. */
#define RK_GOLDEN_T_MS 1200u

#endif /* ROOTKIT_GOLDEN_UTIL_H */
