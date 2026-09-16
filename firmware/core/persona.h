/* persona.h — los modelos del ROOTKIT: una carcasa impresa y su cara.
 *
 * EL CAMBIO DE PRODUCTO QUE ESTE ARCHIVO REPRESENTA
 *
 * Antes la variedad era digital: doce simbiontes, cada uno un cuerpo pixel
 * art distinto, y el que te tocaba salía de la especie de tu planta. Era
 * caro de dibujar y —con una silueta compartida entre los doce— tampoco
 * terminaba de funcionar: doce tortugas repintadas no son doce criaturas.
 *
 * Ahora la variedad es FÍSICA. Comprás una caja ciega, te toca una carcasa
 * impresa en 3D, y esa carcasa ES el personaje: su cresta, su pelo, su
 * visera. Lo único que se diseña en pixeles es la CARA, y la cara hace
 * juego con la carcasa que te tocó.
 *
 * Es mejor reparto de esfuerzo. Imprimir una carcasa nueva cuesta filamento
 * y unas horas de modelado; dibujar y animar un cuerpo nuevo cuesta semanas.
 * Y una cara sola en 128x128 tiene muchos más pixeles por rasgo que un
 * cuerpo entero, así que se expresa mejor, no peor.
 *
 * LA MECÁNICA: EL ÁNIMO DICE QUÉ SIENTE, LA PERSONA DICE CÓMO LO MUESTRA
 *
 * `core/mood.c` sigue decidiendo el estado a partir de la planta, igual que
 * siempre. Lo que cambia es que ese estado ahora se dibuja a través del
 * carácter de la carcasa:
 *
 *   sed + CRESTA   -> ceño apretado, dientes, mirada furiosa
 *   sed + KAWAII   -> ojos llorosos, temblor, una lágrima con brillo
 *   sed + VISOR    -> la onda del visor se quiebra en picos de alerta
 *   sed + CICLOPE  -> la pupila enorme se contrae
 *   sed + HONGO    -> los párpados caen todavía más, lengua afuera
 *
 * Cinco modelos por once ánimos son cincuenta y cinco caras distintas, y
 * salen todas del mismo código porque la cara es PROCEDURAL y no sprites.
 * Agregar un modelo es agregar una fila a esta tabla.
 *
 * LA RAREZA AHORA ES FÍSICA
 *
 * Ya no sale de la dificultad de la planta: sale de qué carcasa te tocó en
 * la caja. Eso mantiene el producto fuera del terreno regulado de las cajas
 * de botín por una razón todavía más sólida que antes —no hay compra
 * aleatoria dentro de un software, hay un juguete en una caja, que es
 * exactamente lo que hacen los Smiski y los Sonny Angel desde hace años.
 *
 * Lo que se gana con el cuidado de la planta ya no es el personaje sino
 * cómo se ve: los días sanos desbloquean capas cosméticas sobre la cara.
 * Ver rk_persona_adornos_etapa().
 */
#ifndef ROOTKIT_PERSONA_H
#define ROOTKIT_PERSONA_H

#include <stdint.h>
#include <stdbool.h>
#include "../gfx/fb.h"

/* Rareza dentro de la caja ciega. Cinco modelos a la vista y un secreto,
 * que es la mecánica exacta que hizo coleccionables a los Smiski. */
typedef enum {
    RK_RAR_COMUN = 0,
    RK_RAR_RARO,
    RK_RAR_SECRETO,
    RK_RAR_COUNT
} rk_rarity_t;

/* Familia de ojos. Decide qué función los dibuja; el ánimo decide con qué
 * forma dentro de esa familia. */
typedef enum {
    RK_OJOS_REDONDOS = 0,   /* el caso base: iris grande, muy expresivo   */
    RK_OJOS_RASGADOS,       /* almendrados y arqueados, estilo kawaii     */
    RK_OJOS_FIEROS,         /* angostos e inclinados, siempre enojados    */
    RK_OJOS_VISOR,          /* una sola banda: la onda es la expresión    */
    RK_OJOS_UNICO,          /* un ojo enorme y centrado                   */
    RK_OJOS_PESADOS,        /* párpados caídos, permanentemente dormido   */
    RK_OJOS_COUNT
} rk_familia_ojos_t;

