#include "kip.h"
#include "../gfx/aa.h"
#include <stddef.h>
#include <string.h>

/* ============================================================ colores === */
/* Medidos sobre las láminas de Rocío. El rojo del cuerpo y la tinta vienen
 * de la piel (core/persona.c); estos son de Kip y no cambian con la rareza. */
#define KIP_BLANCO   RK_HEX(0xF5F3EA)
#define KIP_IRIS     RK_HEX(0xE2A232)
#define KIP_ARO      RK_HEX(0x5A3A12)
#define KIP_APAGADO  RK_HEX(0x8C7F78)
#define KIP_BOCA     RK_HEX(0x3C8A7D)
#define KIP_HONDA    RK_HEX(0x3D6B6B)
#define KIP_LILA     RK_HEX(0xA89CA6)
#define KIP_LENGUA   RK_HEX(0xD8907E)
#define KIP_AGUA     RK_HEX(0x93928B)
#define KIP_ESPUMA   RK_HEX(0x9CC4C2)
#define KIP_GOTA     RK_HEX(0xC4CFD2)
#define KIP_LUZ      RK_HEX(0xF4CFC6)
#define KIP_REFLEJO  RK_HEX(0xDC4828)
#define KIP_NIEVE    RK_HEX(0xFFFFFF)

static rk_color_t gris(rk_color_t c)
{
    int r = ((c >> 11) & 0x1F) * 255 / 31;
    int g = ((c >> 5) & 0x3F) * 255 / 63;
    int b = (c & 0x1F) * 255 / 31;
    int l = (2126 * r + 7152 * g + 722 * b) / 10000;
    return RK_RGB(l, l, l);
}

void rk_kip_paleta(rk_kip_colores_t *k, const rk_piel_t *piel, bool propia)
{
    static const rk_color_t PROPIOS[RK_KIP_COLORES] = {
        0, 0, KIP_BLANCO, KIP_IRIS, KIP_ARO, KIP_APAGADO, KIP_BOCA, KIP_HONDA,
        KIP_LILA, KIP_LENGUA, KIP_AGUA, KIP_ESPUMA, 0, 0, KIP_GOTA, KIP_LUZ,
        KIP_REFLEJO, KIP_NIEVE
    };
    int i;

    if (k == NULL || piel == NULL) {
        return;
    }
    for (i = 0; i < RK_KIP_COLORES; i++) {
        k->c[i] = propia ? PROPIOS[i] : gris(PROPIOS[i]);
    }
    k->c[RK_KIP_CUERPO] = piel->fondo;
    k->c[RK_KIP_TINTA] = piel->ojos;
    /* Las ojeras y los pliegues son el cuerpo hundido hacia la tinta: con
     * cualquier piel quedan del mismo granate que en la lámina. */
    k->c[RK_KIP_OJERA] = rk_mix(piel->fondo, piel->ojos, 140);
    k->c[RK_KIP_RUBOR] = piel->rubor;
}

/* Los tintes van casi al mínimo: en las láminas el rojo de Kip es el mismo
 * con calor, con agua y a oscuras. Lo que sí queda es lo que es producto y no
 * dibujo: de noche la pantalla baja, y sin datos la cara se destiñe. */
#define TINT_GRIS   RK_RGB(110, 120, 130)
#define TINT_SED    RK_RGB(190, 150,  90)
#define TINT_FRIO   RK_RGB( 90, 150, 235)
#define TINT_BRASA  RK_RGB(200,  40,  30)
#define TINT_SOL    RK_RGB(255, 210, 120)

static const rk_look_t LOOKS_KIP[RK_MOOD_COUNT] = {
/*                ojo boca tinte       amt amp vel shv dim blink */
/* UNKNOWN     */ { 0, 0,  0,            0,  3, 30, 0,   0, true  },
/* OFFLINE     */ { 0, 0,  TINT_GRIS,  150,  3, 16, 0,  40, true  },
/* SLEEPING    */ { 0, 0,  0,            0,  3, 14, 0,  95, false },
/* HAPPY       */ { 0, 0,  0,            0,  4, 42, 0,   0, true  },
/* THIRSTY     */ { 0, 0,  TINT_SED,    45,  3, 22, 0,   0, true  },
/* DROWNING    */ { 0, 0,  0,            0,  3, 40, 0,   0, true  },
/* COLD        */ { 0, 0,  TINT_FRIO,   75,  3, 26, 1,   0, true  },
/* HOT         */ { 0, 0,  TINT_BRASA,  60,  3, 55, 0,   0, true  },
/* SCORCHED    */ { 0, 0,  TINT_SOL,    75,  3, 18, 0,   0, true  },
/* DARK        */ { 0, 0,  0,            0,  3, 20, 0,  30, true  },
/* PARCHED_AIR */ { 0, 0,  TINT_SED,    30,  3, 34, 0,   0, true  },
};

const rk_look_t *rk_kip_look(rk_mood_t mood)
{
    if ((int)mood < 0 || mood >= RK_MOOD_COUNT) {
        mood = RK_MOOD_UNKNOWN;
    }
    return &LOOKS_KIP[mood];
}

/* ====================================================== hoja de modelo === */
/* Cada fila es un dibujo de la lámina, medido. Los nombres dicen de dónde
 * sale: "feliz 10" es el cuadro 10 de la lámina de contento, y así. */

enum {
    O_CONTENTO = 0, /* feliz 10: redondo, la mejilla lo empuja un poco      */
    O_PLANO,        /* feliz 8: sereno, casi un rectángulo                  */
    O_LENTE,        /* feliz 1: la cara de nada, con las líneas de largo    */
    O_CANCHERO,     /* feliz 2 y 3: el párpado baja, de costado             */
    O_GUINO,        /* feliz 4 y 5: cerrado en ^, con sus pliegues          */
    O_ASOMA,        /* feliz 6: se vuelve a abrir, una rendija              */
    O_ABIERTO,      /* feliz 4 y 5: el otro ojo, una cúpula                 */
    O_GRANDE,       /* feliz 9: bien abiertos, pícaros                      */
    O_AHOGO,        /* agua: anchos, planos arriba, mirando el agua         */
    O_OSCURO,       /* oscuro: el párpado recto a media altura              */
    O_PESADO,       /* calor 1 y 2: casi cerrados, caídos afuera            */
    O_PENA_CERRADO, /* calor 3: cerrados hacia abajo                        */
    O_ROJO,         /* calor 4: irritados, con bolsas                       */
    O_CANSADO,      /* calor 5 a 9: medio cerrados, con ojeras              */
    O_RENDIJA,      /* aire seco: dos rendijas de fastidio                  */
    O_DORMIDO,
    O_SED,          /* sed: cansados, mirando la tierra                     */
    O_PIDE,         /* sed: te mira y pide                                  */
    O_FRIO,
    O_APRIETA,      /* sol: apretados contra la luz                         */
    O_ESPIA,        /* sol: una rendija para espiar                         */
    O_DUDA_CHICO,   /* sin datos: el ojo desconfiado                        */
    O_DUDA_GRANDE,  /* sin datos: el ojo que pregunta                       */
    O_MIMO,         /* lo acarician: ^ ^                                    */
    O_COUNT
};

static const rk_kip_ojo_t OJOS[O_COUNT] = {
/*                     a     b  incl  red  pico ancho   dy iris plie ojer risa rojo pest apag */
/* CONTENTO     */ {  820,  620,  -60, 750,    0,  960,    0, 330,   0,   0, 450,   0,   0,   0 },
/* PLANO        */ {  380,  380,    0,1000,    0, 1000,   60, 320,   0,   0,   0,   0,   0,   0 },
/* LENTE        */ {  330,  330,    0,1000,    0, 1060,   60, 310,   0,   0,   0,   0, 900,   0 },
/* CANCHERO     */ {  120,  560,   60, 550,  150, 1000,  120, 320, 750,   0, 350,   0,   0,   0 },
/* GUINO        */ {  620, -620,   80, 500,    0,  940,   80, 320, 900,   0,1000,   0,   0,   0 },
/* ASOMA        */ {   80,  230,   60, 500,    0,  940,  100, 330, 900,   0, 900,   0,   0,   0 },
/* ABIERTO      */ { 1500,  300,  -80, 900,  -50,  940,  -60, 340,   0,   0, 500,   0,   0,   0 },
/* GRANDE       */ { 1050,  780, -120, 700,    0,  980,  -20, 330,   0,   0, 300,   0,   0,   0 },
/* AHOGO        */ { 1250,  520,  -60, 820,    0, 1180,  -60, 290,   0,   0,   0,   0, 600,   0 },
/* OSCURO       */ {   60, 1450,   60, 450,    0, 1180,  -40, 350,   0,   0,   0,   0,   0,   0 },
/* PESADO       */ { -150,  380,  260, 350,    0, 1050,   60, 300,   0, 250,   0,   0,   0, 700 },
/* PENA_CERRADO */ { -520,  420,  260, 500,    0, 1050,   60, 300,   0, 250,   0,   0, 300, 700 },
/* ROJO         */ {  320,  640,  220, 700,    0, 1050,   40, 280,1000, 700,   0,1000,   0, 700 },
/* CANSADO      */ {  -30,  620,  200, 500,    0, 1050,   40, 300, 350,1000,   0,   0,   0, 650 },
/* RENDIJA      */ {  140,  260,  -40, 250, -100, 1020,  100, 340,   0,   0,   0,   0,   0, 450 },
/* DORMIDO      */ { -450,  380,  120, 500,    0, 1000,   80, 300,   0,   0,   0,   0, 350,   0 },
/* SED          */ {   80,  720,  230, 550,    0, 1020,   60, 310, 450, 350,   0,   0,   0, 250 },
/* PIDE         */ {  650,  800,  250, 700,    0, 1000,   20, 340, 600, 300,   0,   0,   0,   0 },
/* FRIO         */ {  230,  420,  160, 500,    0,  980,   60, 310, 650,   0, 300,   0,   0,   0 },
/* APRIETA      */ {  200, -200,  -60, 300,    0,  940,   80, 300,1000,   0,1000,   0,   0,   0 },
/* ESPIA        */ {   60,  160,  -40, 250,    0,  960,   80, 330, 900,   0, 900,   0,   0, 200 },
/* DUDA_CHICO   */ {  250,  450,  -60, 450,    0,  960,   60, 330, 500,   0, 400,   0,   0,   0 },
/* DUDA_GRANDE  */ { 1100,  750,  -80, 750,    0,  980,  -40, 320,   0,   0,   0,   0,   0,   0 },
/* MIMO         */ {  720, -720,   40, 600,    0,  960,   60, 320, 600,   0,1000,   0,   0,   0 },
};

enum {
    C_ARCO = 0,     /* feliz 10: arqueadas, la cola afinada                 */
    C_RECTA,        /* feliz 1: dos lomos despeinados                       */
    C_ALZADA,       /* feliz 4: la que salta sobre el ojo abierto           */
    C_BAJA,         /* feliz 4: la que baja sobre el guiño                  */
    C_PICARA,       /* feliz 9: adentro abajo, afuera arriba                */
    C_FASTIDIO,     /* agua: gruesas, bajas adentro, despeinadas            */
    C_MECHON,       /* oscuro: el mechón de nube con la cola al costado     */
    C_AGOBIO,       /* calor 1 y 2: bajas y pesadas                         */
    C_PENA,         /* calor 3 a 9: suben adentro                           */
    C_LOSA,         /* aire seco: una losa baja sobre el ojo                */
    C_SUAVE,        /* dormido                                              */
    C_SED,
    C_RUEGO,
    C_FRIO,
    C_ENCANDILADO,
    C_DUDA_ALTA,
    C_DUDA_BAJA,
    C_MIMO,
    C_COUNT
};

static const rk_kip_ceja_t CEJAS[C_COUNT] = {
/*                    dy   dx   ang  arco grosor tupido cola largo */
/* ARCO        */ {    0,  20,  -60,  60,   96,   380,  760, 1080 },
/* RECTA       */ {   10,  10,    0,  50,   88,   480,  450, 1000 },
/* ALZADA      */ {  -72,  20,   40,  85,   96,   380,  780, 1080 },
/* BAJA        */ {   38,  10, -130,  35,   96,   380,  760, 1060 },
/* PICARA      */ {  -30,  20, -280,  75,   98,   400,  830, 1080 },
/* FASTIDIO    */ {   20,  40, -170,  15,  100,   700,  250, 1180 },
/* MECHON      */ {   28,   0,   60,  25,  112,  1000,  950, 1000 },
/* AGOBIO      */ {   55,  20,   50,  10,   86,   300,  800, 1080 },
/* PENA        */ {    0,  20,  470,  60,   82,   260,  880, 1080 },
/* LOSA        */ {   78,  20,  -70,   0,   80,   620,  700, 1080 },
/* SUAVE       */ {   45,  10,   90,  20,   76,   700,  850,  950 },
/* SED         */ {   10,  20,  320,  30,   76,   360,  860, 1050 },
/* RUEGO       */ {  -35,  20,  460,  45,   76,   360,  860, 1050 },
/* FRIO        */ {  -10,  20,  340,  30,   78,   480,  800, 1020 },
/* ENCANDILADO */ {   60,  10, -280, -10,   96,   550,  500, 1060 },
/* DUDA_ALTA   */ {  -88,  20,  150,  95,   94,   380,  780, 1060 },
/* DUDA_BAJA   */ {   42,  10, -210,  20,   96,   380,  700, 1040 },
/* MIMO        */ {  -62,  20,  120,  90,   92,   380,  780, 1060 },
};

