/* persona.h — los Rooties: cinco personajes botánicos y sus tres pieles.
 *
 * LA FIGURA ES EL PERSONAJE; EL COFRE ES LA PIEL
 *
 * Cada ROOTKIT es un Rooti de verdad: una figura impresa en 3D que es su
 * cuerpo, con la pantalla puesta donde va la cara. Qué Rooti es lo decide la
 * figura que viene en la caja, y la fábrica lo graba en la NVS del aparato
 * ("brote"). Nada en el software lo sortea: la persona ya lo descubrió al
 * abrir la caja, y un cofre que le dijera otra cosa sería confuso.
 *
 * Lo que sí se sortea, una sola vez, es la PIEL: al abrir el cofre en la app
 * sale la rareza (común 70 %, rara 25 %, épica 5 %), y con ella una de las
 * tres paletas de ese personaje. La nube la guarda con la planta y se la
 * manda al aparato en el sync ("rareza": "epico"); la pantalla de la maceta
 * se pinta con esos colores desde ese momento.
 *
 * ESTILO: OOBLETS + POKÉMON CAFÉ REMIX
 *
 * Ojos grandes y oscuros con brillos blancos, una media luna más clara abajo
 * que les da profundidad, mejillas sonrosadas, bocas chicas y dulces. Sin
 * contorno negro: formas planas y redondas con el borde suavizado. Fondos
 * pastel claros, que es donde un ojo oscuro con brillo se lee mejor.
 *
 * LA PANTALLA PONE LA CARA; LA APP DIBUJA EL CUERPO
 *
 * El aparato sólo dibuja la cara (art/face.c). El cuerpo entero —hojas,
 * patitas, sombrero— lo pone la figura impresa en la mesa y, en el teléfono,
 * root-lab/public/lib/cuerpo.mjs, con los mismos colores de esta tabla.
 *
 * ARTE COMO DATOS
 *
 * Cada fila es un personaje y cada número se puede ajustar sin tocar
 * lógica: proporciones de los ojos, el tipo de brillo, la boca, las mejillas
 * y las tres pieles. Los colores van en hexadecimal (RK_HEX), igual que los
 * entrega la artista. root-lab lee esta misma tabla para las paletas de la
 * app (tools/sincronizar-firmware.mjs): hay una sola fuente de verdad.
 */
#ifndef ROOTKIT_PERSONA_H
#define ROOTKIT_PERSONA_H

#include <stdint.h>
#include <stdbool.h>
#include "../gfx/fb.h"

/* Un color como lo escribe una diseñadora: RK_HEX(0xE8F5E9). */
#define RK_HEX(h) RK_RGB(((h) >> 16) & 0xFF, ((h) >> 8) & 0xFF, (h) & 0xFF)

/* La rareza de la piel, que sale del cofre. */
typedef enum {
    RK_RAREZA_COMUN = 0,
    RK_RAREZA_RARA,
    RK_RAREZA_EPICA,
    RK_RAREZA_COUNT
} rk_rareza_t;

/* Familia de ojos: la forma de base, sobre la que actúa el ánimo. */
typedef enum {
    RK_OJOS_REDONDOS = 0,   /* grandes y abiertos: el caso base            */
    RK_OJOS_MEDIALUNA,      /* párpados relajados a media altura: "u u"    */
    RK_OJOS_ARCO,           /* contento, en arco "^ ^", y guiña            */
    RK_OJOS_COUNT
} rk_familia_ojos_t;

/* Cómo brilla el ojo. Es el detalle que lo vuelve húmedo y vivo. */
typedef enum {
    RK_BRILLO_SIMPLE = 0,   /* un brillo grande arriba y un punto abajo     */
    RK_BRILLO_CACHORRO,     /* espejados entre los dos ojos, con destello   */
    RK_BRILLO_DOBLE,        /* dos puntos de luz grandes, estilo Café       */
    RK_BRILLO_COUNT
} rk_brillo_t;

typedef enum {
    RK_CEJA_NINGUNA = 0,
    RK_CEJA_FINA,           /* un trazo corto y suave                       */
    RK_CEJA_FLOTANTE,       /* dos óvalos redondeados, separados del ojo    */
    RK_CEJA_COUNT
} rk_ceja_t;

/* La boca de cada personaje cuando sonríe. Con otros ánimos manda la boca
 * del ánimo (abierta, temblorosa, mueca). */
typedef enum {
    RK_BOCA_SUAVE = 0,      /* una sonrisa chica y blanda                   */
    RK_BOCA_GATO,           /* ":3", la boca de gato                        */
    RK_BOCA_DIENTECITO,     /* sonrisa abierta con un dientito asomando     */
    RK_BOCA_D,              /* ":D", abierta y alegre, con lengua           */
    RK_BOCA_ESTILO_COUNT
} rk_boca_estilo_t;

