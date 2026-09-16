/* revelado.h — la primera vez que el aparato se ve la cara.
 *
 * QUÉ CAMBIÓ RESPECTO DE LA CEREMONIA ANTERIOR
 *
 * Antes esto era un gachapón: una cápsula caía, temblaba y estallaba, y de
 * adentro salía un simbionte elegido por la especie de la planta. Toda la
 * sorpresa era software.
 *
 * Ahora la sorpresa ya ocurrió antes de encender: pasó cuando abriste la
 * caja y viste qué carcasa te tocó. Repetir el gachapón en pantalla sería
 * contar dos veces el mismo chiste, y peor, el segundo sería el falso.
 *
 * Así que la escena cambia de sentido. No es "mirá lo que te tocó" sino
 * **"ah, entonces soy este"**: el aparato despierta a oscuras, se descubre
 * los rasgos de a uno, y termina presentándose con su nombre. El usuario ya
 * sabe qué modelo tiene en la mano; lo que ve en pantalla es a la cosa
 * enterándose.
 *
 * Es más corto que la ceremonia vieja —5,2 s contra 3,6— porque acompaña un
 * momento que ya pasó en vez de intentar ser el momento.
 *
 * Toda la animación es función pura de (persona, tiempo).
 */
#ifndef ROOTKIT_REVELADO_H
#define ROOTKIT_REVELADO_H

#include "../gfx/fb.h"
#include "../core/persona.h"

/* Hitos, en milisegundos desde que arranca. */
#define RK_REV_NEGRO     500u   /* oscuridad y un latido                  */
#define RK_REV_OJOS     1500u   /* los ojos se abren por primera vez      */
#define RK_REV_RASGOS   2400u   /* aparecen cejas, boca y adornos         */
#define RK_REV_NOMBRE   3600u   /* se presenta                            */

typedef enum {
    RK_REV_FASE_NEGRO = 0,
    RK_REV_FASE_OJOS,
    RK_REV_FASE_RASGOS,
    RK_REV_FASE_NOMBRE
} rk_rev_fase_t;

rk_rev_fase_t rk_revelado_fase(uint32_t t_ms);
bool          rk_revelado_termino(uint32_t t_ms);

/* Un cuadro. `p` puede ser NULL: cae en el primer modelo. */
void rk_revelado_draw(rk_fb_t *fb, const rk_persona_t *p, uint32_t t_ms);

#endif /* ROOTKIT_REVELADO_H */