enum {
    B_TEAL_GRANDE = 0, /* feliz 10                                          */
    B_TEAL_GUINO,      /* feliz 5                                           */
    B_SONRISA,         /* feliz 8                                           */
    B_SOBRADOR,        /* feliz 3 y 4: sube de un lado                      */
    B_DIENTES,         /* feliz 9                                           */
    B_LENTE,           /* feliz 1                                           */
    B_AGUA,            /* agua: la boqueada                                 */
    B_AGUA_GRANDE,
    B_AGUA_CERRADA,
    B_AGUA_TORCIDA,
    B_RAYA,            /* oscuro                                            */
    B_LENGUA,          /* calor 1                                           */
    B_JADEO_CHICO,     /* calor 2                                           */
    B_JADEO,           /* calor 3 y 4                                       */
    B_RECTA_GRUESA,    /* calor 5                                           */
    B_RECTA_HOYUELOS,  /* calor 6                                           */
    B_O,               /* calor 7                                           */
    B_BESO,            /* calor 8                                           */
    B_ONDA,            /* calor 9                                           */
    B_MUECA,           /* aire seco                                         */
    B_RONCA,
    B_RONCA_ABIERTA,
    B_SED_LENGUA,
    B_SUSPIRO,
    B_CASTANETEO,
    B_MUECA_DIENTES,
    B_HMM,
    B_HMM_OTRO,
    B_MIMO,
    B_COUNT
};

#define LIN RK_KIP_BOCA_LINEA
#define TEA RK_KIP_BOCA_TEAL
#define DIE RK_KIP_BOCA_DIENTES
#define GRI RK_KIP_BOCA_GRITO
#define SUM RK_KIP_BOCA_SUMERGIDA
#define BOO RK_KIP_BOCA_O
#define BES RK_KIP_BOCA_BESO
#define OND RK_KIP_BOCA_ONDA

static const rk_kip_boca_t BOCAS[B_COUNT] = {
/*                     est ancho curva abre ladeo  dx   dy lengua labio comis */
/* TEAL_GRANDE    */ { TEA, 255,   95, 115,   12,  20,   0,    0,  700,  900 },
/* TEAL_GUINO     */ { TEA, 250,   95,  90,   20,  25,   0,    0,  700,  900 },
/* SONRISA        */ { LIN, 165,   42,   0,    0,   0,   0,    0,  800,  600 },
/* SOBRADOR       */ { LIN, 200,   55,   0,   35,  25,   0,    0,  800,  750 },
/* DIENTES        */ { DIE, 205,   70,  60,  -25, -10,  10,    0,  800,  800 },
/* LENTE          */ { TEA, 245,    0,  38,    0,   0, -20,    0,  700,    0 },
/* AGUA           */ { SUM, 215,  -10, 150,   22,   0,   0,    0,  800,    0 },
/* AGUA_GRANDE    */ { SUM, 230,  -15, 200,   28,   0,   0,    0,  800,    0 },
/* AGUA_CERRADA   */ { SUM, 200,    5,   0,   12,   0,   0,    0,  800,    0 },
/* AGUA_TORCIDA   */ { SUM, 215,   -5, 140,  -18,   0,   0,    0,  800,    0 },
/* RAYA           */ { LIN,  48,    0,   0,    0,   0,   0,    0,    0,    0 },
/* LENGUA         */ { GRI, 100,    0,  22,   10, -10, -40,  800,    0,    0 },
/* JADEO_CHICO    */ { GRI,  78,   -5,  48,    5, -10, -40,  550,    0,    0 },
/* JADEO          */ { GRI, 165,  -40, 240,    0,   0,   0,  350,    0, 1000 },
/* RECTA_GRUESA   */ { LIN, 108,   -8,   0,    0,   0, -30,    0,  400,    0 },
/* RECTA_HOYUELOS */ { LIN,  98,    0,   0,    0,   0, -30,    0,    0,  900 },
/* O              */ { BOO,  56,    0,  86,    0,   0, -30,    0,    0,    0 },
/* BESO           */ { BES,  52,    0,  84,    0,   0, -30,    0,    0,    0 },
/* ONDA           */ { OND, 105,    0,  18,    0,   0, -30,    0,    0,    0 },
/* MUECA          */ { LIN,  60,  -28,   0,    0,   0, -80,    0,    0,    0 },
/* RONCA          */ { BOO,  40,    0,  40,    0,   0, -20,    0,    0,    0 },
/* RONCA_ABIERTA  */ { BOO,  46,    0,  64,    0,   0, -20,    0,    0,    0 },
/* SED_LENGUA     */ { GRI,  90,  -30,  40,    0,   0, -20,  950,    0,    0 },
/* SUSPIRO        */ { LIN,  85,  -48,   0,    0,   0, -20,    0,  300,    0 },
/* CASTANETEO     */ { DIE, 135,  -20,  42,    0,   0, -10,    0,    0,  600 },
/* MUECA_DIENTES  */ { DIE, 175,  -45,  48,   10,   0, -10,    0,    0,  900 },
/* HMM            */ { LIN,  70,  -12,   0,   22,  45, -20,    0,    0,  500 },
/* HMM_OTRO       */ { LIN,  70,  -12,   0,  -22, -45, -20,    0,    0,  500 },
/* MIMO           */ { TEA, 250,  100, 100,    0,   0,   0,    0,  700,  900 },
};

#undef LIN
#undef TEA
#undef DIE
#undef GRI
#undef SUM
#undef BOO
#undef BES
#undef OND

/* ================================================= hojas de exposición === */
/* Una clave: en el milisegundo `t` del ciclo, estos ojos, estas cejas, esta
 * boca y esta mirada. `oi`/`ci` son los de la izquierda de la pantalla. */
typedef struct {
    uint16_t t;
    uint8_t  oi, od, ci, cd, boca;
    int16_t  mx, my;
} clave_t;

typedef struct {
    const clave_t  *claves;
    uint8_t         n;
    uint16_t        periodo;
    const uint16_t *parpadeos;   /* cuándo parpadea, en el ciclo          */
    uint8_t         nparp;
    const uint16_t *saltos;      /* cuándo saltan las cejas               */
    uint8_t         nsaltos;
    int16_t         salto;       /* cuánto saltan, milésimas del lado     */
    int16_t         fx[RK_KIP_FX_COUNT];
} pista_t;

#define N(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))

/* CONTENTO — la lámina de feliz, en el orden en que la numeró: sereno, el
 * costado canchero, el guiño con la sonrisa teal, el ojo que se vuelve a
 * abrir, la sonrisa pícara con dientes y de vuelta a la sonrisa grande. */
static const clave_t K_FELIZ[] = {
    {    0, O_CONTENTO, O_CONTENTO, C_ARCO,   C_ARCO,   B_TEAL_GRANDE,    0,    0 },
    { 1400, O_CONTENTO, O_CONTENTO, C_ARCO,   C_ARCO,   B_TEAL_GRANDE,  350, -150 },
    { 2600, O_CONTENTO, O_CONTENTO, C_ARCO,   C_ARCO,   B_TEAL_GRANDE,  350, -150 },
    { 3000, O_PLANO,    O_PLANO,    C_RECTA,  C_RECTA,  B_SONRISA,        0,  100 },
    { 4100, O_PLANO,    O_PLANO,    C_RECTA,  C_RECTA,  B_SONRISA,        0,  100 },
    { 4500, O_CANCHERO, O_CANCHERO, C_ARCO,   C_ARCO,   B_SOBRADOR,    -500,  200 },
    { 5300, O_CANCHERO, O_CANCHERO, C_ARCO,   C_ARCO,   B_SOBRADOR,    -500,  200 },
    { 5550, O_GUINO,    O_ABIERTO,  C_BAJA,   C_ALZADA, B_SOBRADOR,       0,    0 },
    { 5800, O_GUINO,    O_ABIERTO,  C_BAJA,   C_ALZADA, B_TEAL_GUINO,     0,    0 },
    { 6900, O_GUINO,    O_ABIERTO,  C_BAJA,   C_ALZADA, B_TEAL_GUINO,     0,    0 },
    { 7150, O_ASOMA,    O_ABIERTO,  C_ARCO,   C_ALZADA, B_TEAL_GUINO,     0,    0 },
    { 7450, O_GRANDE,   O_GRANDE,   C_PICARA, C_PICARA, B_DIENTES,        0,    0 },
    { 9000, O_GRANDE,   O_GRANDE,   C_PICARA, C_PICARA, B_DIENTES,     -250,    0 },
    { 9450, O_CONTENTO, O_CONTENTO, C_ARCO,   C_ARCO,   B_TEAL_GRANDE,    0,    0 },
};
static const uint16_t P_FELIZ[] = { 900, 3600, 10300 };
static const uint16_t S_FELIZ[] = { 2000, 8200, 11000 };

/* SE AHOGA — la lámina del agua: boquea abajo del agua, con la boca que se
 * abre, se cierra y se tuerce, mirando el agua con fastidio. */
static const clave_t K_AHOGO[] = {
    {    0, O_AHOGO, O_AHOGO, C_FASTIDIO, C_FASTIDIO, B_AGUA,          -80, 700 },
    {  300, O_AHOGO, O_AHOGO, C_FASTIDIO, C_FASTIDIO, B_AGUA_TORCIDA,  -40, 720 },
    {  600, O_AHOGO, O_AHOGO, C_FASTIDIO, C_FASTIDIO, B_AGUA_GRANDE,     0, 760 },
    {  900, O_AHOGO, O_AHOGO, C_FASTIDIO, C_FASTIDIO, B_AGUA_CERRADA,   60, 700 },
    { 1200, O_AHOGO, O_AHOGO, C_FASTIDIO, C_FASTIDIO, B_AGUA,           80, 680 },
    { 1500, O_AHOGO, O_AHOGO, C_FASTIDIO, C_FASTIDIO, B_AGUA_TORCIDA,   40, 700 },
    { 1800, O_AHOGO, O_AHOGO, C_FASTIDIO, C_FASTIDIO, B_AGUA_CERRADA,  -60, 720 },
    { 2100, O_AHOGO, O_AHOGO, C_FASTIDIO, C_FASTIDIO, B_AGUA,         -100, 700 },
    { 2400, O_AHOGO, O_AHOGO, C_FASTIDIO, C_FASTIDIO, B_AGUA_CERRADA,  -60, 740 },
    { 2700, O_AHOGO, O_AHOGO, C_FASTIDIO, C_FASTIDIO, B_AGUA_GRANDE,  -120, 700 },
};
static const uint16_t P_AHOGO[] = { 2250 };
static const uint16_t S_AHOGO[] = { 580 };

/* A OSCURAS — la lámina de oscuro: aburrido y triste, con los párpados a
 * media asta, busca con la mirada de dónde puede venir la luz. */
static const clave_t K_OSCURO[] = {
    {    0, O_OSCURO, O_OSCURO, C_MECHON, C_MECHON, B_RAYA,  600, -700 },
    { 1300, O_OSCURO, O_OSCURO, C_MECHON, C_MECHON, B_RAYA,  600, -700 },
    { 1550, O_OSCURO, O_OSCURO, C_MECHON, C_MECHON, B_RAYA,  650, -450 },
    { 2300, O_OSCURO, O_OSCURO, C_MECHON, C_MECHON, B_RAYA,  650, -450 },
    { 2900, O_OSCURO, O_OSCURO, C_MECHON, C_MECHON, B_RAYA, -550, -650 },
    { 4100, O_OSCURO, O_OSCURO, C_MECHON, C_MECHON, B_RAYA, -550, -650 },
    { 4400, O_OSCURO, O_OSCURO, C_MECHON, C_MECHON, B_RAYA,  150, -850 },
    { 5500, O_OSCURO, O_OSCURO, C_MECHON, C_MECHON, B_RAYA,  150, -850 },
};
static const uint16_t P_OSCURO[] = { 2450, 5700 };
static const uint16_t S_OSCURO[] = { 4450 };

/* CALOR — la lámina de calor, cuadro por cuadro: la lengua afuera, el
 * jadeo con los ojos cerrados, los ojos irritados, y después el agobio: la
 * boca recta, soplando, el beso al aire y la mueca ondulada. */
static const clave_t K_CALOR[] = {
    {    0, O_PESADO,       O_PESADO,       C_AGOBIO, C_AGOBIO, B_LENGUA,           0, 300 },
    {  450, O_PESADO,       O_PESADO,       C_AGOBIO, C_AGOBIO, B_LENGUA,           0, 300 },
    {  650, O_PESADO,       O_PESADO,       C_AGOBIO, C_AGOBIO, B_JADEO_CHICO,      0, 300 },
    { 1000, O_PESADO,       O_PESADO,       C_AGOBIO, C_AGOBIO, B_JADEO_CHICO,      0, 300 },
    { 1350, O_PENA_CERRADO, O_PENA_CERRADO, C_PENA,   C_PENA,   B_JADEO,            0,   0 },
    { 1850, O_PENA_CERRADO, O_PENA_CERRADO, C_PENA,   C_PENA,   B_JADEO,            0,   0 },
    { 2050, O_ROJO,         O_ROJO,         C_PENA,   C_PENA,   B_JADEO,            0, 200 },
    { 2550, O_ROJO,         O_ROJO,         C_PENA,   C_PENA,   B_JADEO,            0, 200 },
    { 2800, O_CANSADO,      O_CANSADO,      C_PENA,   C_PENA,   B_RECTA_GRUESA,     0, 250 },
    { 3100, O_CANSADO,      O_CANSADO,      C_PENA,   C_PENA,   B_RECTA_GRUESA,     0, 250 },
    { 3250, O_CANSADO,      O_CANSADO,      C_PENA,   C_PENA,   B_RECTA_HOYUELOS, 150, 250 },
    { 3550, O_CANSADO,      O_CANSADO,      C_PENA,   C_PENA,   B_RECTA_HOYUELOS, 150, 250 },
    { 3750, O_CANSADO,      O_CANSADO,      C_PENA,   C_PENA,   B_O,              150, 250 },
    { 4150, O_CANSADO,      O_CANSADO,      C_PENA,   C_PENA,   B_O,              150, 250 },
    { 4350, O_CANSADO,      O_CANSADO,      C_PENA,   C_PENA,   B_BESO,          -100, 250 },
    { 4700, O_CANSADO,      O_CANSADO,      C_PENA,   C_PENA,   B_BESO,          -100, 250 },
    { 4900, O_CANSADO,      O_CANSADO,      C_PENA,   C_PENA,   B_ONDA,             0, 250 },
    { 5200, O_CANSADO,      O_CANSADO,      C_PENA,   C_PENA,   B_ONDA,             0, 250 },
};
static const uint16_t P_CALOR[] = { 2700 };

