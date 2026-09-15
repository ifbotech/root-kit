/* gacha.h — la ceremonia de apertura.
 *
 * Cuando se registra una planta se abre una cápsula y aparece el simbionte
 * que la habita. Es el momento que el usuario va a recordar del producto, así
 * que tiene su propia pantalla, su propio ritmo y destellos graduados por
 * rareza.
 *
 * Lo único que NO tiene es azar. Qué simbionte sale ya está decidido por la
 * especie antes de que la cápsula aparezca en pantalla —ver companion.h— y la
 * ceremonia es presentación pura. Eso conserva entero el momento y mantiene
 * el producto fuera del terreno regulado de las cajas de botín.
 *
 * Toda la animación es función pura de (rareza, tiempo): el mismo instante da
 * el mismo cuadro, y se testea por hash igual que el resto de la pantalla.
 */
#ifndef ROOTKIT_GACHA_H
#define ROOTKIT_GACHA_H

#include "../gfx/fb.h"
#include "../core/companion.h"

/* Hitos de la ceremonia, en milisegundos desde que arranca. */
#define RK_GACHA_CAIDA     700u    /* la cápsula entra y se asienta      */
#define RK_GACHA_TEMBLOR  1700u    /* tiembla, se abren grietas de luz   */
#define RK_GACHA_ESTALLIDO 2300u   /* estalla: destellos según rareza    */
#define RK_GACHA_REVELADO  4100u   /* el simbionte sube con su cartel    */
#define RK_GACHA_FIN       5200u   /* queda en reposo esperando un toque */

typedef enum {
    RK_GACHA_FASE_CAIDA = 0,
    RK_GACHA_FASE_TEMBLOR,
    RK_GACHA_FASE_ESTALLIDO,
    RK_GACHA_FASE_REVELADO,
    RK_GACHA_FASE_REPOSO
} rk_gacha_fase_t;

rk_gacha_fase_t rk_gacha_fase(uint32_t t_ms);
bool            rk_gacha_termino(uint32_t t_ms);

/* Dibuja un cuadro de la ceremonia. `c` puede ser NULL, en cuyo caso se
 * muestra una cápsula genérica: útil mientras la identificación por foto
 * todavía está en curso. */
void rk_gacha_draw(rk_fb_t *fb, const rk_companion_t *c, uint32_t t_ms);

/* Color característico de cada rareza. Se usa en la ceremonia, en el borde de
 * la ficha de la colección y en el cartel: una sola escala para todo. */
rk_color_t rk_rarity_color(rk_rarity_t r);

#endif /* ROOTKIT_GACHA_H */
