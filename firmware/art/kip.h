/* kip.h — Kip, dibujado como lo dibuja Rocío.
 *
 * POR QUÉ KIP TIENE SU PROPIO DIBUJANTE
 *
 * El rig de art/face.c arma once ánimos para cualquier Rooti cruzando una
 * tabla de expresiones con una fila de proporciones. Funciona, pero lo que
 * sale es una cara de catálogo: ojos de compás, cejas de palito y bocas que
 * son curvas de programa. Rocío entregó las láminas de Kip —cuadro por
 * cuadro, con la cara que pone cuando la planta se ahoga, cuando tiene
 * calor, cuando está a oscuras, cuando el aire está seco y cuando está
 * contento— y esas caras no salen de ningún rig genérico: salen de
 * dibujarlas.
 *
 * Así que Kip deja de pasar por el rig y tiene su hoja de modelo:
 *
 *   - LOS RASGOS SON LOS DE LA LÁMINA. Cuerpo rojo plano, cejas negras
 *     tupidas que saltan, ojos blancos con un contorno de tinta más grueso
 *     arriba que abajo, iris ámbar con su aro, su pupila y un brillo, y la
 *     boca teal. Todo sale de trazos de pincel: contornos que engordan en
 *     el medio y se afinan en la punta (gfx/aa.h, rk_aa_poligono).
 *
 *   - LAS POSES SON LAS DE LA LÁMINA. Cada ojo, cada ceja y cada boca que
 *     dibujó la artista es una fila de una tabla (kip.c: OJOS, CEJAS,
 *     BOCAS). Nada de eso es una fórmula: son sus dibujos, medidos.
 *
 *   - LAS ANIMACIONES SON HOJAS DE EXPOSICIÓN. Cada ánimo es una pista de
 *     claves —en tal milisegundo, estos ojos, estas cejas, esta boca, esta
 *     mirada— que sigue los cuadros de la lámina en el orden en que ella los
 *     numeró. Entre clave y clave todo se interpola: la cara no salta de un
 *     dibujo al siguiente, pasa por el medio. Eso es lo que la vuelve fluida
 *     en una pantalla que dibuja treinta cuadros por segundo con diez
 *     dibujos.
 *
 * CÓMO SE MUEVE CADA COSA
 *
 *   - Los ojos, la boca y la mirada con una curva suave; la mirada, además,
 *     rápida: llega en el primer tercio del tramo y se queda, que es como
 *     mira un ojo de verdad (un salto y una fijación).
 *   - LAS CEJAS SALTAN: van con una curva que se pasa un poco y vuelve. Son
 *     lo más expresivo que tiene Kip, y una ceja que llega justo a destino
 *     se ve de muñeco.
 *   - La boca sólo cambia de forma cerrada. Una boca teal no se vuelve una
 *     boca con dientes a mitad de camino: se cierra, cambia y se abre. Es lo
 *     que hace un animador, y por eso la transición no tiene un cuadro feo.
 *   - Encima de la pista van los parpadeos y los "cejazos" de cada ánimo,
 *     en instantes fijos: todo sigue siendo función pura del tiempo.
 *
 * Todo lo que no es de la lámina (sed, frío, sol, dormido, sin datos,
 * desconectado) se dibujó con el mismo vocabulario: las mismas cejas, los
 * mismos ojos, las mismas bocas y los mismos garabatos de tinta.
 */
#ifndef ROOTKIT_KIP_H
#define ROOTKIT_KIP_H

#include "../gfx/fb.h"
#include "../core/mood.h"
#include "../core/persona.h"
#include "look.h"

/* ------------------------------------------------------------ colores --- */
/* Los colores de un cuadro de Kip. Los dos primeros salen de la piel (el
 * cuerpo y la tinta); el resto son de Kip y no cambian con la rareza: son
 * parte de quién es. art/face.c les aplica el tinte del ánimo a todos por
 * igual, y los funde en las transiciones. */