/* AIRE SECO — la lámina de las grietas: dos rendijas que miran de reojo a
 * un lado y al otro, con fastidio, y parpadean una vez. */
static const clave_t K_SECO[] = {
    {    0, O_RENDIJA, O_RENDIJA, C_LOSA, C_LOSA, B_MUECA,  850,  50 },
    { 1150, O_RENDIJA, O_RENDIJA, C_LOSA, C_LOSA, B_MUECA,  850,  50 },
    { 1350, O_RENDIJA, O_RENDIJA, C_LOSA, C_LOSA, B_MUECA, -850,  80 },
    { 2350, O_RENDIJA, O_RENDIJA, C_LOSA, C_LOSA, B_MUECA, -850,  80 },
    { 2550, O_RENDIJA, O_RENDIJA, C_LOSA, C_LOSA, B_MUECA,  850, -30 },
    { 3600, O_RENDIJA, O_RENDIJA, C_LOSA, C_LOSA, B_MUECA,  850, -30 },
    { 3800, O_RENDIJA, O_RENDIJA, C_LOSA, C_LOSA, B_MUECA,    0, 150 },
    { 4700, O_RENDIJA, O_RENDIJA, C_LOSA, C_LOSA, B_MUECA,    0, 150 },
};
static const uint16_t P_SECO[] = { 3100 };
static const uint16_t S_SECO[] = { 2560 };

/* DORMIDO — no está en la lámina: los ojos cerrados de "oscuro" y "calor",
 * las cejas mullidas y una "o" que respira. */
static const clave_t K_DORMIDO[] = {
    {    0, O_DORMIDO, O_DORMIDO, C_SUAVE, C_SUAVE, B_RONCA,         0, 0 },
    { 2100, O_DORMIDO, O_DORMIDO, C_SUAVE, C_SUAVE, B_RONCA_ABIERTA, 0, 0 },
};

/* SED — mira la tierra con la lengua afuera, levanta la vista y te pide. */
static const clave_t K_SED[] = {
    {    0, O_SED,  O_SED,  C_SED,   C_SED,   B_SED_LENGUA,   0,  800 },
    { 1800, O_SED,  O_SED,  C_SED,   C_SED,   B_SED_LENGUA, 150,  800 },
    { 2200, O_PIDE, O_PIDE, C_RUEGO, C_RUEGO, B_SUSPIRO,    -80, -150 },
    { 3700, O_PIDE, O_PIDE, C_RUEGO, C_RUEGO, B_SUSPIRO,    -80, -150 },
    { 4100, O_SED,  O_SED,  C_SED,   C_SED,   B_SED_LENGUA, 150,  820 },
    { 5600, O_SED,  O_SED,  C_SED,   C_SED,   B_SED_LENGUA,   0,  800 },
};
static const uint16_t P_SED[] = { 1100, 4800 };
static const uint16_t S_SED[] = { 2250 };

/* FRÍO — le castañetean los dientes y mira de un lado al otro. */
static const clave_t K_FRIO[] = {
    {    0, O_FRIO, O_FRIO, C_FRIO, C_FRIO, B_CASTANETEO, -300, 100 },
    { 1500, O_FRIO, O_FRIO, C_FRIO, C_FRIO, B_CASTANETEO, -300, 100 },
    { 1800, O_FRIO, O_FRIO, C_FRIO, C_FRIO, B_CASTANETEO,  300, 100 },
    { 3300, O_FRIO, O_FRIO, C_FRIO, C_FRIO, B_CASTANETEO,  300, 100 },
};
static const uint16_t P_FRIO[] = { 900, 2600 };

/* SOL DIRECTO — aprieta los ojos contra la luz, espía y los vuelve a
 * apretar. */
static const clave_t K_SOL[] = {
    {    0, O_APRIETA, O_APRIETA, C_ENCANDILADO, C_ENCANDILADO, B_MUECA_DIENTES,   0,    0 },
    { 1500, O_APRIETA, O_APRIETA, C_ENCANDILADO, C_ENCANDILADO, B_MUECA_DIENTES,   0,    0 },
    { 1750, O_ESPIA,   O_ESPIA,   C_LOSA,        C_LOSA,        B_MUECA_DIENTES, 200, -300 },
    { 2400, O_ESPIA,   O_ESPIA,   C_LOSA,        C_LOSA,        B_MUECA_DIENTES, 200, -300 },
    { 2600, O_APRIETA, O_APRIETA, C_ENCANDILADO, C_ENCANDILADO, B_MUECA_DIENTES,   0,    0 },
};

/* SIN DATOS — no sabe qué pasa: una ceja arriba, la otra abajo, y a los
 * dos segundos se le dan vuelta. */
static const clave_t K_DUDA[] = {
    {    0, O_DUDA_CHICO,  O_DUDA_GRANDE, C_DUDA_BAJA, C_DUDA_ALTA, B_HMM,      -450, -550 },
    { 2000, O_DUDA_CHICO,  O_DUDA_GRANDE, C_DUDA_BAJA, C_DUDA_ALTA, B_HMM,      -450, -550 },
    { 2500, O_DUDA_GRANDE, O_DUDA_CHICO,  C_DUDA_ALTA, C_DUDA_BAJA, B_HMM_OTRO,  450, -550 },
    { 4500, O_DUDA_GRANDE, O_DUDA_CHICO,  C_DUDA_ALTA, C_DUDA_BAJA, B_HMM_OTRO,  450, -550 },
};
static const uint16_t P_DUDA[] = { 1200, 3700 };

/* DESCONECTADO — la cara de nada de "feliz 1", esperando. */
static const clave_t K_ESPERA[] = {
    {    0, O_LENTE, O_LENTE, C_RECTA, C_RECTA, B_LENTE, -450, 550 },
    { 2500, O_LENTE, O_LENTE, C_RECTA, C_RECTA, B_LENTE, -450, 550 },
    { 3000, O_LENTE, O_LENTE, C_RECTA, C_RECTA, B_LENTE,  450, 550 },
    { 5500, O_LENTE, O_LENTE, C_RECTA, C_RECTA, B_LENTE,  450, 550 },
};
static const uint16_t P_ESPERA[] = { 1500, 4300 };

/*                                                                       agua grie sudo calo zzz  niev duda espe sol  sed  ceno */
static const pista_t PISTAS[RK_MOOD_COUNT] = {
/* UNKNOWN     */ { K_DUDA,    N(K_DUDA),    5000, P_DUDA,   N(P_DUDA),   NULL,     0,            0, {   0,   0,   0,   0,   0,   0,1000,   0,   0,   0,   0 } },
/* OFFLINE     */ { K_ESPERA,  N(K_ESPERA),  6000, P_ESPERA, N(P_ESPERA), NULL,     0,            0, {   0,   0,   0,   0,   0,   0,   0,1000,   0,   0,   0 } },
/* SLEEPING    */ { K_DORMIDO, N(K_DORMIDO), 4200, NULL,     0,           NULL,     0,            0, {   0,   0,   0,   0,1000,   0,   0,   0,   0,   0,   0 } },
/* HAPPY       */ { K_FELIZ,   N(K_FELIZ),  12000, P_FELIZ,  N(P_FELIZ),  S_FELIZ,  N(S_FELIZ),  55, {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 } },
/* THIRSTY     */ { K_SED,     N(K_SED),     6000, P_SED,    N(P_SED),    S_SED,    N(S_SED),    35, {   0,   0,   0,   0,   0,   0,   0,   0,   0,1000,   0 } },
/* DROWNING    */ { K_AHOGO,   N(K_AHOGO),   3000, P_AHOGO,  N(P_AHOGO),  S_AHOGO,  N(S_AHOGO),  25, {1000,   0,   0,   0,   0,   0,   0,   0,   0,   0,1000 } },
/* COLD        */ { K_FRIO,    N(K_FRIO),    3600, P_FRIO,   N(P_FRIO),   NULL,     0,            0, {   0,   0,   0,   0,   0,1000,   0,   0,   0,   0,   0 } },
/* HOT         */ { K_CALOR,   N(K_CALOR),   5600, P_CALOR,  N(P_CALOR),  NULL,     0,            0, {   0,   0,1000,1000,   0,   0,   0,   0,   0,   0,   0 } },
/* SCORCHED    */ { K_SOL,     N(K_SOL),     4000, NULL,     0,           NULL,     0,            0, {   0,   0,   0,   0,   0,   0,   0,   0,1000,   0, 700 } },
/* DARK        */ { K_OSCURO,  N(K_OSCURO),  6000, P_OSCURO, N(P_OSCURO), S_OSCURO, N(S_OSCURO), 25, {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 } },
/* PARCHED_AIR */ { K_SECO,    N(K_SECO),    5000, P_SECO,   N(P_SECO),   S_SECO,   N(S_SECO),   20, {   0,1000,   0,   0,   0,   0,   0,   0,   0,   0,   0 } },
};

static const pista_t *pista_de(rk_mood_t mood)
{
    if ((int)mood < 0 || mood >= RK_MOOD_COUNT) {
        mood = RK_MOOD_UNKNOWN;
    }
    return &PISTAS[mood];
}

uint32_t rk_kip_periodo(rk_mood_t mood)
{
    return pista_de(mood)->periodo;
}

/* ============================================================ curvas === */
/* Todas en milésimas: entra un avance de 0 a 1000 y sale otro. */

/* Arranca y termina suave. */
static int32_t suave(int32_t p)
{
    int64_t q = p < 0 ? 0 : p > 1000 ? 1000 : p;
    return (int32_t)(q * q * (3000 - 2 * q) / 1000000);
}

/* Se pasa un diez por ciento y vuelve: el salto de las cejas. Es la curva
 * "back out" de siempre, con s = 1,70158. */
static int32_t salto(int32_t p)
{
    int64_t q = (p < 0 ? 0 : p > 1000 ? 1000 : p) - 1000;
    return (int32_t)(1000 + 27016 * q * q * q / 10000000000LL + 17016 * q * q / 10000000LL);
}

/* La mirada: llega en el primer tercio del tramo, rápido al principio, y se
 * queda. Un ojo no se desliza: salta y fija. */
static int32_t sacada(int32_t p)
{
    int64_t q = p * 3 > 1000 ? 1000 : (p < 0 ? 0 : p * 3);
    int64_t r = 1000 - q;
    return (int32_t)(1000 - r * r * r / 1000000);
}

static int16_t mezclar(int a, int b, int32_t k)
{
    return (int16_t)(a + (int32_t)(b - a) * k / 1000);
}

/* ============================================================== pose === */
static void ojo_entre(rk_kip_ojo_t *o, const rk_kip_ojo_t *a, const rk_kip_ojo_t *b,
                      int32_t k)
{
    o->a       = mezclar(a->a, b->a, k);
    o->b       = mezclar(a->b, b->b, k);
    o->incl    = mezclar(a->incl, b->incl, k);
    o->redondo = mezclar(a->redondo, b->redondo, k);
    o->pico    = mezclar(a->pico, b->pico, k);
    o->ancho   = mezclar(a->ancho, b->ancho, k);
    o->dy      = mezclar(a->dy, b->dy, k);
    o->iris    = mezclar(a->iris, b->iris, k);
    o->pliegue = mezclar(a->pliegue, b->pliegue, k);
    o->ojera   = mezclar(a->ojera, b->ojera, k);
    o->risa    = mezclar(a->risa, b->risa, k);
    o->rojo    = mezclar(a->rojo, b->rojo, k);
    o->pestana = mezclar(a->pestana, b->pestana, k);
    o->apagado = mezclar(a->apagado, b->apagado, k);
}

static void ceja_entre(rk_kip_ceja_t *c, const rk_kip_ceja_t *a, const rk_kip_ceja_t *b,
                       int32_t k)
{
    c->dy     = mezclar(a->dy, b->dy, k);
    c->dx     = mezclar(a->dx, b->dx, k);
    c->ang    = mezclar(a->ang, b->ang, k);
    c->arco   = mezclar(a->arco, b->arco, k);
    c->grosor = mezclar(a->grosor, b->grosor, k);
    c->tupido = mezclar(a->tupido, b->tupido, k);
    c->cola   = mezclar(a->cola, b->cola, k);
    c->largo  = mezclar(a->largo, b->largo, k);
}

/* La boca cambia de forma sólo cerrada: si las dos son de estilos
 * distintos, la primera mitad del tramo se cierra y la segunda se abre ya
 * con la forma nueva. `p` es el avance; `k`, el avance con su curva. */
static void boca_entre(rk_kip_boca_t *o, const rk_kip_boca_t *a, const rk_kip_boca_t *b,
                       int32_t p, int32_t k)
{
    o->ancho    = mezclar(a->ancho, b->ancho, k);
    o->curva    = mezclar(a->curva, b->curva, k);
    o->abre     = mezclar(a->abre, b->abre, k);
    o->ladeo    = mezclar(a->ladeo, b->ladeo, k);
    o->dx       = mezclar(a->dx, b->dx, k);
    o->dy       = mezclar(a->dy, b->dy, k);
    o->lengua   = mezclar(a->lengua, b->lengua, k);
    o->labio    = mezclar(a->labio, b->labio, k);
    o->comisura = mezclar(a->comisura, b->comisura, k);
    o->estilo   = a->estilo;
    if (a->estilo != b->estilo) {
        if (p < 500) {
            int32_t c = 1000 - suave(p * 2);
            o->abre = (int16_t)(a->abre * c / 1000);
            o->lengua = (int16_t)(a->lengua * c / 1000);
        } else {
            int32_t c = suave(p * 2 - 1000);
            o->estilo = b->estilo;
            o->abre = (int16_t)(b->abre * c / 1000);
            o->lengua = (int16_t)(b->lengua * c / 1000);
        }
    }
}

