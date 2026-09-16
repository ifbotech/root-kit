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
 *
 * EL CAMBIO DE ÁNIMO NO SALTA
 *
 * Cuando el ánimo cambia, la cara tarda RK_CARA_TRANSICION_MS en llegar a la
 * expresión nueva (art/face.h, rk_face_draw_mezcla). rk_cara_anim_t es el
 * reloj de esa transición: quien dibuja le dice cada cuadro cuál es el ánimo
 * vigente, y él sabe desde cuál viene y cuánto falta. Es una función pura
 * del tiempo, así que el emulador y la placa muestran lo mismo.
 */
#ifndef ROOTKIT_CARA_H
#define ROOTKIT_CARA_H

#include "../gfx/fb.h"
#include "../gfx/panel.h"
#include "../core/node.h"

/* La pantalla en régimen. `n` es el nodo propio; NULL dibuja una cara sin
 * datos. Sin transición: el cuadro es función sólo del nodo y del tiempo. */
void rk_cara_draw(rk_fb_t *fb, const rk_node_t *n, uint32_t t_ms);

/* El ánimo que muestra la pantalla para este nodo. Casi siempre es el
 * veredicto; con la batería crítica y la planta bien, la cara se duerme (es
 * la única pista de batería en la pantalla, y es a propósito una cara). */
rk_mood_t rk_cara_animo_de(const rk_node_t *n);

/* Vinculado pero con el cofre cerrado: unos ojos dormidos, en gris, que
 * respiran. No usa los colores del personaje porque todavía no se reveló:
 * mostrarlos en la maceta arruinaría la sorpresa del cofre. */
void rk_cara_dormida(rk_fb_t *fb, uint32_t t_ms);

/* La persona neutra con la que se dibuja la cara dormida. */
extern const rk_persona_t rk_persona_incognita;

/* ------------------------------------------------------- la transición --- */
#define RK_CARA_TRANSICION_MS 350u

typedef struct {
    uint8_t  desde;      /* rk_mood_t del que viene                        */
    uint8_t  hacia;      /* rk_mood_t vigente                              */
    uint32_t t0_ms;      /* cuándo empezó la transición                    */
    bool     iniciada;
} rk_cara_anim_t;

/* Arranca ya en `mood`, sin transición. */
void rk_cara_anim_iniciar(rk_cara_anim_t *a, rk_mood_t mood, uint32_t t_ms);

/* Se llama cada cuadro con el ánimo vigente. Si cambió, empieza una
 * transición desde el que se estaba mostrando. Si todavía no arrancó, arranca
 * en ese ánimo sin transición. */
void rk_cara_anim_animo(rk_cara_anim_t *a, rk_mood_t mood, uint32_t t_ms);

/* Cuánto de la transición pasó, ya con la curva suave: 0..100, y 100 cuando
 * terminó (o si nunca hubo transición). */
uint8_t rk_cara_anim_pct(const rk_cara_anim_t *a, uint32_t t_ms);
bool    rk_cara_anim_en_curso(const rk_cara_anim_t *a, uint32_t t_ms);

/* Dibuja el cuadro que corresponde a este instante de la transición. */
void rk_cara_anim_draw(rk_fb_t *fb, const rk_persona_t *p, const rk_cara_anim_t *a,
                       rk_severity_t sev, uint8_t adornos_extra, uint8_t cierre,
                       uint32_t t_ms);

/* Como rk_cara_draw, con transición: le pasa a `a` el ánimo del nodo y dibuja
 * el cuadro. Es lo que usa la placa en régimen. */
void rk_cara_draw_anim(rk_fb_t *fb, const rk_node_t *n, rk_cara_anim_t *a,
                       uint8_t cierre, uint32_t t_ms);

#endif /* ROOTKIT_CARA_H */
