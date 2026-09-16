/* cara.h — la pantalla del ROOTKIT cuando ya sabe quién es.
 *
 * LA PANTALLA MUESTRA DOS COSAS EN TODA SU VIDA: EL QR Y LOS OJOS
 *
 * Nada de números, íconos, batería, wifi ni texto. Todo eso vive en la app.
 * El aparato que está en la maceta se mira de reojo, al pasar, desde un
 * metro: contesta una sola pregunta —¿cómo está mi planta?— y la contesta
 * con una cara. Los números los va a buscar alguien que ya decidió
 * preocuparse, y esa persona tiene el teléfono en la mano.
 *
 * Sacar la interfaz también es lo que hace que la carcasa impresa mande.
 * Una pantalla llena de barras compite con el objeto; una cara lo completa.
 *
 * Hasta la batería baja se avisa por la app (una notificación) y con la
 * cara: con poca carga la cara se pone somnolienta. Ver rk_cara_draw.
 *
 * LOS ESTADOS DE LA PANTALLA
 *
 *   ui/qr.h         sin vincular: el QR y el código corto
 *   rk_cara_dormida vinculado, con el cofre todavía cerrado en la app
 *   ui/despertar.h  el cofre se abrió: los ojos se abren por primera vez
 *   rk_cara_draw    de ahí en adelante, siempre
 */
#ifndef ROOTKIT_CARA_H
#define ROOTKIT_CARA_H

#include "../gfx/fb.h"
#include "../gfx/panel.h"
#include "../core/node.h"

/* La pantalla en régimen. `n` es el nodo propio; NULL dibuja una cara sin
 * datos. */
void rk_cara_draw(rk_fb_t *fb, const rk_node_t *n, uint32_t t_ms);

/* Vinculado pero con el cofre cerrado: unos ojos dormidos, en gris, que
 * respiran. No usa los colores del personaje porque todavía no se reveló:
 * mostrarlos en la maceta arruinaría la sorpresa del cofre. */
void rk_cara_dormida(rk_fb_t *fb, uint32_t t_ms);

/* La persona neutra con la que se dibuja la cara dormida. */
extern const rk_persona_t rk_persona_incognita;

#endif /* ROOTKIT_CARA_H */
