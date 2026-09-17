/* face.h — la cara. Lo único que se dibuja en el ROOTKIT además del QR.
 *
 * ESTILO
 *
 * Ooblets + Pokémon Café ReMix: ojos grandes y oscuros con brillos blancos
 * y una media luna más clara abajo, mejillas sonrosadas, bocas chicas y
 * dulces, formas planas y redondas sin contorno negro. Cada curva pasa por
 * gfx/aa.c, que suaviza el borde en punto fijo.
 *
 * El fondo de la pantalla es liso, del color de fondo de la piel del Rooti.
 * Los párpados se pintan de ese mismo color, y por eso funcionan: un
 * párpado que baja no es una forma nueva sino fondo que tapa parte del ojo.
 * Con un fondo en degradé eso sería imposible, y es la razón de que no lo
 * haya.
 *
 * POR QUÉ PROCEDURAL Y NO IMÁGENES
 *
 * Cinco personajes por tres pieles por once ánimos son ciento sesenta y
 * cinco caras, más parpadeo, respiración, mirada, transiciones y guiños.
 * Como imágenes serían megas de flash; acá son un puñado de formas por
 * cuadro. Y una piel nueva es una fila en core/persona.c.
 *
 * CÓMO SE COMBINAN PERSONA, PIEL Y ÁNIMO
 *
 *   core/mood.c    QUÉ siente       -> art/look.c da la expresión abstracta
 *   core/persona.c CÓMO lo muestra  -> ojos, brillo, boca, mejillas
 *                  CON QUÉ COLORES  -> la piel que salió del cofre
 *   art/face.c     los cruza y dibuja
 *
 * Todo es función pura del tiempo: el mismo instante da el mismo cuadro, y
 * por eso se testea por hash.
 *
 * LA TRANSICIÓN ENTRE ÁNIMOS
 *
 * La expresión se separa en GEOMETRÍA (rk_face_geom_t: cuánto abre el ojo,
 * cuánto tapa el párpado, cuánto curva la boca...), que se interpola, y lo
 * DISCRETO (ojos en cruz, lengua afuera), que cambia a mitad de camino
 * escondido detrás de un parpadeo. rk_face_draw_mezcla dibuja cualquier
 * punto intermedio; el reloj lo lleva ui/cara.h.
 */
#ifndef ROOTKIT_FACE_H
#define ROOTKIT_FACE_H

#include "../gfx/fb.h"
#include "../core/mood.h"
#include "../core/persona.h"

/* La geometría continua de una expresión, en unidades relativas (100 =
 * normal, 0..100 para los párpados, -100..100 para mirada y boca). */
typedef struct {
    int abre;         /* apertura vertical del ojo, 100 = normal          */
    int pupila;       /* 100 = el ojo entero oscuro; menos = susto        */
    int tapa_sup;     /* párpado de arriba: cuánto cubre, 0..100           */
    int tapa_ang;     /* inclinación del párpado de arriba                 */
    int tapa_inf;     /* párpado de abajo: cuánto sube, 0..100             */
    int mira_x;       /* hacia dónde mira, -100..100                       */
    int mira_y;
    int ceja_ang;     /* inclinación extra de la ceja                      */
    int ceja_dy;      /* altura extra de la ceja                           */
    int boca_curva;   /* -100 mueca .. 0 recta .. 100 sonrisa              */
} rk_face_geom_t;

/* Interpola cada dial entre `a` (t=0) y `b` (t=100). Lineal: la curva del
 * tiempo la pone rk_face_ease. */
rk_face_geom_t rk_face_geom_lerp(const rk_face_geom_t *a,
                                 const rk_face_geom_t *b, uint8_t t_pct);

/* Arranca y termina suave (smoothstep), en enteros: 0->0, 50->50, 100->100. */
uint8_t rk_face_ease(uint8_t t_pct);

/* Dibuja la cara completa, fondo incluido, ocupando todo `fb`. La cara se
 * centra en el panel y se escala con su lado corto, así que la misma función
 * sirve para 128x128 y para 240x320.
 *
 * `p` puede ser NULL: sale el primer Rooti. `rareza` elige la piel
 * (rk_rareza_t; fuera de rango, la común). `adornos_extra` son los que
 * agrega el crecimiento (ver rk_face_adornos_etapa); los de la piel se
 * suman solos. */
void rk_face_draw(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                  rk_mood_t mood, rk_severity_t sev,
                  uint8_t adornos_extra, uint32_t t_ms);

/* Igual, con los párpados forzados a cerrarse `cierre` por ciento (0 abiertos,
 * 100 cerrados). Es lo que usa el despertar. */
void rk_face_draw_cierre(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                         rk_mood_t mood, rk_severity_t sev,
                         uint8_t adornos_extra, uint8_t cierre, uint32_t t_ms);

/* Un punto intermedio entre dos ánimos: `t_pct` 0 es exactamente `desde`,
 * 100 es exactamente `hacia`. `t_pct` ya viene con su curva. */
void rk_face_draw_mezcla(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                         rk_mood_t desde, rk_mood_t hacia, uint8_t t_pct,
                         rk_severity_t sev, uint8_t adornos_extra,
                         uint8_t cierre, uint32_t t_ms);

/* La cara cuando la acarician desde la app: la de contento con los ojos en
 * ^ ^, las cejas altas y un ronroneo en vez de la respiración. `mimo_pct`
 * 0 es exactamente la cara del ánimo; 100, el mimo entero. La maceta no lo
 * usa: el aparato no sabe que lo tocan. */
void rk_face_draw_mimo(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                       rk_mood_t mood, uint8_t mimo_pct,
                       rk_severity_t sev, uint8_t adornos_extra,
                       uint32_t t_ms);

/* La mirada dirigida, para el invernadero de la app: varios Rooties que se
 * miran entre ellos y miran con preocupación al vecino que tiene sed o frío.
 * `mira_x`/`mira_y` (-100..100) se suman a la mirada del ánimo y giran un
 * poco la cara; `preocupado` (0..100) sube las cejas por el lado de adentro
 * y afloja la sonrisa. Con todo en cero es exactamente rk_face_draw. */
typedef struct {
    int     mira_x;
    int     mira_y;
    uint8_t preocupado;
} rk_face_mirada_t;

void rk_face_draw_mirada(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                         rk_mood_t mood, rk_severity_t sev,
                         uint8_t adornos_extra, const rk_face_mirada_t *m,
                         uint32_t t_ms);

/* Con una piel que no es de la tabla: la cara dormida antes del cofre usa
 * una gris, para mostrar al personaje sin adelantar la rareza. */
void rk_face_draw_con_piel(rk_fb_t *fb, const rk_persona_t *p,
                           const rk_piel_t *piel, rk_mood_t mood,
                           rk_severity_t sev, uint8_t adornos_extra,
                           uint32_t t_ms);

/* Qué adornos desbloquea cada etapa del vínculo. La piel te toca en el
 * cofre; el aura y la corona también se ganan cuidando la planta. */
uint8_t rk_face_adornos_etapa(int etapa);

/* Color de fondo de la cara para un Rooti, una piel y un ánimo, con el tinte
 * y la penumbra ya aplicados. */
rk_color_t rk_face_fondo(const rk_persona_t *p, uint8_t rareza, rk_mood_t mood);

#endif /* ROOTKIT_FACE_H */
