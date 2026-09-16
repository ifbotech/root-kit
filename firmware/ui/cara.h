/* cara.h — la pantalla del ROOTKIT.
 *
 * Es una sola cosa: la cara, a sangre, ocupando todo el panel.
 *
 * QUÉ SE FUE DE ACÁ, Y POR QUÉ
 *
 * Esta pantalla antes mostraba nombre, wifi, batería, una frase, tres
 * medidores con su zona de confort y una tira selectora. Todo eso se mudó a
 * la app. El aparato que está en la maceta se mira de reojo, al pasar, desde
 * un metro: contesta una sola pregunta —¿cómo está mi planta?— y la contesta
 * con una cara. Los números los va a buscar alguien que ya decidió
 * preocuparse, y esa persona tiene el teléfono en la mano.
 *
 * Sacar la interfaz también es lo que hace que la carcasa impresa mande.
 * Una pantalla llena de barras compite con el objeto; una cara lo completa.
 *
 * Quedan exactamente dos elementos además de la cara, y los dos aparecen
 * sólo cuando hacen falta:
 *
 *   - un PICTOGRAMA en la esquina si hay algo que pedir (gota, sol, copo).
 *     La cara dice que algo anda mal; el pictograma dice qué, sin idioma.
 *   - un AVISO DE BATERÍA cuando la celda está por terminarse. Es lo único
 *     que la app no puede resolver sola, porque para verlo hay que ir.
 */
#ifndef ROOTKIT_CARA_H
#define ROOTKIT_CARA_H

#include "../gfx/fb.h"
#include "../gfx/panel.h"
#include "../core/node.h"

/* La pantalla en régimen. `n` es el nodo propio. */
void rk_cara_draw(rk_fb_t *fb, const rk_node_t *n, uint32_t t_ms);

/* Pantalla de emparejamiento: todavía no sabe qué planta cuida. Muestra el
 * código corto que se carga en la app. */
void rk_cara_emparejar(rk_fb_t *fb, const uint8_t id[6],
                       const rk_persona_t *p, uint32_t t_ms);

#endif /* ROOTKIT_CARA_H */
