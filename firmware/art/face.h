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
 *
 * LA TRANSICIÓN ENTRE ÁNIMOS
 *
 * Una cara que salta de contenta a sedienta en un cuadro se ve de máquina de
 * estados. La de verdad cambia en un tercio de segundo: los párpados bajan,
 * la boca se afloja, la mirada se mueve. Para eso la expresión se separa en
 * dos partes: la GEOMETRÍA (rk_face_geom_t: cuánto abre el ojo, cuánto tapa
 * el párpado, cuánto curva la boca...) que se interpola, y lo DISCRETO (un
 * ojo en cruz, una lengua afuera) que cambia a mitad de camino, escondido
 * detrás de un parpadeo. rk_face_draw_mezcla dibuja cualquier punto
 * intermedio entre dos ánimos; quién lleva el reloj de la transición es
 * ui/cara.h.
 */
#ifndef ROOTKIT_FACE_H
#define ROOTKIT_FACE_H

#include "../gfx/fb.h"
#include "../core/mood.h"
#include "../core/persona.h"

/* La geometría continua de una expresión. Son los diales que se pueden
 * girar de a poco entre dos caras; todos en unidades relativas (100 =
 * normal, 0..100 para los párpados, -100..100 para mirada y boca). */
typedef struct {
    int abre;         /* apertura vertical del ojo, 100 = normal          */
    int pupila;       /* radio de la pupila, 100 = normal                  */
    int tapa_sup;     /* párpado de arriba: cuánto cubre, 0..100           */
    int tapa_ang;     /* inclinación del párpado de arriba                 */
    int tapa_inf;     /* párpado de abajo: cuánto sube, 0..100             */
    int mira_x;       /* hacia dónde mira la pupila, -100..100             */
    int mira_y;
    int ceja_ang;     /* inclinación extra de la ceja                      */
    int ceja_dy;      /* altura extra de la ceja                           */
    int boca_curva;   /* -100 mueca .. 0 recta .. 100 sonrisa              */
} rk_face_geom_t;

/* Interpola cada dial entre `a` (t=0) y `b` (t=100). Lineal: la curva del
 * tiempo la pone rk_face_ease. */
rk_face_geom_t rk_face_geom_lerp(const rk_face_geom_t *a,
                                 const rk_face_geom_t *b, uint8_t t_pct);

/* Arranca y termina suave (smoothstep), en enteros: 0->0, 50->50, 100->100,
 * y en el medio más lento en las puntas que en el centro. */
uint8_t rk_face_ease(uint8_t t_pct);

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

/* Un punto intermedio entre dos ánimos: `t_pct` 0 es exactamente `desde`,
 * 100 es exactamente `hacia`. La geometría se interpola; los colores, la
 * respiración también; lo que no se puede interpolar cambia a la mitad,
 * tapado por un parpadeo. `t_pct` ya viene con su curva (rk_face_ease). */
void rk_face_draw_mezcla(rk_fb_t *fb, const rk_persona_t *p,
                         rk_mood_t desde, rk_mood_t hacia, uint8_t t_pct,
                         rk_severity_t sev, uint8_t adornos_extra,
                         uint8_t cierre, uint32_t t_ms);

/* Qué adornos desbloquea cada etapa del vínculo. El modelo te toca por azar;
 * cómo se ve se gana cuidando la planta. */
uint8_t rk_face_adornos_etapa(int etapa);

/* Color de fondo que usa la cara para un modelo y un ánimo, con el tinte y
 * la penumbra ya aplicados. Lo necesita quien quiera dibujar algo encima sin
 * que se note el borde. */
rk_color_t rk_face_fondo(const rk_persona_t *p, rk_mood_t mood);

#endif /* ROOTKIT_FACE_H */
