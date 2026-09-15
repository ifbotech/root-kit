/* golden_util.h — escenario fijo y hash de cuadro, compartidos entre los
 * tests de regresión visual y el generador de golden.h.
 *
 * Viven aparte para romper la dependencia circular: gen_golden necesita
 * renderizar sin que golden.h exista todavía.
 *
 * Hay DOS escenarios y DOS tablas de referencia, uno por panel. Un cambio en
 * el rig del brote no mueve un solo pixel del Prime y viceversa, así que un
 * único hash global diría "algo cambió" sin decir dónde, que es exactamente
 * la información que uno necesita al revisar un cambio de arte.
 */
#ifndef ROOTKIT_GOLDEN_UTIL_H
#define ROOTKIT_GOLDEN_UTIL_H

#include <stdint.h>
#include "../gfx/fb.h"
#include "../gfx/panel.h"
#include "../ui/prime.h"
#include "../ui/mini.h"
#include "../core/mood.h"
#include "../core/node.h"

/* FNV-1a de 32 bits sobre el framebuffer. */
uint32_t rk_frame_hash(const rk_color_t *px, int n);

/* Escenario determinista: un Prime y tres Minis con datos fijos, el nodo
 * `nodo` forzado al ánimo pedido. La única fuente de variación es el código
 * de dibujo. Los vínculos tienen edades distintas a propósito, para que el
 * escenario ejercite varias etapas de crecimiento. */
void rk_golden_kit(rk_roster_t *r, int nodo, rk_mood_t mood);

/* Renderiza y devuelve el hash, uno por panel. */
uint32_t rk_golden_prime(rk_mood_t mood, uint32_t t_ms);
uint32_t rk_golden_mini(rk_mood_t mood, uint32_t t_ms);

/* Instante de animación usado para cada cuadro de referencia. Fijo, para que
 * el hash no dependa de un reloj. */
#define RK_GOLDEN_T_MS 1200u

#endif /* ROOTKIT_GOLDEN_UTIL_H */
