/* face.h — la cara. Lo único que se dibuja en el ROOTKIT.
 *
 * POR QUÉ PROCEDURAL Y NO SPRITES
 *
 * Seis modelos por once ánimos son sesenta y seis caras. Dibujarlas a mano
 * sería un mes de trabajo y un archivo de datos enorme, y peor todavía:
 * agregar un modelo costaría once caras más. Acá una cara es un puñado de
 * elipses, arcos y trazos, y un modelo es una fila de parámetros.
 *
 * A este tamaño eso no es una concesión sino una ventaja. Un ojo ocupa unos
 * 35 px de ancho: suficiente para que una curva calculada se vea deliberada.
 * Y se gana lo que un flipbook no da —la pupila mira a cualquier lado, el
 * párpado cierra a cualquier altura, el parpadeo cae cuando tiene que caer—
 * que es lo que separa "una animación en bucle" de "algo que está vivo".
 *
 * CÓMO SE COMBINAN PERSONA Y ÁNIMO
 *
 *   core/mood.c   dice QUÉ siente  -> rk_look() da la forma semántica
 *   core/persona.c dice CÓMO lo muestra -> familia de ojos, cejas, boca
 *   art/face.c    los cruza y dibuja
 *
 * Ninguna de las dos tablas conoce a la otra. Agregar un ánimo no toca las
 * personas; agregar una persona no toca los ánimos.
 *
 * LA CARA OCUPA TODA LA PANTALLA
 *
 * No hay barras, ni números, ni nombre. Esos datos viven en la app, que es
 * donde alguien los va a ir a leer de verdad. El aparato en la maceta
 * contesta una sola pregunta —¿cómo está mi planta?— y la contesta con una
 * cara, que es lo que se lee de reojo al pasar y desde un metro.
 *
 * Lo único que acompaña a la cara es un pictograma chico, y sólo cuando hay
 * un problema: la cara dice que algo anda mal, el pictograma dice qué.
 */
#ifndef ROOTKIT_FACE_H
#define ROOTKIT_FACE_H

#include "../gfx/fb.h"
#include "../core/mood.h"
#include "../core/persona.h"

/* Dibuja la cara completa, incluido el fondo, ocupando todo `fb`.
 *
 * `p` puede ser NULL: sale el primer modelo, que es lo que corresponde
 * mientras el aparato todavía no sabe qué carcasa lleva puesta.
 *
 * `adornos_extra` son los que agrega el crecimiento (aura, corona) sobre los
 * que la persona ya trae. Ver rk_face_adornos_etapa().
 *
 * Función pura de (p, mood, sev, adornos_extra, t_ms): el mismo instante
 * siempre produce el mismo cuadro, y por eso se testea por hash. */
void rk_face_draw(rk_fb_t *fb, const rk_persona_t *p,
                  rk_mood_t mood, rk_severity_t sev,
                  uint8_t adornos_extra, uint32_t t_ms);

/* Sólo los rasgos, sin fondo, centrados en (cx, cy) y escalados a `ancho`
 * pixeles de cara. Lo usa la ficha de la app y la revelación de la caja. */
void rk_face_rasgos(rk_fb_t *fb, int cx, int cy, int ancho,
                    const rk_persona_t *p, rk_mood_t mood,
                    uint8_t adornos_extra, uint32_t t_ms);

/* Qué adornos desbloquea cada etapa del vínculo. Es lo que quedó de la
 * mecánica de mérito cuando la rareza se mudó a la caja física: el modelo
 * te toca por azar, pero cómo se ve se gana cuidando la planta. */
uint8_t rk_face_adornos_etapa(int etapa);

/* Pictograma de qué necesita la planta, para la esquina. Devuelve false si
 * el ánimo no tiene nada que pedir. */
bool rk_face_pictograma(rk_fb_t *fb, int x, int y, int lado,
                        rk_mood_t mood, rk_color_t c);

#endif /* ROOTKIT_FACE_H */