typedef enum {
    RK_MEJILLA_CIRCULO = 0, /* dos chapitas redondas                        */
    RK_MEJILLA_HORIZONTAL,  /* rubor ancho y bajito                         */
    RK_MEJILLA_BRILLO,      /* mejilla con un punto de brillo               */
    RK_MEJILLA_PECAS,       /* rubor tenue con tres pecas                   */
    RK_MEJILLA_SUAVE,       /* un óvalo chico y difuso                      */
    RK_MEJILLA_COUNT
} rk_mejilla_t;

/* Adornos. Bitmask porque se combinan: los trae la piel (la rara brilla, la
 * épica tiene corona o aura) y los suma el crecimiento del vínculo. */
#define RK_ADORNO_BRILLOS   0x01u   /* destellos que orbitan la cara       */
#define RK_ADORNO_AURA      0x02u   /* un anillo que respira en el borde   */
#define RK_ADORNO_CORONA    0x04u   /* corona dorada arriba de los ojos    */
#define RK_ADORNO_LUCES     0x08u   /* luces que suben: bioluminiscencia   */

/* Una piel: los cuatro colores de la paleta y lo que trae de regalo. */
typedef struct {
    const char *nombre;        /* "Flor de Cerezo Dorada"                 */
    rk_color_t  fondo;         /* toda la pantalla                        */
    rk_color_t  ojos;          /* ojos, boca y cejas                      */
    rk_color_t  piel;          /* el cuerpo (o la flor, o el sombrero)    */
    rk_color_t  rubor;         /* las mejillas                            */
    uint8_t     adornos;       /* RK_ADORNO_*                             */
} rk_piel_t;

typedef struct {
    const char *id;            /* clave estable: "brote"                  */
    const char *nombre;        /* lo que muestra la app: "Brote"          */
    const char *carcasa;       /* archivo imprimible                      */
    const char *lema;          /* una línea de personalidad               */

    /* Medidas en CENTÉSIMAS DEL LADO CORTO DEL PANEL: la misma tabla sirve
     * para la pantalla de 1,44" (128x128) y para la de 2,2" (240x320). Las
     * alturas (`_dy`) son desde el centro; negativo es más arriba. */
    uint8_t  familia;          /* rk_familia_ojos_t                       */
    uint8_t  brillo;           /* rk_brillo_t                             */
    uint8_t  ojo_rx;
    uint8_t  ojo_ry;
    uint8_t  ojo_dx;           /* separación desde el centro              */
    int8_t   ojo_dy;

    uint8_t  ceja;             /* rk_ceja_t                               */
    int8_t   ceja_angulo;      /* positivo = preocupación, negativo = ceño */
    uint8_t  ceja_alto;        /* separación del ojo                      */

    uint8_t  boca;             /* rk_boca_estilo_t                        */
    uint8_t  boca_ancho;
    int8_t   boca_dy;

    uint8_t  mejilla;          /* rk_mejilla_t                            */

    /* Común, rara y épica, en ese orden. El fondo es liso a propósito: los
     * párpados se pintan del color del fondo, y un párpado que "tapa" el
     * ojo sólo funciona si el color que tapa es exactamente el de
     * alrededor. */
    rk_piel_t pieles[RK_RAREZA_COUNT];
} rk_persona_t;

extern const rk_persona_t rk_persona_table[];
extern const int          rk_persona_count;

/* NULL si el id no existe. */
const rk_persona_t *rk_persona_find(const char *id);
const rk_persona_t *rk_persona_at(int idx);
int                 rk_persona_index(const rk_persona_t *p);

/* La piel de una rareza. Una rareza fuera de rango cae en la común; con
 * `p` NULL devuelve NULL. */
const rk_piel_t *rk_persona_piel(const rk_persona_t *p, int rareza);

/* "comun", "raro", "epico": la clave del protocolo y de la base. */
const char *rk_rareza_id(rk_rareza_t r);
/* "COMUN", "RARA", "EPICA": para rótulos con la fuente de 5x7. */
const char *rk_rareza_nombre(rk_rareza_t r);
/* La rareza de una clave, o -1 si no es ninguna. */
int         rk_rareza_parse(const char *id);
/* El color del borde de la ficha en las láminas. */
rk_color_t  rk_rareza_color(rk_rareza_t r);
/* Cuántos destellos merece cada rareza en la revelación. */
int         rk_rareza_destellos(rk_rareza_t r);

#endif /* ROOTKIT_PERSONA_H */
