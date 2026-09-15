/* prime.h — composición de la pantalla del Prime.
 *
 * Una sola función pura: dado el kit y un instante, escribe el cuadro. Sin
 * estado propio, sin efectos secundarios. Eso hace que el simulador y la
 * placa rendericen exactamente lo mismo, y que se pueda capturar cualquier
 * cuadro para revisarlo.
 *
 * EL PANEL, Y LAS CINCO BANDAS
 *
 * 240x320 nativo, sin lienzo lógico intermedio. De arriba a abajo:
 *
 *   0  .. 33   barra      nombre del nodo, enlace, wifi, batería
 *   34 ..209   escena     el ADULTO a 2x, 22 mm, con su fondo y sus efectos
 *   210..241   diálogo    lo que el simbionte dice, en primera persona
 *   242..283   datos      tierra, clima y luz con su zona de confort
 *   284..319   selector   una ficha por nodo del kit
 *
 * Las proporciones no son arbitrarias: la escena se lleva el 55% del alto
 * porque el producto es la criatura, no el panel de control. Los medidores
 * están abajo y son lo primero que se sacrifica cuando algo tiene que
 * achicarse.
 */
#ifndef ROOTKIT_PRIME_H
#define ROOTKIT_PRIME_H

#include "../gfx/fb.h"
#include "../gfx/panel.h"
#include "../core/node.h"

void rk_prime_draw(rk_fb_t *fb, const rk_roster_t *r, uint32_t t_ms);

/* Índice de nodo que corresponde a un toque, o -1 si el toque no cayó sobre
 * la tira selectora. Coordenadas en pixeles del panel. */
int  rk_prime_hit(const rk_roster_t *r, int x, int y);

#endif /* ROOTKIT_PRIME_H */