enum {
    RK_KIP_CUERPO = 0,   /* el fondo: el cuerpo rojo                      */
    RK_KIP_TINTA,        /* cejas, contornos, garabatos                   */
    RK_KIP_BLANCO,       /* el blanco del ojo, los dientes                */
    RK_KIP_IRIS,         /* ámbar                                         */
    RK_KIP_ARO,          /* el aro oscuro del iris                        */
    RK_KIP_APAGADO,      /* el iris cansado, gris pardo                   */
    RK_KIP_BOCA,         /* el teal de la boca                            */
    RK_KIP_HONDA,        /* el teal oscuro: el agua honda y la boca en ella */
    RK_KIP_LILA,         /* adentro de la boca, bajo el agua              */
    RK_KIP_LENGUA,
    RK_KIP_AGUA,         /* el gris del agua que le llega a los ojos      */
    RK_KIP_ESPUMA,       /* las ondas claras del agua honda               */
    RK_KIP_OJERA,        /* el granate de las ojeras y los pliegues       */
    RK_KIP_RUBOR,        /* los ojos irritados del calor                  */
    RK_KIP_GOTA,         /* la gota de sudor                              */
    RK_KIP_LUZ,          /* el brillo de la piel sudada                   */
    RK_KIP_REFLEJO,      /* las franjas del reflejo del calor y del sol   */
    RK_KIP_NIEVE,
    RK_KIP_COLORES
};

typedef struct {
    rk_color_t c[RK_KIP_COLORES];
} rk_kip_colores_t;

/* Los colores de Kip para una piel, SIN tratar. `propia` dice si la piel es
 * una de las suyas: con una ajena (la gris de la cara dormida) los colores
 * de Kip pasan a gris, para no adelantar nada que el cofre no sorteó. */
void rk_kip_paleta(rk_kip_colores_t *k, const rk_piel_t *piel, bool propia);

/* Tinte, penumbra, respiración y temblor de Kip en cada ánimo: la versión de
 * Kip de la tabla de art/look.c. Los tintes son suaves a propósito: en las
 * láminas el rojo de Kip casi no cambia de un ánimo a otro, y lo que cuenta
 * el ánimo es la cara. Nunca devuelve NULL. */
const rk_look_t *rk_kip_look(rk_mood_t mood);

/* --------------------------------------------------------------- pose --- */
/* La pose es lo que se interpola. Todo en enteros: las medidas en milésimas
 * (del lado corto del panel, o del ojo de la fila de persona.c, según dice
 * cada campo), las intensidades de 0 a 1000. */

/* Un ojo: dos curvas que se juntan en las comisuras. La de arriba sube `a` y
 * la de abajo baja `b` (en milésimas del alto del ojo de la fila). Si la de
 * arriba baja más de lo que sube la de abajo, el ojo está cerrado: queda
 * una línea, curvada hacia abajo (dormido) o hacia arriba (contento). */
typedef struct {
    int16_t a, b;        /* milésimas de ojo_ry                            */
    int16_t incl;        /* la comisura de afuera baja (+) o sube (-)      */
    int16_t redondo;     /* 0 almendra con puntas .. 1000 redondo          */
    int16_t pico;        /* corre el punto más alto hacia afuera (+)       */
    int16_t ancho;       /* milésimas de ojo_rx                            */
    int16_t dy;          /* milésimas de ojo_ry                            */
    int16_t iris;        /* radio, milésimas de ojo_rx                     */
    int16_t pliegue;     /* 0..1000: la línea del párpado de arriba        */
    int16_t ojera;       /* 0..1000: la media luna oscura de abajo         */
    int16_t risa;        /* 0..1000: las rayitas de la mejilla que sube    */
    int16_t rojo;        /* 0..1000: irritado                              */
    int16_t pestana;     /* 0..1000: las líneas que siguen de largo        */
    int16_t apagado;     /* 0..1000: el iris pierde el ámbar               */
} rk_kip_ojo_t;

/* Una ceja: un trazo grueso de adentro hacia afuera. */
typedef struct {
    int16_t dy, dx;      /* milésimas del lado; dx positivo, hacia afuera  */
    int16_t ang;         /* la punta de adentro sube (+) o baja (-)        */
    int16_t arco;        /* cuánto se arquea hacia arriba en el medio      */
    int16_t grosor;      /* milésimas del lado, en lo más grueso           */
    int16_t tupido;      /* 0..1000: lo despeinado del borde de arriba     */
    int16_t cola;        /* 0..1000: cuánto se afina la punta de afuera    */
    int16_t largo;       /* media ceja, milésimas de ojo_rx                */
} rk_kip_ceja_t;

