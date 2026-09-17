/* despertar.h — los ojos se abren por primera vez.
 *
 * DÓNDE CAE EN EL FLUJO
 *
 *   QR  ->  vinculado, esperando el cofre  ->  DESPERTAR  ->  la cara
 *
 * La sorpresa de qué piel te tocó ocurre en el teléfono, cuando abrís el
 * cofre. El aparato no la repite: sería contar dos veces el mismo chiste, y
 * la pantalla chica perdería contra la animación grande de la app. Lo que
 * hace el aparato es la consecuencia: en el instante en que el cofre se abre,
 * la maceta abre los ojos, ya con los colores de esa piel.
 *
 * Por eso no hay texto, ni nombre, ni rareza. La pantalla del ROOTKIT sólo
 * muestra dos cosas en toda su vida —el QR y los ojos— y esta escena es la
 * bisagra entre las dos.
 *
 * LA ESCENA, EN 2,6 SEGUNDOS
 *
 *   0 - 500 ms    negro con un latido del fondo de la piel
 *   500 - 900     la piel entra, con los ojos cerrados
 *   900 - 2000    los párpados se levantan con dos intentos: se abren un
 *                 poco, se vuelven a cerrar, y se abren del todo. Es lo que
 *                 hace cualquiera que se despierta, y es lo que lo vuelve
 *                 un personaje y no una transición.
 *   2000 - 2600   mira a los costados, contento
 *
 * Todo es función pura de (persona, rareza, tiempo).
 */
#ifndef ROOTKIT_DESPERTAR_H
#define ROOTKIT_DESPERTAR_H

#include <stdbool.h>
#include "../gfx/fb.h"
#include "../core/persona.h"

#define RK_DESP_NEGRO_MS    500u
#define RK_DESP_PIEL_MS     900u
#define RK_DESP_OJOS_MS    2000u
#define RK_DESP_FIN_MS     2600u

/* Cuánto tapan los párpados en el instante t: 100 cerrados, 0 abiertos. */
uint8_t rk_despertar_cierre(uint32_t t_ms);

bool rk_despertar_termino(uint32_t t_ms);

/* Un cuadro. `p` puede ser NULL: cae en el primer Rooti. */
void rk_despertar_draw(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                       uint32_t t_ms);

#endif /* ROOTKIT_DESPERTAR_H */
