/* eyes.h — la cara del simbionte en la OLED de 128x32 del Spore.
 *
 * En la maceta no entra el bicho entero: entran los ojos. Y alcanza, porque
 * los ojos son donde vive toda la expresión — el Tamagotchi original era
 * monocromo de 32x16 y nadie dudaba de lo que sentía.
 *
 * Todo es procedural, no sprites. A esta escala las formas son simples
 * (óvalos, discos, arcos) y en cambio se gana lo que un flipbook no da: la
 * pupila puede mirar a cualquier lado, el párpado puede cerrarse a cualquier
 * altura, y el parpadeo cae donde tiene que caer en vez de en el cuadro que
 * tocaba. Eso es lo que hace que parezca vivo y no una animación en bucle.
 *
 * La función es pura respecto del tiempo: el mismo instante da el mismo
 * cuadro, lo que permite testear la cara por hash igual que la Terminal.
 */
#ifndef ROOTKIT_EYES_H
#define ROOTKIT_EYES_H

#include "../gfx/mono.h"
#include "../core/mood.h"
#include "../core/companion.h"

/* Dibuja la cara completa para un ánimo y un instante. */
void rk_eyes_draw(rk_mono_t *m, rk_mood_t mood, uint32_t t_ms);

/* Igual, pero con la etapa de crecimiento: los ojos se agrandan y se separan
 * a medida que el simbionte madura, que es la forma más barata y más legible
 * de mostrar en monocromo que el bicho creció. */
void rk_eyes_draw_stage(rk_mono_t *m, rk_mood_t mood, rk_stage_t etapa,
                        uint32_t t_ms);

/* Pantalla de arranque del Spore: el nombre del simbionte y su etapa.
 * Se muestra un segundo al despertar por un toque, no de forma continua. */
void rk_eyes_splash(rk_mono_t *m, const char *nombre, rk_stage_t etapa);

/* Presupuesto de pixeles encendidos, y de dónde sale el número.
 *
 * En una OLED sólo consume lo que está encendido. Un SSD1306 de 128x32 con
 * el panel entero prendido pide unos 20 mA; a 1.200 pixeles de 4.096 —menos
 * de un tercio— quedan alrededor de 7 mA. Con la cara visible dos minutos por
 * día eso son ~230.000 nAh diarios sobre los 1.900.000 del Spore, o sea que
 * la autonomía baja de 926 días a unos 750: aceptable.
 *
 * Duplicar este número llevaría el consumo a 14 mA y la autonomía a poco más
 * de 600 días, que ya no lo es. De ahí el tope, y de ahí que los tests
 * recorran todos los ánimos en todas las etapas verificándolo. */
#define RK_EYES_MAX_PIXELES 1200

#endif /* ROOTKIT_EYES_H */