typedef enum {
    RK_CEJA_NINGUNA = 0,
    RK_CEJA_FINA,
    RK_CEJA_GRUESA,
    RK_CEJA_DESPEINADA,     /* trazos sueltos, no una línea               */
    RK_CEJA_COUNT
} rk_ceja_t;

typedef enum {
    RK_BOCA_LINEA = 0,      /* un trazo: mínima, deja hablar a los ojos   */
    RK_BOCA_GATO,           /* la "w" felina, kawaii                      */
    RK_BOCA_DIENTES,        /* boca abierta con dentadura: agresiva       */
    RK_BOCA_ONDA,           /* una onda: robótica, sin labios             */
    RK_BOCA_CHICA,          /* un puntito, para caras dominadas por ojos  */
    RK_BOCA_ESTILO_COUNT
} rk_boca_estilo_t;

/* Adornos. Bitmask porque se combinan y porque las etapas de crecimiento
 * agregan los suyos encima de los que la persona ya trae de fábrica. */
#define RK_ADORNO_BRILLOS   0x01u   /* destellos flotando                 */
#define RK_ADORNO_RUBOR     0x02u   /* dos manchas en los pómulos         */
#define RK_ADORNO_COLMILLO  0x04u   /* un diente que asoma                */
#define RK_ADORNO_SCANLINE  0x08u   /* barrido horizontal sobre la cara   */
#define RK_ADORNO_ESPORAS   0x10u   /* motas que suben lentamente         */
#define RK_ADORNO_ESTATICA  0x20u   /* ruido: sólo el modelo secreto      */
#define RK_ADORNO_AURA      0x40u   /* resplandor: lo da el crecimiento   */
#define RK_ADORNO_CORONA    0x80u   /* tres puntas: la última etapa       */

typedef struct {
    const char *id;            /* clave estable: "cresta"                 */
    const char *nombre;        /* lo que muestra la app: "Cresta"         */
    const char *carcasa;       /* archivo imprimible                      */
    const char *lema;          /* una línea de personalidad               */
    rk_rarity_t rareza;

    uint8_t  familia;          /* rk_familia_ojos_t                       */
    uint8_t  ojo_rx;           /* radios del ojo, en centésimas del ancho */
    uint8_t  ojo_ry;
    uint8_t  ojo_dx;           /* separación, en centésimas del ancho     */
    int8_t   ojo_inclina;      /* inclinación del ojo, -20 a 20           */

    uint8_t  ceja;             /* rk_ceja_t                               */
    int8_t   ceja_angulo;      /* negativo = enojado, positivo = triste   */
    uint8_t  ceja_alto;        /* separación del ojo, en centésimas       */

    uint8_t  boca;             /* rk_boca_estilo_t                        */
    uint8_t  boca_ancho;       /* en centésimas del ancho del panel       */

    uint8_t  adornos;          /* los de fábrica                          */

    /* Paleta de la cara. Son pocos colores a propósito: el personaje lo
     * pone la carcasa, y una cara con demasiados tonos compite con ella. */
    rk_color_t fondo;
    rk_color_t fondo2;         /* el degradé de la escena                 */
    rk_color_t trazo;          /* contorno de ojos y boca                 */
    rk_color_t blanco;         /* esclerótica                             */
    rk_color_t iris;
    rk_color_t acento;         /* brillos, rubor, aura                    */
} rk_persona_t;

extern const rk_persona_t rk_persona_table[];
extern const int          rk_persona_count;

/* Nunca devuelve NULL con un índice válido; NULL si el id no existe. */
const rk_persona_t *rk_persona_find(const char *id);
const rk_persona_t *rk_persona_at(int idx);
int                 rk_persona_index(const rk_persona_t *p);

const char *rk_rarity_name(rk_rarity_t r);
rk_color_t  rk_rarity_color(rk_rarity_t r);
/* Cuántos destellos merece cada rareza en la revelación. */
int         rk_rarity_sparkles(rk_rarity_t r);
/* Cuántos modelos hay de cada rareza. Lo usa la app para mostrar la
 * probabilidad de la caja sin inventarla. */
int         rk_rarity_count(rk_rarity_t r);

#endif /* ROOTKIT_PERSONA_H */