/* Dos poses. `p` es el avance crudo; si `curvar` es falso ya viene con su
 * curva (el reloj de la transición de ánimo la pone). */
static void pose_entre(rk_kip_pose_t *o, const rk_kip_pose_t *a, const rk_kip_pose_t *b,
                       int32_t p, bool curvar)
{
    int32_t k = curvar ? suave(p) : p;
    int32_t s = salto(p);
    int32_t m = curvar ? sacada(p) : p;
    rk_kip_pose_t r;
    int i;

    for (i = 0; i < 2; i++) {
        ojo_entre(&r.ojo[i], &a->ojo[i], &b->ojo[i], k);
        ceja_entre(&r.ceja[i], &a->ceja[i], &b->ceja[i], s);
    }
    boca_entre(&r.boca, &a->boca, &b->boca, p, k);
    r.mira_x = mezclar(a->mira_x, b->mira_x, m);
    r.mira_y = mezclar(a->mira_y, b->mira_y, m);
    r.dy = mezclar(a->dy, b->dy, k);
    for (i = 0; i < RK_KIP_FX_COUNT; i++) {
        r.fx[i] = mezclar(a->fx[i], b->fx[i], k);
    }
    *o = r;
}

static void pose_de_clave(rk_kip_pose_t *o, const clave_t *c, const int16_t *fx)
{
    o->ojo[0] = OJOS[c->oi];
    o->ojo[1] = OJOS[c->od];
    o->ceja[0] = CEJAS[c->ci];
    o->ceja[1] = CEJAS[c->cd];
    o->boca = BOCAS[c->boca];
    o->mira_x = c->mx;
    o->mira_y = c->my;
    o->dy = 0;
    memcpy(o->fx, fx, sizeof o->fx);
}

/* Cuánto de un gesto de `dura` ms que arrancó en `t0` se ve en `f`, dentro
 * de un ciclo de `periodo`: 0 si todavía no empezó o ya terminó. */
static int32_t desde_que(uint32_t f, uint16_t t0, uint32_t periodo, uint32_t dura)
{
    uint32_t d = (f + periodo - t0) % periodo;
    return d < dura ? (int32_t)d : -1;
}

/* Cierra un ojo `k` milésimas, bajando el párpado de arriba hasta la curva
 * de abajo. Un ojo que ya está cerrado se queda como está. */
static void cerrar(rk_kip_ojo_t *o, int32_t k)
{
    if (o->a + o->b > 0 && k > 0) {
        int cerrado = -o->b - 30;
        o->a = mezclar(o->a, cerrado, k > 1000 ? 1000 : k);
    }
}

/* La pose de un ánimo en `t`, con sus parpadeos, sus cejazos y la mirada que
 * deriva sola. */
static void pose_de_animo(rk_kip_pose_t *o, rk_mood_t mood, uint32_t t)
{
    const pista_t *pi = pista_de(mood);
    uint32_t f = t % pi->periodo;
    rk_kip_pose_t a, b;
    uint32_t t0, t1;
    int i, j;

    /* La clave vigente y la siguiente; la última vuelve a la primera. */
    for (i = pi->n - 1; i > 0 && pi->claves[i].t > f; i--) {
    }
    j = (i + 1 < pi->n) ? i + 1 : 0;
    t0 = pi->claves[i].t;
    t1 = (j == 0) ? pi->periodo : pi->claves[j].t;
    pose_de_clave(&a, &pi->claves[i], pi->fx);
    pose_de_clave(&b, &pi->claves[j], pi->fx);
    pose_entre(o, &a, &b, t1 > t0 ? (int32_t)((f - t0) * 1000u / (t1 - t0)) : 1000, true);

    /* Los parpadeos: 90 ms bajando, 40 cerrado, 90 subiendo. Las cejas
     * acompañan un poco: nadie parpadea con la frente quieta. */
    for (i = 0; i < pi->nparp; i++) {
        int32_t d = desde_que(f, pi->parpadeos[i], pi->periodo, 220u);
        int32_t k;
        if (d < 0) {
            continue;
        }
        k = d < 90 ? suave(d * 1000 / 90) : d < 130 ? 1000 : 1000 - suave((d - 130) * 1000 / 90);
        cerrar(&o->ojo[0], k);
        cerrar(&o->ojo[1], k);
        o->ceja[0].dy = (int16_t)(o->ceja[0].dy + 14 * k / 1000);
        o->ceja[1].dy = (int16_t)(o->ceja[1].dy + 14 * k / 1000);
    }

    /* Los cejazos: las cejas saltan, se pasan un poco, y bajan despacio. */
    for (i = 0; i < pi->nsaltos; i++) {
        int32_t d = desde_que(f, pi->saltos[i], pi->periodo, 450u);
        int32_t k;
        if (d < 0) {
            continue;
        }
        k = d < 150 ? salto(d * 1000 / 150) : 1000 - suave((d - 150) * 1000 / 300);
        for (j = 0; j < 2; j++) {
            o->ceja[j].dy = (int16_t)(o->ceja[j].dy - pi->salto * k / 1000);
            o->ceja[j].arco = (int16_t)(o->ceja[j].arco + pi->salto * k / 3000);
        }
    }

    /* El castañeteo del frío: la boca tiembla ocho veces por segundo. Con
     * un período que entra justo en el ciclo, para que el ciclo empalme. */
    if (mood == RK_MOOD_COLD) {
        int s = rk_sin8((uint8_t)(f * 256u / 120u));
        o->boca.abre = (int16_t)(o->boca.abre + (s < 0 ? -s : s) * 14 / 127 - 7);
    }

    /* Un ojo perfectamente quieto se ve de muñeco: la mirada deriva. */
    o->mira_x = (int16_t)(o->mira_x + rk_sin8((uint8_t)(t / 61u)) * 45 / 127);
    o->mira_y = (int16_t)(o->mira_y + rk_sin8((uint8_t)(t / 97u + 40u)) * 25 / 127);
}

/* Lo que pone cuando lo acarician: ^ ^, las cejas arriba y la sonrisa teal
 * grande. Los efectos se van: acariciado no se ahoga. */
static void pose_mimo(rk_kip_pose_t *o)
{
    static const int16_t nada[RK_KIP_FX_COUNT] = { 0 };
    clave_t c = { 0, O_MIMO, O_MIMO, C_MIMO, C_MIMO, B_MIMO, 0, 0 };
    pose_de_clave(o, &c, nada);
}

