/* face.h — los ojitos. Lo único que se dibuja en el ROOTKIT además del QR.
 *
 * ESTILO
 *
 * Ilustración plana del tipo Duolingo: formas grandes y redondas, colores
 * planos, sin contorno negro, bordes suavizados. Ojos enormes con pupila
 * oscura y dos brillos, párpados que cortan el ojo para expresar, cejas como
 * cápsulas gruesas. Nada de pixel art: cada curva pasa por gfx/aa.c.
 *
 * El fondo de la pantalla es la piel del personaje, liso. Los párpados se
 * pintan de ese mismo color, y por eso funcionan: un párpado que baja no es
 * una forma nueva sino piel que tapa parte del ojo. Con un fondo en degradé
 * eso sería imposible, y es la razón de que no lo haya.
 *
 * POR QUÉ PROCEDURAL Y NO IMÁGENES
 *
 * Ocho modelos por once ánimos son ochenta y ocho caras, más parpadeo,
 * respiración y mirada que se mueve. Como imágenes serían cientos de
 * cuadros y megas de flash; acá son un puñado de formas por cuadro. Y un
 * modelo nuevo es una fila en core/persona.c, no sesenta imágenes más.
 *
 * CÓMO SE COMBINAN PERSONA Y ÁNIMO
 *
 *   core/mood.c    QUÉ siente       -> art/look.c da la expresión abstracta
 *   core/persona.c CÓMO lo muestra  -> familia de ojos, cejas, boca, colores
 *   art/face.c     los cruza y dibuja
 *
 * Todo es función pura del tiempo: el mismo instante da el mismo cuadro, y
 * por eso se testea por hash.
 */
#ifndef ROOTKIT_FACE_H
#define ROOTKIT_FACE_H

#include "../gfx/fb.h"
#include "../core/mood.h"
#include "../core/persona.h"

/* Dibuja la cara completa, fondo incluido, ocupando todo `fb`. La cara se
 * centra en el panel y se escala con su lado corto, así que la misma función
 * sirve para 128x128 y para 240x320.
 *
 * `p` puede ser NULL: sale el primer modelo. `adornos_extra` son los que
 * agrega el crecimiento (ver rk_face_adornos_etapa). */
void rk_face_draw(rk_fb_t *fb, const rk_persona_t *p,
                  rk_mood_t mood, rk_severity_t sev,
                  uint8_t adornos_extra, uint32_t t_ms);

/* Igual, con los párpados forzados a cerrarse `cierre` por ciento (0 abiertos,
 * 100 cerrados). Es lo que usa el despertar: los ojos se abren de a poco la
 * primera vez que el aparato sabe quién es. */
void rk_face_draw_cierre(rk_fb_t *fb, const rk_persona_t *p,
                         rk_mood_t mood, rk_severity_t sev,
                         uint8_t adornos_extra, uint8_t cierre, uint32_t t_ms);

/* Qué adornos desbloquea cada etapa del vínculo. El modelo te toca por azar;
 * cómo se ve se gana cuidando la planta. */
uint8_t rk_face_adornos_etapa(int etapa);

/* Color de fondo que usa la cara para un modelo y un ánimo, con el tinte y
 * la penumbra ya aplicados. Lo necesita quien quiera dibujar algo encima sin
 * que se note el borde. */
rk_color_t rk_face_fondo(const rk_persona_t *p, rk_mood_t mood);

#endif /* ROOTKIT_FACE_H */