typedef enum {
    RK_KIP_BOCA_LINEA = 0,  /* un trazo: sonrisa, recta o mueca           */
    RK_KIP_BOCA_TEAL,       /* media luna teal con el labio de tinta      */
    RK_KIP_BOCA_DIENTES,    /* media luna de dientes                      */
    RK_KIP_BOCA_GRITO,      /* abierta y oscura: dientes, lengua, jadeo   */
    RK_KIP_BOCA_SUMERGIDA,  /* labios teal gruesos, lila adentro          */
    RK_KIP_BOCA_O,          /* soplando                                   */
    RK_KIP_BOCA_BESO,       /* el "3"                                     */
    RK_KIP_BOCA_ONDA,       /* un trazo ondulado                          */
    RK_KIP_BOCA_COUNT
} rk_kip_boca_estilo_t;

typedef struct {
    int16_t estilo;      /* rk_kip_boca_estilo_t                          */
    int16_t ancho;       /* media boca, milésimas del lado                */
    int16_t curva;       /* cuánto suben las comisuras (- bajan)          */
    int16_t abre;        /* milésimas del lado                            */
    int16_t ladeo;       /* la comisura derecha sube (+)                  */
    int16_t dx, dy;      /* milésimas del lado                            */
    int16_t lengua;      /* 0..1000: la lengua afuera                     */
    int16_t labio;       /* 0..1000: la rayita del labio de abajo         */
    int16_t comisura;    /* 0..1000: los ganchitos de las puntas          */
} rk_kip_boca_t;

/* Los efectos de cada ánimo, de 0 a 1000. En una transición se funden: el
 * agua sube o se escurre, las grietas crecen o se borran. */
enum {
    RK_KIP_FX_AGUA = 0,  /* el agua hasta los ojos                        */
    RK_KIP_FX_GRIETAS,   /* la cara cuarteada                             */
    RK_KIP_FX_SUDOR,     /* la gota que baja por la cara                  */
    RK_KIP_FX_CALOR,     /* el garabato del calor, el brillo y el reflejo */
    RK_KIP_FX_ZZZ,
    RK_KIP_FX_NIEVE,
    RK_KIP_FX_DUDA,      /* el signo de pregunta                          */
    RK_KIP_FX_ESPERA,    /* los tres puntitos                             */
    RK_KIP_FX_SOL,       /* el encandilamiento                            */
    RK_KIP_FX_SED,       /* la gota vacía: pide agua                      */
    RK_KIP_FX_CENO,      /* las rayitas entre las cejas                   */
    RK_KIP_FX_COUNT
};

typedef struct {
    rk_kip_ojo_t  ojo[2];     /* 0 el de la izquierda de la pantalla       */
    rk_kip_ceja_t ceja[2];
    rk_kip_boca_t boca;
    int16_t mira_x, mira_y;   /* -1000..1000                               */
    int16_t dy;               /* la cara entera, milésimas del lado        */
    int16_t fx[RK_KIP_FX_COUNT];
} rk_kip_pose_t;

/* ------------------------------------------------------------ escena --- */
/* Todo lo que decide un cuadro. Con `desde` == `hacia`, `mimo` 0, mirada y
 * preocupación en 0 y `cierre` 0, es la cara pura de un ánimo. */
typedef struct {
    rk_mood_t desde, hacia;
    uint8_t   mezcla;        /* 0 es `desde`, 100 es `hacia`; ya con curva */
    uint8_t   cierre;        /* 0..100: párpados forzados (el despertar)   */
    uint8_t   mimo;          /* 0..100: lo acarician desde la app          */
    int       mira_x, mira_y;/* -100..100: mira a un vecino                */
    uint8_t   preocupado;    /* 0..100                                     */
    uint32_t  t_ms;
} rk_kip_escena_t;

/* La pose de un instante. Es función pura de la escena. */
void rk_kip_pose(rk_kip_pose_t *out, const rk_kip_escena_t *e);

/* Cuánto dura el ciclo de un ánimo, en ms. */
uint32_t rk_kip_periodo(rk_mood_t mood);

/* Dibuja los rasgos de Kip —no el fondo— con el centro de la cara en
 * (cx, cy), en Q4, sobre un panel de lado corto `u`. Las medidas de base
 * (dónde van los ojos, qué tan grandes, dónde va la boca) son las de la fila
 * de Kip en core/persona.c. */
void rk_kip_dibujar(rk_fb_t *fb, const rk_persona_t *p, const rk_kip_colores_t *col,
                    const rk_kip_pose_t *pose, int32_t cx, int32_t cy, int32_t u,
                    uint32_t t_ms);

#endif /* ROOTKIT_KIP_H */