static int acotar(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

void rk_kip_pose(rk_kip_pose_t *out, const rk_kip_escena_t *e)
{
    rk_kip_pose_t p;
    int i;

    if (out == NULL || e == NULL) {
        return;
    }
    pose_de_animo(&p, e->hacia, e->t_ms);
    if (e->desde != e->hacia && e->mezcla < 100u) {
        rk_kip_pose_t a;
        pose_de_animo(&a, e->desde, e->t_ms);
        pose_entre(&p, &a, &p, (int32_t)e->mezcla * 10, false);
    }
    if (e->mimo > 0u) {
        rk_kip_pose_t m;
        pose_mimo(&m);
        pose_entre(&p, &p, &m, (int32_t)(e->mimo > 100u ? 100u : e->mimo) * 10, false);
    }
    if (e->mira_x != 0 || e->mira_y != 0) {
        p.mira_x = (int16_t)acotar(p.mira_x + acotar(e->mira_x, -100, 100) * 10, -1000, 1000);
        p.mira_y = (int16_t)acotar(p.mira_y + acotar(e->mira_y, -100, 100) * 10, -1000, 1000);
    }
    if (e->preocupado > 0u) {
        /* Mira con preocupación al vecino que tiene sed: las cejas suben por
         * el lado de adentro y la sonrisa se afloja. */
        int k = e->preocupado > 100u ? 100 : (int)e->preocupado;
        for (i = 0; i < 2; i++) {
            p.ceja[i].ang = (int16_t)(p.ceja[i].ang + 300 * k / 100);
            p.ceja[i].dy = (int16_t)(p.ceja[i].dy - 30 * k / 100);
        }
        if (p.boca.curva > 0) {
            p.boca.curva = (int16_t)(p.boca.curva - p.boca.curva * 7 * k / 1000);
        }
    }
    if (e->cierre > 0u) {
        int32_t k = (int32_t)(e->cierre > 100u ? 100u : e->cierre) * 10;
        cerrar(&p.ojo[0], k);
        cerrar(&p.ojo[1], k);
    }
    *out = p;
}

/* ============================================================ dibujo === */
typedef struct {
    int32_t x, y;
} pt_t;

typedef struct {
    rk_fb_t                *fb;
    const rk_persona_t     *p;
    const rk_color_t       *c;      /* los colores, por RK_KIP_*          */
    int32_t                 u;      /* lado corto, en pixeles             */
    int32_t                 cx, cy; /* centro de la cara, Q4              */
    uint32_t                t;
} lienzo_t;

/* Milésimas del lado a Q4. */
#define MIL(l, v)  ((int32_t)((int64_t)(l)->u * 16 * (v) / 1000))
/* Un punto en milésimas, relativo al centro de la cara. */
static pt_t en(const lienzo_t *l, int32_t x, int32_t y)
{
    pt_t r;
    r.x = l->cx + MIL(l, x);
    r.y = l->cy + MIL(l, y);
    return r;
}

static pt_t punto(int32_t x, int32_t y)
{
    pt_t r;
    r.x = x;
    r.y = y;
    return r;
}

/* De `a` hacia `b`, `k` milésimas. */
static pt_t hacia(pt_t a, pt_t b, int32_t k)
{
    return punto(a.x + (int32_t)((int64_t)(b.x - a.x) * k / 1000),
                 a.y + (int32_t)((int64_t)(b.y - a.y) * k / 1000));
}

/* Bézier cuadrática y cúbica, con t en 0..1024. */
static pt_t bez2(pt_t a, pt_t b, pt_t c, int32_t t)
{
    int64_t s = 1024 - t, T = t;
    return punto((int32_t)((a.x * s * s + 2 * b.x * s * T + c.x * T * T) / 1048576),
                 (int32_t)((a.y * s * s + 2 * b.y * s * T + c.y * T * T) / 1048576));
}

static pt_t bez3(pt_t a, pt_t b, pt_t c, pt_t d, int32_t t)
{
    int64_t s = 1024 - t, T = t;
    return punto((int32_t)((a.x * s * s * s + 3 * b.x * s * s * T + 3 * c.x * s * T * T
                            + d.x * T * T * T) / 1073741824),
                 (int32_t)((a.y * s * s * s + 3 * b.y * s * s * T + 3 * c.y * s * T * T
                            + d.y * T * T * T) / 1073741824));
}

/* El control de una cuadrática que pasa por `m` en la mitad. */
static pt_t control_por(pt_t a, pt_t m, pt_t b)
{
    return punto(2 * m.x - (a.x + b.x) / 2, 2 * m.y - (a.y + b.y) / 2);
}

/* ------------------------------------------------------- el polígono --- */
static int32_t g_xy[2 * RK_AA_POLI_MAX];
static int     g_n;

static void poli_ini(void)
{
    g_n = 0;
}

static void poli_pt(pt_t p)
{
    if (g_n < RK_AA_POLI_MAX) {
        g_xy[2 * g_n] = p.x;
        g_xy[2 * g_n + 1] = p.y;
        g_n++;
    }
}

static void poli_pintar(lienzo_t *l, rk_color_t col, uint8_t alfa)
{
    rk_relleno_t r = rk_plano(col);
    rk_aa_poligono(l->fb, g_xy, g_n, &r, alfa, NULL, 0);
}

/* ------------------------------------------------------------ pincel --- */
#define KIP_MUESTRAS 32

/* Normal unitaria en Q12 del tramo en `i`, apuntando a la izquierda del
 * sentido del trazo. */
static void normal(const pt_t *p, int n, int i, int32_t *nx, int32_t *ny)
{
    int a = i > 0 ? i - 1 : 0, b = i + 1 < n ? i + 1 : n - 1;
    int64_t dx = p[b].x - p[a].x, dy = p[b].y - p[a].y;
    int64_t len = (int64_t)rk_isqrt64((uint64_t)(dx * dx + dy * dy));

    if (len == 0) {
        *nx = 0;
        *ny = -4096;
        return;
    }
    *nx = (int32_t)(-dy * 4096 / len);
    *ny = (int32_t)(dx * 4096 / len);
}

/* Un trazo de pincel por los `n` puntos de `p`, con el ancho entero `w[i]`
 * (Q4) en cada uno y las puntas redondas. Es un solo polígono: se puede
 * pintar con alfa sin que se noten las uniones. */
static void pincel(lienzo_t *l, const pt_t *p, const int32_t *w, int n,
                   rk_color_t col, uint8_t alfa)
{
    /* Las puntas: media vuelta en cinco pasos (cos, sen de 45 en 45). */
    static const int32_t CS[3][2] = { { 2896, 2896 }, { 0, 4096 }, { -2896, 2896 } };
    int32_t nx[KIP_MUESTRAS], ny[KIP_MUESTRAS];
    int i, k;

    if (n < 2 || alfa == 0) {
        return;
    }
    if (n > KIP_MUESTRAS) {
        n = KIP_MUESTRAS;
    }
    for (i = 0; i < n; i++) {
        normal(p, n, i, &nx[i], &ny[i]);
    }
    poli_ini();
    for (i = 0; i < n; i++) {
        int32_t r = w[i] / 2;
        poli_pt(punto(p[i].x + nx[i] * r / 4096, p[i].y + ny[i] * r / 4096));
    }
    {
        /* La punta del final: de la izquierda, por adelante, a la derecha.
         * La tangente es la normal girada: (ny, -nx). */
        int32_t r = w[n - 1] / 2, x = nx[n - 1], y = ny[n - 1];
        for (k = 0; k < 3; k++) {
            int32_t ex = (x * CS[k][0] + y * CS[k][1]) / 4096;
            int32_t ey = (y * CS[k][0] - x * CS[k][1]) / 4096;
            poli_pt(punto(p[n - 1].x + ex * r / 4096, p[n - 1].y + ey * r / 4096));
        }
    }
    for (i = n - 1; i >= 0; i--) {
        int32_t r = w[i] / 2;
        poli_pt(punto(p[i].x - nx[i] * r / 4096, p[i].y - ny[i] * r / 4096));
    }
    {
        int32_t r = w[0] / 2, x = -nx[0], y = -ny[0];
        for (k = 0; k < 3; k++) {
            int32_t ex = (x * CS[k][0] + y * CS[k][1]) / 4096;
            int32_t ey = (y * CS[k][0] - x * CS[k][1]) / 4096;
            poli_pt(punto(p[0].x + ex * r / 4096, p[0].y + ey * r / 4096));
        }
    }
    poli_pintar(l, col, alfa);
}

/* El ancho a lo largo de un trazo: `w0` al principio, `w1` en la mitad y
 * `w2` al final, por una parábola. `t` en 0..1024. */
static int32_t perfil(int32_t w0, int32_t w1, int32_t w2, int32_t t)
{
    int64_t s = 1024 - t, T = t;
    int64_t wc = (4 * (int64_t)w1 - w0 - w2) / 2;
    int64_t w = (w0 * s * s + 2 * wc * s * T + w2 * T * T) / 1048576;
    return (int32_t)(w < 0 ? 0 : w);
}

/* Cuántas muestras pide una curva de este largo (en Q4): una cada dos
 * pixeles y medio, entre 5 y KIP_MUESTRAS. */
static int muestras(pt_t a, pt_t b, pt_t c)
{
    int32_t largo = (int32_t)rk_isqrt64((uint64_t)((int64_t)(b.x - a.x) * (b.x - a.x)
                                                   + (int64_t)(b.y - a.y) * (b.y - a.y)))
                  + (int32_t)rk_isqrt64((uint64_t)((int64_t)(c.x - b.x) * (c.x - b.x)
                                                   + (int64_t)(c.y - b.y) * (c.y - b.y)));
    int n = (int)(largo / 40) + 2;
    return n < 5 ? 5 : n > KIP_MUESTRAS ? KIP_MUESTRAS : n;
}

/* Un trazo por una cuadrática de `a` a `c` con control `b`. */
static void trazo(lienzo_t *l, pt_t a, pt_t b, pt_t c, int32_t w0, int32_t w1,
                  int32_t w2, rk_color_t col, uint8_t alfa)
{
    pt_t p[KIP_MUESTRAS];
    int32_t w[KIP_MUESTRAS];
    int n = muestras(a, b, c), i;

    for (i = 0; i < n; i++) {
        int32_t t = (int32_t)(i * 1024 / (n - 1));
        p[i] = bez2(a, b, c, t);
        w[i] = perfil(w0, w1, w2, t);
    }
    pincel(l, p, w, n, col, alfa);
}

/* Un trazo por una línea quebrada de puntos en milésimas, del grosor `g`,
 * dibujado hasta `hasta` milésimas de su largo. Es lo que dibujan las
 * grietas y los garabatos. `hierve` sacude cada vértice un poco, ocho veces
 * por segundo: la línea "hierve" como en un dibujo animado hecho a mano. */
static void quebrada(lienzo_t *l, const int16_t *xy, int n, int32_t g, int32_t hasta,
                     int32_t dx, int32_t dy, int hierve, rk_color_t col, uint8_t alfa)
{
    pt_t p[KIP_MUESTRAS];
    int32_t w[KIP_MUESTRAS];
    int32_t tramos = (int32_t)(n - 1) * hasta / 1000;
    int32_t resto = (int32_t)(n - 1) * hasta % 1000;
    uint16_t cuadro = (uint16_t)(l->t / 125u);
    int i, m;

    if (n < 2 || hasta <= 0) {
        return;
    }
    if (n > KIP_MUESTRAS) {
        n = KIP_MUESTRAS;
    }
    for (i = 0; i < n; i++) {
        int32_t hx = 0, hy = 0;
        if (hierve) {
            uint16_t h = rk_hash((uint16_t)(cuadro * 31u + (uint16_t)i * 7u));
            hx = (int32_t)(h % 9u) - 4;
            hy = (int32_t)((h >> 4) % 9u) - 4;
        }
        p[i] = en(l, xy[2 * i] + dx + hx * hierve, xy[2 * i + 1] + dy + hy * hierve);
    }
    m = (int)tramos + 1;
    if (m < n && resto > 0) {
        p[m] = hacia(p[m - 1], p[m], resto);
        m++;
    }
    if (m < 2) {
        return;
    }
    for (i = 0; i < m; i++) {
        /* Afinado en las puntas, como un trazo de pluma. */
        int32_t t = (int32_t)(i * 1024 / (m - 1));
        w[i] = perfil(g * 55 / 100, g, g * 45 / 100, t);
    }
    pincel(l, p, w, m, col, alfa);
}

/* ---------------------------------------------------------------- ojo --- */
#define OJO_N 14

typedef struct {
    pt_t    arriba[OJO_N];   /* de la comisura de afuera a la de adentro    */
    pt_t    abajo[OJO_N];    /* de la de adentro a la de afuera             */
    int32_t xy[4 * OJO_N];   /* el contorno, para recortar el iris          */
    int     n;
    bool    cerrado;
} ojo_geo_t;

static void ojo_geometria(ojo_geo_t *g, pt_t O, pt_t I, int32_t A, int32_t B,
                          const rk_kip_ojo_t *o)
{
    int32_t s = (1000 - acotar(o->redondo, 0, 1000)) / 3;
    int32_t s1 = acotar(s - o->pico, 0, 1000), s2 = acotar(1000 - s - o->pico, 0, 1000);
    pt_t P1, P2, Q1, Q2;
    int i;

    g->cerrado = A + B <= 0;
    if (A + B < 0) {
        /* Cerrado: las dos curvas se vuelven una, a mitad de camino. */
        int32_t m = (B - A) / 2;
        A = -m;
        B = m;
    }
    P1 = hacia(O, I, s1);
    P2 = hacia(O, I, s2);
    P1.y -= A * 4 / 3;
    P2.y -= A * 4 / 3;
    Q1 = hacia(I, O, s);
    Q2 = hacia(I, O, 1000 - s);
    Q1.y += B * 4 / 3;
    Q2.y += B * 4 / 3;
    g->n = 0;
    for (i = 0; i < OJO_N; i++) {
        int32_t t = (int32_t)(i * 1024 / (OJO_N - 1));
        g->arriba[i] = bez3(O, P1, P2, I, t);
        g->abajo[i] = bez3(I, Q1, Q2, O, t);
    }
    for (i = 0; i < OJO_N; i++) {
        g->xy[2 * g->n] = g->arriba[i].x;
        g->xy[2 * g->n + 1] = g->arriba[i].y;
        g->n++;
    }
    for (i = 1; i < OJO_N - 1; i++) {
        g->xy[2 * g->n] = g->abajo[i].x;
        g->xy[2 * g->n + 1] = g->abajo[i].y;
        g->n++;
    }
}

/* Un tramo de una curva de muestras, de `desde` a `hasta` milésimas de su
 * recorrido, corrido `dy` (Q4), como trazo de ancho `w0/w1/w2`. */
static void tramo_de(lienzo_t *l, const pt_t *c, int n, int32_t desde, int32_t hasta,
                     int32_t dy, int32_t w0, int32_t w1, int32_t w2, rk_color_t col,
                     uint8_t alfa)
{
    enum { M = 16 };
    pt_t p[M];
    int32_t w[M];
    int i;

    for (i = 0; i < M; i++) {
        /* La posición en milésimas de índice de la curva. */
        int32_t pos = (desde + (hasta - desde) * i / (M - 1)) * (n - 1);
        int k = (int)(pos / 1000);
        int32_t f = pos % 1000;
        if (k >= n - 1) {
            k = n - 2;
            f = 1000;
        }
        p[i] = hacia(c[k], c[k + 1], f);
        p[i].y += dy;
        w[i] = perfil(w0, w1, w2, (int32_t)(i * 1024 / (M - 1)));
    }
    pincel(l, p, w, M, col, alfa);
}

static void ojo(lienzo_t *l, const rk_kip_ojo_t *o, int lado, int32_t mira_x,
                int32_t mira_y)
{
    const rk_persona_t *p = l->p;
    int32_t rx0 = MIL(l, p->ojo_rx * 10), ry0 = MIL(l, p->ojo_ry * 10);
    int32_t ex = l->cx + lado * MIL(l, p->ojo_dx * 10);
    int32_t ey = l->cy + MIL(l, p->ojo_dy * 10) + ry0 * o->dy / 1000;
    int32_t rx = rx0 * o->ancho / 1000;
    int32_t inc = ry0 * o->incl / 1000;
    int32_t A = ry0 * o->a / 1000, B = ry0 * o->b / 1000;
    int32_t linea = MIL(l, 21), fina = MIL(l, 11);
    pt_t O = punto(ex + lado * rx, ey + inc / 2), I = punto(ex - lado * rx, ey - inc / 2);
    ojo_geo_t g;
    rk_relleno_t r;
    rk_forma_t f;

    ojo_geometria(&g, O, I, A, B, o);

    /* 1. las ojeras, abajo de todo: una media luna granate pegada al párpado */
    if (o->ojera > 0) {
        int i;
        poli_ini();
        for (i = 1; i < OJO_N - 1; i++) {
            poli_pt(g.abajo[i]);
        }
        for (i = OJO_N - 2; i >= 1; i--) {
            int32_t k = rk_sin8((uint8_t)(i * 128 / (OJO_N - 1)));
            pt_t q = g.abajo[i];
            q.y += ry0 * 10 / 100 + ry0 * 34 / 100 * k / 127;
            poli_pt(q);
        }
        poli_pintar(l, l->c[RK_KIP_OJERA], (uint8_t)(o->ojera * 200 / 1000));
    }

    if (!g.cerrado) {
        int32_t ri = rx0 * o->iris / 1000;
        int32_t cima = (O.y + I.y) / 2 - A, fondo = (O.y + I.y) / 2 + B;
        int32_t ix = ex + (int32_t)((int64_t)mira_x * (rx - ri) * 70 / 100000);
        /* La mirada recorre el ojo entero: mirando arriba, el párpado le
         * corta el iris, que es como lo dibuja ella. */
        int32_t iy = (cima + fondo) / 2 + (int32_t)((int64_t)mira_y * (fondo - cima) / 2000);
        rk_color_t iris = rk_mix(l->c[RK_KIP_IRIS], l->c[RK_KIP_APAGADO],
                                 (uint8_t)(o->apagado * 255 / 1000));
        int32_t aro = ri * 15 / 100 > 12 ? ri * 15 / 100 : 12;

        /* 2. el blanco del ojo; irritado, se pone rosado */
        r = rk_plano(rk_mix(l->c[RK_KIP_BLANCO], l->c[RK_KIP_RUBOR],
                            (uint8_t)(o->rojo * 80 / 1000)));
        rk_aa_poligono(l->fb, g.xy, g.n, &r, 255, NULL, 0);

        /* 3. el iris, recortado por el ojo: aro, ámbar, pupila y brillo */
        f = rk_circulo_q4(ix, iy, ri);
        r = rk_plano(l->c[RK_KIP_ARO]);
        rk_aa_poligono(l->fb, g.xy, g.n, &r, 255, &f, 1);
        f = rk_circulo_q4(ix, iy, ri - aro);
        r = rk_plano(iris);
        rk_aa_poligono(l->fb, g.xy, g.n, &r, 255, &f, 1);
        f = rk_circulo_q4(ix, iy, ri * 50 / 100);
        r = rk_plano(l->c[RK_KIP_TINTA]);
        rk_aa_poligono(l->fb, g.xy, g.n, &r, 255, &f, 1);
        f = rk_circulo_q4(ix + ri * 26 / 100, iy - ri * 30 / 100, ri * 24 / 100);
        r = rk_plano(l->c[RK_KIP_NIEVE]);
        rk_aa_poligono(l->fb, g.xy, g.n, &r, 255, &f, 1);

        /* 4. el irritado: una raya rosada adentro del párpado de abajo */
        if (o->rojo > 0) {
            tramo_de(l, g.abajo, OJO_N, 150, 850, -fina, fina / 2, fina, fina / 2,
                     l->c[RK_KIP_RUBOR], (uint8_t)(o->rojo * 180 / 1000));
        }

        /* 5. los párpados: arriba grueso, abajo fino y sin llegar a las
         * puntas, que es como los dibuja ella */
        tramo_de(l, g.arriba, OJO_N, 0, 1000, 0, linea * 55 / 100, linea, linea * 75 / 100,
                 l->c[RK_KIP_TINTA], 255);
        tramo_de(l, g.abajo, OJO_N, 70, 930, 0, fina / 4, fina, fina / 4,
                 l->c[RK_KIP_TINTA], 255);
    } else {
        /* Cerrado: una sola línea, más gruesa en el medio. */
        tramo_de(l, g.arriba, OJO_N, 0, 1000, 0, linea * 40 / 100, linea * 125 / 100,
                 linea * 40 / 100, l->c[RK_KIP_TINTA], 255);
        if (o->pestana > 0) {
            /* Dormido: dos pestañitas hacia abajo, del lado de afuera. */
            int k;
            for (k = 0; k < 2; k++) {
                pt_t q = g.arriba[2 + k * 2];
                trazo(l, q, punto(q.x + lado * MIL(l, 6), q.y + MIL(l, 22)),
                      punto(q.x + lado * MIL(l, 14), q.y + MIL(l, 38)),
                      fina, fina * 70 / 100, fina / 4, l->c[RK_KIP_TINTA],
                      (uint8_t)(o->pestana * 255 / 1000));
            }
        }
    }

    /* 6. el pliegue de arriba del párpado. Con el ojo cerrado es más corto
     * y va del lado de adentro: si fuera tan largo como el ojo, el ^ se
     * leería como tres rayas paralelas. */
    if (o->pliegue > 0) {
        if (g.cerrado) {
            tramo_de(l, g.arriba, OJO_N, 380, 800, -ry0 * 48 / 100, fina / 4, fina * 80 / 100,
                     fina / 4, l->c[RK_KIP_TINTA], (uint8_t)(o->pliegue * 230 / 1000));
        } else {
            tramo_de(l, g.arriba, OJO_N, 180, 820, -ry0 * 30 / 100, fina / 4, fina * 80 / 100,
                     fina / 4, l->c[RK_KIP_TINTA], (uint8_t)(o->pliegue * 230 / 1000));
        }
    }
    /* 7. la mejilla que sube: una rayita abajo, del lado de afuera; con el
     * ojo cerrado, sólo en la punta. */
    if (o->risa > 0) {
        if (g.cerrado) {
            tramo_de(l, g.abajo, OJO_N, 740, 1000, ry0 * 42 / 100, fina / 4, fina * 80 / 100,
                     fina / 4, l->c[RK_KIP_TINTA], (uint8_t)(o->risa * 230 / 1000));
        } else {
            tramo_de(l, g.abajo, OJO_N, 520, 960, ry0 * 26 / 100, fina / 4, fina * 80 / 100,
                     fina / 4, l->c[RK_KIP_TINTA], (uint8_t)(o->risa * 230 / 1000));
        }
    }
    /* 8. las líneas que siguen de largo en las comisuras */
    if (o->pestana > 0 && !g.cerrado) {
        uint8_t a = (uint8_t)(o->pestana * 255 / 1000);
        pt_t q = g.arriba[0];
        trazo(l, q, punto(q.x + lado * MIL(l, 22), q.y), punto(q.x + lado * MIL(l, 42), q.y),
              fina, fina * 80 / 100, fina / 4, l->c[RK_KIP_TINTA], a);
        q = g.abajo[OJO_N - 1];
        trazo(l, q, punto(q.x + lado * MIL(l, 20), q.y - MIL(l, 2)),
              punto(q.x + lado * MIL(l, 38), q.y - MIL(l, 14)),
              fina, fina * 70 / 100, fina / 4, l->c[RK_KIP_TINTA], a);
        q = g.arriba[OJO_N - 1];
        trazo(l, q, punto(q.x - lado * MIL(l, 12), q.y), punto(q.x - lado * MIL(l, 24), q.y),
              fina * 80 / 100, fina * 60 / 100, fina / 4, l->c[RK_KIP_TINTA], a);
    }
}

/* -------------------------------------------------------------- ceja --- */
#define CEJA_N 22

static void ceja(lienzo_t *l, const rk_kip_ceja_t *c, int lado)
{
    const rk_persona_t *p = l->p;
    int32_t rx0 = MIL(l, p->ojo_rx * 10);
    int32_t bx = l->cx + lado * (MIL(l, p->ojo_dx * 10) + MIL(l, c->dx));
    int32_t by = l->cy + MIL(l, p->ojo_dy * 10) - MIL(l, p->ceja_alto * 10) + MIL(l, c->dy);
    int32_t L = rx0 * c->largo / 1000;
    int32_t G = MIL(l, c->grosor);
    int32_t k = (int32_t)((int64_t)L * c->ang / 1000);
    pt_t in = punto(bx - lado * L, by - k), out = punto(bx + lado * L, by + k);
    pt_t ctl = punto((in.x + out.x) / 2, (in.y + out.y) / 2 - 2 * MIL(l, c->arco));
    pt_t eje[CEJA_N], arriba[CEJA_N], abajo[CEJA_N];
    int i;

    for (i = 0; i < CEJA_N; i++) {
        int32_t t = (int32_t)(i * 1024 / (CEJA_N - 1));
        int32_t tm = t * 1000 / 1024;
        int32_t nx, ny, th, sube, pelo;
        eje[i] = bez2(in, ctl, out, t);
        /* El grosor: la punta de adentro redonda y llena, lo más gordo a un
         * cuarto, y de ahí se afina hacia la sien tanto como diga `cola`. */
        th = tm < 220 ? 820 + 180 * tm / 220
                      : 1000 - (tm - 220) * 850 / 780 * c->cola / 1000;
        th = G * th / 1000;
        {
            int a = i > 0 ? i - 1 : 0, b = i + 1 < CEJA_N ? i + 1 : CEJA_N - 1;
            pt_t pa = bez2(in, ctl, out, (int32_t)(a * 1024 / (CEJA_N - 1)));
            pt_t pb = bez2(in, ctl, out, (int32_t)(b * 1024 / (CEJA_N - 1)));
            int64_t dx = pb.x - pa.x, dy = pb.y - pa.y;
            int64_t len = (int64_t)rk_isqrt64((uint64_t)(dx * dx + dy * dy));
            if (len == 0) {
                nx = 0;
                ny = -4096;
            } else {
                nx = (int32_t)(-dy * 4096 / len);
                ny = (int32_t)(dx * 4096 / len);
                if (ny > 0) {           /* la normal que apunta arriba */
                    nx = -nx;
                    ny = -ny;
                }
            }
        }
        /* Lo tupido: el borde de arriba no es una curva, son mechones. Dos
         * ondas de distinto largo, para que no se vea un serrucho. */
        pelo = rk_sin8((uint8_t)(tm * 5 * 128 / 1000 + 20));
        pelo = (pelo < 0 ? -pelo : pelo) * 70 / 127
             + rk_sin8((uint8_t)(tm * 13 * 128 / 1000 + 90)) * 30 / 127 - 45;
        sube = th * 62 / 100 + th * 62 / 100 * c->tupido / 1000 * pelo / 100 * 45 / 100;
        arriba[i] = punto(eje[i].x + nx * sube / 4096, eje[i].y + ny * sube / 4096);
        abajo[i] = punto(eje[i].x - nx * (th * 38 / 100) / 4096,
                         eje[i].y - ny * (th * 38 / 100) / 4096);
    }

    poli_ini();
    for (i = 0; i < CEJA_N; i++) {
        poli_pt(abajo[i]);
    }
    for (i = CEJA_N - 1; i >= 0; i--) {
        poli_pt(arriba[i]);
    }
    {
        /* La punta de adentro, redonda: media vuelta de arriba a abajo. */
        pt_t m = punto((arriba[0].x + abajo[0].x) / 2, (arriba[0].y + abajo[0].y) / 2);
        int32_t hx = (arriba[0].x - m.x), hy = (arriba[0].y - m.y);
        int32_t tx = -lado * (int32_t)rk_isqrt64((uint64_t)((int64_t)hx * hx + (int64_t)hy * hy));
        poli_pt(punto(m.x + hx * 7 / 10 + tx * 7 / 10, m.y + hy * 7 / 10));
        poli_pt(punto(m.x + tx, m.y));
        poli_pt(punto(m.x - hx * 7 / 10 + tx * 7 / 10, m.y - hy * 7 / 10));
    }
    poli_pintar(l, l->c[RK_KIP_TINTA], 255);
}

/* -------------------------------------------------------------- boca --- */
typedef struct {
    pt_t izq, der;       /* las comisuras                                  */
    pt_t sup, inf;       /* los controles de la curva de arriba y de abajo */
    pt_t medio;          /* el centro de la boca                           */
    int32_t W, ab;       /* media boca y apertura, Q4                      */
} boca_geo_t;

/* La curva de abajo de una boca abierta, redonda: una cúbica con los
 * controles corridos hacia afuera, que da un fondo ancho y no una V. */
static pt_t inf_redonda(const boca_geo_t *g, int32_t t)
{
    int32_t y = g->medio.y + g->ab / 2;
    int32_t yc = (8 * y - g->izq.y - g->der.y) / 6;
    pt_t c1 = punto(g->der.x + g->W * 12 / 100, yc);
    pt_t c2 = punto(g->izq.x - g->W * 12 / 100, yc);
    return bez3(g->der, c1, c2, g->izq, t);
}

static void boca(lienzo_t *l, const rk_kip_boca_t *b)
{
    const rk_persona_t *p = l->p;
    boca_geo_t g;
    int32_t C, Ld, ancho = MIL(l, 15), fino = MIL(l, 7);
    const rk_color_t *c = l->c;
    int i;

    g.W = MIL(l, b->ancho) * p->boca_ancho / 20;
    g.ab = MIL(l, b->abre < 0 ? 0 : b->abre);
    C = MIL(l, b->curva);
    Ld = MIL(l, b->ladeo);
    g.medio = punto(l->cx + MIL(l, b->dx), l->cy + MIL(l, p->boca_dy * 10) + MIL(l, b->dy));
    g.izq = punto(g.medio.x - g.W, g.medio.y - C + Ld);
    g.der = punto(g.medio.x + g.W, g.medio.y - C - Ld);
    g.sup = control_por(g.izq, punto(g.medio.x, g.medio.y - g.ab / 2), g.der);
    g.inf = control_por(g.izq, punto(g.medio.x, g.medio.y + g.ab / 2), g.der);

    switch ((rk_kip_boca_estilo_t)b->estilo) {
    case RK_KIP_BOCA_TEAL:
    case RK_KIP_BOCA_DIENTES: {
        /* La media luna: la curva de arriba y la de abajo, rellenas. */
        bool teal = b->estilo == RK_KIP_BOCA_TEAL;
        int n = muestras(g.izq, g.inf, g.der);
        poli_ini();
        for (i = 0; i < n; i++) {
            poli_pt(bez2(g.izq, g.sup, g.der, (int32_t)(i * 1024 / (n - 1))));
        }
        for (i = n - 1; i >= 0; i--) {
            poli_pt(bez2(g.izq, g.inf, g.der, (int32_t)(i * 1024 / (n - 1))));
        }
        poli_pintar(l, teal ? c[RK_KIP_BOCA] : c[RK_KIP_BLANCO], 255);
        /* El labio de arriba, de tinta, sigue un poco más allá de la boca. */
        trazo(l, g.izq, g.sup, g.der, ancho * 25 / 100, teal ? ancho * 80 / 100 : ancho,
              ancho * 25 / 100, c[RK_KIP_TINTA], 255);
        if (!teal) {
            trazo(l, g.izq, g.inf, g.der, fino / 4, fino * 110 / 100, fino / 4,
                  c[RK_KIP_TINTA], 255);
        }
        break;
    }

    case RK_KIP_BOCA_GRITO: {
        /* Abierta y oscura. La lengua puede asomar afuera, abajo a la
         * izquierda, como en "calor 1": va primero, y la boca le pasa por
         * encima. */
        int n = 16;
        static int32_t hueco[2 * 34];
        int nh = 0;
        if (b->lengua > 0 && g.ab < MIL(l, 90)) {
            int32_t lx = g.medio.x - g.W * 30 / 100;
            int32_t ly = g.medio.y + g.ab / 2 + MIL(l, 14) * b->lengua / 1000;
            int32_t lw = g.W * 50 / 100, lh = MIL(l, 14) + MIL(l, 26) * b->lengua / 1000;
            rk_forma_t f = rk_elipse_q4(lx, ly, lw, lh);
            rk_aa_pintar(l->fb, &f, 1, c[RK_KIP_LENGUA], 255, 0, 0, l->fb->w, l->fb->h);
            trazo(l, punto(lx, ly - lh / 3), punto(lx - lw / 8, ly + lh / 4),
                  punto(lx - lw / 5, ly + lh / 2), fino / 3, fino * 70 / 100, fino / 4,
                  rk_mix(c[RK_KIP_LENGUA], c[RK_KIP_TINTA], 110), 200);
        }
        for (i = 0; i < n; i++) {
            pt_t q = bez2(g.izq, g.sup, g.der, (int32_t)(i * 1024 / (n - 1)));
            hueco[2 * nh] = q.x;
            hueco[2 * nh + 1] = q.y;
            nh++;
        }
        for (i = 1; i < n - 1; i++) {
            pt_t q = inf_redonda(&g, (int32_t)(i * 1024 / (n - 1)));
            hueco[2 * nh] = q.x;
            hueco[2 * nh + 1] = q.y;
            nh++;
        }
        {
            rk_relleno_t r = rk_plano(c[RK_KIP_TINTA]);
            rk_aa_poligono(l->fb, hueco, nh, &r, 255, NULL, 0);
        }
        if (g.ab > MIL(l, 40)) {
            /* Los dientes de arriba, pegados al labio. */
            rk_relleno_t r = rk_plano(c[RK_KIP_BLANCO]);
            rk_forma_t f = rk_elipse_q4(g.medio.x, g.medio.y - g.ab / 2 - g.ab / 12,
                                        g.W * 70 / 100, g.ab * 30 / 100);
            rk_aa_poligono(l->fb, hueco, nh, &r, 255, &f, 1);
            /* Y la lengua, redonda, abajo. */
            r = rk_plano(c[RK_KIP_LENGUA]);
            f = rk_elipse_q4(g.medio.x - g.W / 10, g.medio.y + g.ab * 42 / 100,
                             g.W * 62 / 100, g.ab * 34 / 100 + g.ab * b->lengua / 4000);
            rk_aa_poligono(l->fb, hueco, nh, &r, 255, &f, 1);
            trazo(l, punto(g.medio.x - g.W / 10, g.medio.y + g.ab * 20 / 100),
                  punto(g.medio.x - g.W / 12, g.medio.y + g.ab * 35 / 100),
                  punto(g.medio.x - g.W / 14, g.medio.y + g.ab * 48 / 100),
                  fino / 3, fino * 60 / 100, fino / 4,
                  rk_mix(c[RK_KIP_LENGUA], c[RK_KIP_TINTA], 110), 180);
        }
        trazo(l, g.izq, g.sup, g.der, fino, fino * 120 / 100, fino, c[RK_KIP_TINTA], 255);
        if (b->comisura > 0) {
            /* Los cachetes del jadeo: un paréntesis a cada lado y una
             * rayita abajo. */
            uint8_t a = (uint8_t)(b->comisura * 255 / 1000);
            int32_t h = g.ab * 60 / 100 + MIL(l, 20);
            trazo(l, punto(g.izq.x - MIL(l, 16), g.izq.y - MIL(l, 6)),
                  punto(g.izq.x - MIL(l, 40), g.izq.y + h / 2),
                  punto(g.izq.x - MIL(l, 12), g.izq.y + h), fino / 4, fino, fino / 4,
                  c[RK_KIP_TINTA], a);
            trazo(l, punto(g.der.x + MIL(l, 16), g.der.y - MIL(l, 6)),
                  punto(g.der.x + MIL(l, 40), g.der.y + h / 2),
                  punto(g.der.x + MIL(l, 12), g.der.y + h), fino / 4, fino, fino / 4,
                  c[RK_KIP_TINTA], a);
            trazo(l, punto(g.medio.x - g.W * 35 / 100, g.medio.y + g.ab / 2 + MIL(l, 30)),
                  punto(g.medio.x, g.medio.y + g.ab / 2 + MIL(l, 42)),
                  punto(g.medio.x + g.W * 35 / 100, g.medio.y + g.ab / 2 + MIL(l, 30)),
                  fino / 4, fino, fino / 4, c[RK_KIP_TINTA], a);
        }
        break;
    }

    case RK_KIP_BOCA_SUMERGIDA: {
        /* Bajo el agua: labios teal gruesos, lila adentro, torcida. */
        if (g.ab < MIL(l, 20)) {
            trazo(l, g.izq, punto(g.medio.x, g.medio.y + MIL(l, 16)), g.der,
                  ancho / 3, ancho * 90 / 100, ancho / 3, c[RK_KIP_HONDA], 255);
        } else {
            int n = 16;
            boca_geo_t d = g;
            poli_ini();
            for (i = 0; i < n; i++) {
                poli_pt(bez2(g.izq, g.sup, g.der, (int32_t)(i * 1024 / (n - 1))));
            }
            for (i = 1; i < n - 1; i++) {
                poli_pt(inf_redonda(&g, (int32_t)(i * 1024 / (n - 1))));
            }
            poli_pintar(l, c[RK_KIP_HONDA], 255);
            /* Adentro: el labio de arriba es el grueso, y el hueco cae a la
             * izquierda. */
            d.W = g.W * 76 / 100;
            d.ab = g.ab * 52 / 100;
            d.medio = punto(g.medio.x - g.W * 9 / 100, g.medio.y + g.ab * 14 / 100);
            d.izq = punto(d.medio.x - d.W, g.izq.y + g.ab * 12 / 100);
            d.der = punto(d.medio.x + d.W, g.der.y + g.ab * 22 / 100);
            d.sup = control_por(d.izq, punto(d.medio.x, d.medio.y - d.ab / 2), d.der);
            poli_ini();
            for (i = 0; i < n; i++) {
                poli_pt(bez2(d.izq, d.sup, d.der, (int32_t)(i * 1024 / (n - 1))));
            }
            for (i = 1; i < n - 1; i++) {
                poli_pt(inf_redonda(&d, (int32_t)(i * 1024 / (n - 1))));
            }
            poli_pintar(l, c[RK_KIP_LILA], 255);
        }
        if (b->labio > 0) {
            int32_t y = g.medio.y + g.ab / 2 + MIL(l, 30);
            trazo(l, punto(g.medio.x - g.W * 40 / 100, y), punto(g.medio.x - g.W / 10, y + MIL(l, 8)),
                  punto(g.medio.x + g.W * 15 / 100, y - MIL(l, 2)), fino / 4, fino, fino / 4,
                  c[RK_KIP_HONDA], (uint8_t)(b->labio * 255 / 1000));
        }
        return;
    }

    case RK_KIP_BOCA_O: {
        /* Soplando: un aro de tinta, más grueso a la izquierda, y adentro
         * el hueco oscuro corrido hacia ese lado, como en "calor 7". */
        rk_forma_t f = rk_elipse_q4(g.medio.x, g.medio.y, g.W, g.ab / 2 + fino);
        rk_aa_pintar(l->fb, &f, 1, c[RK_KIP_TINTA], 255, 0, 0, l->fb->w, l->fb->h);
        if (g.ab > fino * 2) {
            f = rk_elipse_q4(g.medio.x + fino / 2, g.medio.y,
                             g.W - fino * 12 / 10, g.ab / 2 - fino / 3);
            rk_aa_pintar(l->fb, &f, 1, rk_mix(c[RK_KIP_CUERPO], c[RK_KIP_OJERA], 90), 255,
                         0, 0, l->fb->w, l->fb->h);
            f = rk_elipse_q4(g.medio.x - g.W * 22 / 100, g.medio.y + g.ab / 12,
                             g.W * 36 / 100, g.ab * 26 / 100);
            rk_aa_pintar(l->fb, &f, 1, c[RK_KIP_TINTA], 255, 0, 0, l->fb->w, l->fb->h);
        }
        return;
    }

    case RK_KIP_BOCA_BESO: {
        /* El "3": dos lomos hacia afuera y el fruncido a la izquierda. */
        int32_t x0 = g.medio.x - g.W * 25 / 100, h = g.ab / 2;
        rk_forma_t f;
        trazo(l, punto(x0, g.medio.y - h), punto(g.medio.x + g.W * 150 / 100, g.medio.y - h * 110 / 100),
              punto(x0 + g.W / 8, g.medio.y), fino, ancho, fino * 80 / 100, c[RK_KIP_TINTA], 255);
        trazo(l, punto(x0 + g.W / 8, g.medio.y), punto(g.medio.x + g.W * 150 / 100, g.medio.y + h * 110 / 100),
              punto(x0, g.medio.y + h), fino * 80 / 100, ancho, fino, c[RK_KIP_TINTA], 255);
        f = rk_elipse_q4(x0 - g.W * 10 / 100, g.medio.y, g.W * 30 / 100, h * 34 / 100 + fino / 2);
        rk_aa_pintar(l->fb, &f, 1, c[RK_KIP_TINTA], 255, 0, 0, l->fb->w, l->fb->h);
        return;
    }

    case RK_KIP_BOCA_ONDA: {
        pt_t q[KIP_MUESTRAS];
        int32_t w[KIP_MUESTRAS];
        int n = 18;
        for (i = 0; i < n; i++) {
            int32_t x = -1000 + i * 2000 / (n - 1);          /* milésimas de W */
            int32_t arco = C * (1000 - x * x / 1000) / 1000;
            int32_t onda = g.ab / 2 * rk_sin8((uint8_t)((x + 1000) * 192 / 1000)) / 127;
            q[i] = punto(g.medio.x + g.W * x / 1000, g.medio.y - arco + onda);
            w[i] = perfil(fino / 3, ancho * 80 / 100, fino / 3, (int32_t)(i * 1024 / (n - 1)));
        }
        pincel(l, q, w, n, c[RK_KIP_TINTA], 255);
        break;
    }

    default:
        /* La línea: sonrisa, recta o mueca, más gruesa en el medio. */
        trazo(l, g.izq, control_por(g.izq, g.medio, g.der), g.der, ancho * 35 / 100, ancho,
              ancho * 35 / 100, c[RK_KIP_TINTA], 255);
        break;
    }

    /* Los ganchitos de las comisuras: un paréntesis que cruza cada punta. */
    if (b->comisura > 0) {
        uint8_t a = (uint8_t)(b->comisura * 255 / 1000);
        int32_t h = g.W * 12 / 100 > MIL(l, 20) ? g.W * 12 / 100 : MIL(l, 20);
        trazo(l, punto(g.izq.x + h / 4, g.izq.y - h), punto(g.izq.x - h * 70 / 100, g.izq.y),
              punto(g.izq.x + h / 4, g.izq.y + h), fino / 4, fino, fino / 4, c[RK_KIP_TINTA], a);
        trazo(l, punto(g.der.x - h / 4, g.der.y - h), punto(g.der.x + h * 70 / 100, g.der.y),
              punto(g.der.x - h / 4, g.der.y + h), fino / 4, fino, fino / 4, c[RK_KIP_TINTA], a);
    }
    /* Y la rayita del labio de abajo. */
    if (b->labio > 0) {
        int32_t y = g.medio.y + g.ab / 2 + MIL(l, 36);
        int32_t x = g.medio.x + g.W / 20;
        trazo(l, punto(x - g.W * 22 / 100, y), punto(x, y + MIL(l, 8)),
              punto(x + g.W * 22 / 100, y), fino / 4, fino, fino / 4, c[RK_KIP_TINTA],
              (uint8_t)(b->labio * 255 / 1000));
    }
}

/* =========================================================== efectos === */
/* La gota: un círculo abajo y una punta arriba, con su contorno y su brillo
 * como la dibuja Rocío. `r` en Q4. */
static void lagrima(lienzo_t *l, pt_t c, int32_t r, rk_color_t relleno, rk_color_t borde,
                    uint8_t alfa, bool hueca)
{
    int capa, j;

    if (r < 8) {
        return;
    }
    for (capa = 0; capa < 2; capa++) {
        int32_t rr = capa == 0 ? r + MIL(l, 7) : r;
        poli_ini();
        poli_pt(punto(c.x, c.y - rr * 27 / 10));
        for (j = 0; j <= 12; j++) {
            /* de -25 a 205 grados, por abajo */
            uint8_t a = (uint8_t)(-18 + j * 164 / 12);
            poli_pt(punto(c.x + rk_sin8((uint8_t)(a + 64)) * rr / 127,
                          c.y + rk_sin8(a) * rr / 127));
        }
        poli_pintar(l, capa == 0 ? borde : (hueca ? l->c[RK_KIP_CUERPO] : relleno), alfa);
    }
    if (!hueca) {
        rk_forma_t f = rk_elipse_q4(c.x - r * 38 / 100, c.y - r * 10 / 100, r * 20 / 100, r * 42 / 100);
        rk_aa_pintar(l->fb, &f, 1, l->c[RK_KIP_NIEVE], (uint8_t)(alfa * 220 / 255), 0, 0,
                     l->fb->w, l->fb->h);
    }
}

/* SE AHOGA: el agua le llega a los ojos. Gris y opaca como en la lámina,
 * con la superficie que ondula, y abajo de todo la franja honda teal con
 * sus ondas claras que pasan. Con `k` a medias el agua está subiendo (o
 * escurriéndose): la transición se ve. */
static void agua(lienzo_t *l, int32_t k)
{
    int32_t alto = (int32_t)l->fb->h * 16, ancho = (int32_t)l->fb->w * 16;
    /* El nivel queda justo abajo de los ojos, tapando la línea del
     * párpado de abajo, como en la lámina. */
    int32_t nivel = alto + (l->cy + MIL(l, 5) - alto) * k / 1000;
    int32_t hondo = alto - MIL(l, 125) * k / 1000;
    int32_t fino = MIL(l, 7);
    uint8_t fase = (uint8_t)(l->t / 12u);
    int32_t la = l->u * 55 / 100 + 1, lb = l->u * 23 / 100 + 1;
    int i, capa, n = 24;

    for (capa = 0; capa < 2; capa++) {
        int32_t y = capa == 0 ? nivel : hondo;
        int32_t amp = MIL(l, capa == 0 ? 9 : 5);
        poli_ini();
        for (i = 0; i < n; i++) {
            int32_t x = -16 + (ancho + 32) * i / (n - 1);
            int32_t px = x / 16;
            int32_t ola = rk_sin8((uint8_t)(px * 256 / la + fase * (capa + 1))) * amp / 127
                        + rk_sin8((uint8_t)(px * 256 / lb - fase * 2)) * amp * 4 / 9 / 127;
            poli_pt(punto(x, y + ola));
        }
        poli_pt(punto(ancho + 16, alto + 16));
        poli_pt(punto(-16, alto + 16));
        poli_pintar(l, l->c[capa == 0 ? RK_KIP_AGUA : RK_KIP_HONDA], 250);
    }
    /* Las ondas claras del agua honda, que pasan despacio. */
    for (i = 0; i < 3; i++) {
        int32_t vuelta = l->fb->w + 40;
        int32_t x = ((int32_t)((l->t / (6u + (uint32_t)i * 2u) + (uint32_t)i * 311u)
                               % (uint32_t)vuelta) - 20) * 16;
        int32_t y = hondo + MIL(l, 24 + i * 30);
        int32_t w = MIL(l, 60 + i * 15);
        trazo(l, punto(x, y), punto(x + w / 2, y - MIL(l, 7)), punto(x + w, y),
              fino / 4, fino, fino / 4, l->c[RK_KIP_ESPUMA], (uint8_t)(220 * k / 1000));
    }
}

/* AIRE SECO: las grietas, calcadas de la lámina: una en la frente con su
 * rama, y la grande abajo a la derecha. Crecen con `k`. */
static const int16_t GRIETA_A[]  = { -282, -408, -232, -370, -201, -351, -88, -351 };
static const int16_t GRIETA_A2[] = { -201, -351, -176, -307 };
static const int16_t GRIETA_B[]  = { -60, 297, 212, 278, 275, 325, 290, 380, 478, 411 };
static const int16_t GRIETA_B2[] = { -77, 376, -7, 372, 48, 344, 251, 317, 302, 290, 330, 259 };

static void grietas(lienzo_t *l, int32_t k)
{
    int32_t g = MIL(l, 8);
    rk_color_t t = l->c[RK_KIP_TINTA];

    quebrada(l, GRIETA_A, 4, g, k, 0, 0, 0, t, 255);
    quebrada(l, GRIETA_B, 5, g, k, 0, 0, 0, t, 255);
    quebrada(l, GRIETA_B2, 6, g, k, 0, 0, 0, t, 255);
    if (k > 500) {
        quebrada(l, GRIETA_A2, 2, g, (k - 500) * 2, 0, 0, 0, t, 255);
    }
}

/* CALOR, lo que va detrás de la cara: el brillo de la piel sudada a la
 * izquierda y las dos franjas del reflejo arriba a la derecha. */
static void calor_fondo(lienzo_t *l, int32_t k)
{
    uint8_t pulso = (uint8_t)(140 + rk_sin8((uint8_t)(l->t / 16u)) * 25 / 127);
    rk_forma_t f;

    trazo(l, en(l, -470, -185), en(l, -452, -290), en(l, -330, -305),
          MIL(l, 26), MIL(l, 16), MIL(l, 2), l->c[RK_KIP_LUZ], (uint8_t)(150 * k / 1000));
    f = rk_capsula_q4(l->cx + MIL(l, 192), l->cy - MIL(l, 450), l->cx + MIL(l, 192),
                      l->cy - MIL(l, 335), MIL(l, 17));
    rk_aa_pintar(l->fb, &f, 1, l->c[RK_KIP_REFLEJO], (uint8_t)(pulso * k / 1000), 0, 0,
                 l->fb->w, l->fb->h);
    f = rk_capsula_q4(l->cx + MIL(l, 286), l->cy - MIL(l, 450), l->cx + MIL(l, 286),
                      l->cy - MIL(l, 300), MIL(l, 30));
    rk_aa_pintar(l->fb, &f, 1, l->c[RK_KIP_REFLEJO], (uint8_t)(pulso * k / 1000), 0, 0,
                 l->fb->w, l->fb->h);
}

/* CALOR, lo que va encima: el garabato en zigzag de la lámina, que hierve,
 * y la gota que baja por el cachete y se cae. */
static const int16_t ZIGZAG[] = {
    -205, -455, -320, -445, -345, -425, -310, -408, -255, -405, -250, -385, -300, -355, -318, -332
};

static void calor(lienzo_t *l, int32_t k)
{
    uint32_t f = l->t % 2700u;
    int32_t crece = f < 300u ? (int32_t)f * 1000 / 300 : 1000;
    int32_t baja = f < 300u ? 0 : suave((int32_t)(f - 300u) * 1000 / 1900);
    int32_t cae = f > 2200u ? (int32_t)(f - 2200u) : 0;
    uint8_t alfa = (uint8_t)((f > 2200u ? (2700u - f) * 230u / 500u : 230u) * (uint32_t)k / 1000u);
    pt_t c = en(l, -425 + 25 * baja / 1000, -110 + 420 * baja / 1000 + cae * cae / 900);

    quebrada(l, ZIGZAG, 8, MIL(l, 10), k, 0, 0, 2, l->c[RK_KIP_TINTA], 255);
    lagrima(l, c, MIL(l, 27) * crece / 1000, l->c[RK_KIP_GOTA],
            rk_mix(l->c[RK_KIP_GOTA], l->c[RK_KIP_TINTA], 110), alfa, false);
}

/* SOL DIRECTO: un resplandor que entra por arriba a la derecha, con rayos
 * que giran despacio, y las franjas del reflejo. */
static void sol(lienzo_t *l, int32_t k)
{
    pt_t c = en(l, 470, -470);
    rk_forma_t f = rk_circulo_q4(c.x, c.y, MIL(l, 330));
    /* Un degradado que termina en el color del cuerpo: el resplandor no
     * tiene borde, se funde con la piel. */
    rk_relleno_t r = rk_radial(c.x, c.y, MIL(l, 330),
                               rk_mix(l->c[RK_KIP_CUERPO], l->c[RK_KIP_LUZ], (uint8_t)(200 * k / 1000)),
                               l->c[RK_KIP_CUERPO]);
    int i;

    rk_aa_pintar_relleno(l->fb, &f, 1, &r, 255, 0, 0, l->fb->w, l->fb->h);
    for (i = 0; i < 5; i++) {
        uint8_t a = (uint8_t)(64 + 12 + i * 22 + (int)(l->t / 90u % 22u));
        int32_t cs = rk_sin8((uint8_t)(a + 64)), sn = rk_sin8(a);
        pt_t p0 = punto(c.x - cs * MIL(l, 290) / 127, c.y + sn * MIL(l, 290) / 127);
        pt_t p1 = punto(c.x - cs * MIL(l, 400) / 127, c.y + sn * MIL(l, 400) / 127);
        trazo(l, p0, punto((p0.x + p1.x) / 2, (p0.y + p1.y) / 2), p1, MIL(l, 12), MIL(l, 9),
              MIL(l, 2), l->c[RK_KIP_LUZ], (uint8_t)(170 * k / 1000));
    }
    calor_fondo(l, k * 70 / 100);
}

static void zzz(lienzo_t *l, int32_t k)
{
    int i;

    for (i = 0; i < 3; i++) {
        uint32_t ph = (l->t / 11u + (uint32_t)i * 110u) % 330u;
        int16_t s = (int16_t)(22 + ph / 12u);
        int16_t z[8];
        uint8_t alfa = (uint8_t)((ph > 250u ? (330u - ph) * 3u : 240u) * (uint32_t)k / 1000u);
        if (ph > 320u) {
            continue;
        }
        z[0] = (int16_t)-s; z[1] = (int16_t)-s; z[2] = s; z[3] = (int16_t)-s;
        z[4] = (int16_t)-s; z[5] = s;           z[6] = s; z[7] = s;
        quebrada(l, z, 4, MIL(l, 9), 1000, 250 + (int32_t)ph * 3 / 4, -180 - (int32_t)ph,
                 1, l->c[RK_KIP_TINTA], alfa);
    }
}

static void nieve(lienzo_t *l, int32_t k)
{
    int i, j;

    for (i = 0; i < 6; i++) {
        uint16_t h = rk_hash((uint16_t)(i * 1597 + 11));
        int32_t baja = (int32_t)((l->t / 26u + (h >> 6)) % 256u);
        int32_t x = (int32_t)(h % (uint16_t)l->fb->w) * 16
                  + rk_sin8((uint8_t)(l->t / 18u + (uint32_t)i * 51u)) * MIL(l, 30) / 127;
        int32_t y = baja * (int32_t)l->fb->h * 16 / 256;
        int32_t r = MIL(l, 16 + (i % 2) * 8);
        for (j = 0; j < 3; j++) {
            int32_t a = rk_sin8((uint8_t)(j * 43 + 64)) * r / 127;
            int32_t b = rk_sin8((uint8_t)(j * 43)) * r / 127;
            rk_forma_t f = rk_capsula_q4(x - a, y - b, x + a, y + b, MIL(l, 5));
            rk_aa_pintar(l->fb, &f, 1, l->c[RK_KIP_NIEVE], (uint8_t)(230 * k / 1000), 0, 0,
                         l->fb->w, l->fb->h);
        }
    }
}

/* SIN DATOS: un signo de pregunta de tinta, del lado de la ceja que está
 * arriba. Cuando las cejas se dan vuelta, salta al otro lado. */
static const int16_t PREGUNTA[] = { -30, -34, -24, -70, 4, -82, 30, -62, 25, -32, 3, -14, 0, 12 };

static void duda(lienzo_t *l, int32_t k)
{
    uint32_t f = l->t % 5000u;
    int32_t lado = 1000;
    int32_t dy = rk_sin8((uint8_t)(l->t / 9u)) * 14 / 127;
    pt_t punto_q;
    rk_forma_t c;

    if (f >= 2000u && f < 2500u) {
        lado = 1000 - 2 * salto((int32_t)(f - 2000u) * 2);
    } else if (f >= 2500u && f < 4500u) {
        lado = -1000;
    } else if (f >= 4500u) {
        lado = -1000 + 2 * salto((int32_t)(f - 4500u) * 2);
    }
    /* Arriba, por encima de la ceja que pregunta: más abajo se pisarían. */
    quebrada(l, PREGUNTA, 7, MIL(l, 13) * k / 1000, 1000, 385 * lado / 1000, -400 + dy, 1,
             l->c[RK_KIP_TINTA], 255);
    punto_q = en(l, 385 * lado / 1000, -365 + dy);
    c = rk_circulo_q4(punto_q.x, punto_q.y, MIL(l, 12) * k / 1000);
    rk_aa_pintar(l->fb, &c, 1, l->c[RK_KIP_TINTA], 255, 0, 0, l->fb->w, l->fb->h);
}

/* DESCONECTADO: tres puntitos que aparecen de a uno, como quien espera. */
static void espera(lienzo_t *l, int32_t k)
{
    uint32_t f = l->t % 1800u;
    int i;

    for (i = 0; i < 3; i++) {
        uint32_t t0 = (uint32_t)i * 300u;
        int32_t r;
        pt_t c;
        rk_forma_t d;
        if (f < t0 || f >= 1500u) {
            continue;
        }
        r = f - t0 < 150u ? salto((int32_t)(f - t0) * 1000 / 150) : 1000;
        c = en(l, 250 + i * 80, -395);
        d = rk_circulo_q4(c.x, c.y, MIL(l, 20) * r / 1000 * k / 1000);
        rk_aa_pintar(l->fb, &d, 1, l->c[RK_KIP_TINTA], 255, 0, 0, l->fb->w, l->fb->h);
    }
}

/* SED: una gota vacía, de puro contorno, que se hamaca arriba a la derecha:
 * lo que le falta. */
static void sed(lienzo_t *l, int32_t k)
{
    int32_t dy = rk_sin8((uint8_t)(l->t / 12u)) * 18 / 127;
    lagrima(l, en(l, 355, -330 + dy), MIL(l, 36) * k / 1000, l->c[RK_KIP_CUERPO],
            l->c[RK_KIP_TINTA], 255, true);
}

/* Las rayitas entre las cejas, del fastidio y del esfuerzo. */
static void ceno(lienzo_t *l, const rk_kip_pose_t *pose, int32_t k)
{
    const rk_persona_t *p = l->p;
    int32_t dy = (pose->ceja[0].dy + pose->ceja[1].dy) / 2;
    int32_t y = p->ojo_dy * 10 - p->ceja_alto * 10 + dy;
    int32_t fino = MIL(l, 6);
    uint8_t a = (uint8_t)(230 * k / 1000);
    int lado;

    for (lado = -1; lado <= 1; lado += 2) {
        trazo(l, en(l, lado * 36, y - 34), en(l, lado * 52, y - 2), en(l, lado * 38, y + 32),
              fino / 4, fino, fino / 4, l->c[RK_KIP_TINTA], a);
    }
}

/* ========================================================== el cuadro === */
void rk_kip_dibujar(rk_fb_t *fb, const rk_persona_t *p, const rk_kip_colores_t *col,
                    const rk_kip_pose_t *pose, int32_t cx, int32_t cy, int32_t u,
                    uint32_t t_ms)
{
    lienzo_t l;
    const int16_t *fx;

    if (fb == NULL || fb->px == NULL || p == NULL || col == NULL || pose == NULL || u <= 0) {
        return;
    }
    l.fb = fb;
    l.p = p;
    l.c = col->c;
    l.u = u;
    l.cx = cx;
    l.cy = cy;
    l.t = t_ms;
    l.cy += MIL(&l, pose->dy);
    fx = pose->fx;

    /* Lo que va sobre la piel, detrás de los rasgos. */
    if (fx[RK_KIP_FX_SOL] > 0) {
        sol(&l, fx[RK_KIP_FX_SOL]);
    }
    if (fx[RK_KIP_FX_CALOR] > 0) {
        calor_fondo(&l, fx[RK_KIP_FX_CALOR]);
    }

    ojo(&l, &pose->ojo[0], -1, pose->mira_x, pose->mira_y);
    ojo(&l, &pose->ojo[1], +1, pose->mira_x, pose->mira_y);
    ceja(&l, &pose->ceja[0], -1);
    ceja(&l, &pose->ceja[1], +1);
    if (fx[RK_KIP_FX_CENO] > 0) {
        ceno(&l, pose, fx[RK_KIP_FX_CENO]);
    }

    /* El agua va entre los ojos y la boca: tapa el borde de abajo de los
     * ojos, y la boca se dibuja encima, nítida, como en la lámina. */
    if (fx[RK_KIP_FX_AGUA] > 0) {
        agua(&l, fx[RK_KIP_FX_AGUA]);
    }
    boca(&l, &pose->boca);

    if (fx[RK_KIP_FX_GRIETAS] > 0) {
        grietas(&l, fx[RK_KIP_FX_GRIETAS]);
    }
    if (fx[RK_KIP_FX_SUDOR] > 0 || fx[RK_KIP_FX_CALOR] > 0) {
        calor(&l, fx[RK_KIP_FX_CALOR] > fx[RK_KIP_FX_SUDOR] ? fx[RK_KIP_FX_CALOR]
                                                           : fx[RK_KIP_FX_SUDOR]);
    }
    if (fx[RK_KIP_FX_ZZZ] > 0) {
        zzz(&l, fx[RK_KIP_FX_ZZZ]);
    }
    if (fx[RK_KIP_FX_NIEVE] > 0) {
        nieve(&l, fx[RK_KIP_FX_NIEVE]);
    }
    if (fx[RK_KIP_FX_DUDA] > 0) {
        duda(&l, fx[RK_KIP_FX_DUDA]);
    }
    if (fx[RK_KIP_FX_ESPERA] > 0) {
        espera(&l, fx[RK_KIP_FX_ESPERA]);
    }
    if (fx[RK_KIP_FX_SED] > 0) {
        sed(&l, fx[RK_KIP_FX_SED]);
    }
}
