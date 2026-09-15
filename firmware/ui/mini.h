/* mini.h — composición de la pantalla del Mini.
 *
 * 128x128 sobre un panel de 25,9 mm. Es un cuarto de los pixeles del Prime y
 * un tercio de su área, así que no es la misma pantalla achicada: es otra
 * pantalla, con otra jerarquía.
 *
 * LO QUE EL MINI MUESTRA, Y LO QUE NO
 *
 * El Mini está en la maceta, se mira de reojo al pasar y desde un metro de
 * distancia. Su trabajo es una sola pregunta: ¿esta planta está bien? Todo
 * lo que no conteste eso es ruido que le cuesta batería.
 *
 * Por eso el brote se lleva el 70% del alto y lo único que lo acompaña es la
 * frase del simbionte y tres barritas. No hay selector, no hay menú, no hay
 * historial: eso vive en el Prime, que está enchufado y se mira de frente.
 *
 *   0  .. 13   barra      nombre abreviado y celda
 *   14 ..101   escena     el BROTE a 2x, 12,9 mm, con sus hojas de etapa
 *   102..117   frase      lo que dice, a escala 1
 *   118..127   tira       tierra, clima y luz, tres barras sin números
 *
 * POR QUÉ NO HAY NÚMEROS
 *
 * Un "34%" en 1 mm de alto no se lee, y si se leyera no significaría nada
 * sin el rango de la especie al lado. Tres barras con la zona cómoda marcada
 * dicen lo mismo en un tercio del espacio y se entienden de un vistazo. El
 * número exacto está en el Prime y en el Hub, que es donde alguien lo va a
 * ir a buscar de verdad.
 */
#ifndef ROOTKIT_MINI_H
#define ROOTKIT_MINI_H

#include "../gfx/fb.h"
#include "../gfx/panel.h"
#include "../core/node.h"

/* Dibuja la pantalla de un Mini. `n` es SU nodo, no el kit: un Mini no sabe
 * de los demás y no tiene por qué. */
void rk_mini_draw(rk_fb_t *fb, const rk_node_t *n, uint32_t t_ms);

/* Pantalla de emparejamiento, mientras el Mini todavía no tiene planta
 * asignada. Muestra el código corto que el usuario tipea en el Hub. */
void rk_mini_emparejar(rk_fb_t *fb, const uint8_t id[6], uint32_t t_ms);

#endif /* ROOTKIT_MINI_H */
