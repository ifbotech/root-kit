#include "face.h"
#include "look.h"
#include "../gfx/aa.h"
#include <stddef.h>
#include <string.h>

/* ----------------------------------------------------------- unidades --- */
/* Todo se mide en centésimas del lado corto del panel (U) y se lleva a Q4.
 * Es la única conversión del archivo. */
#define PQ(c, pct)   ((int32_t)((int64_t)(c)->u * 16 * (pct) / 100))
/* Un "pixel de cara": U/128 pixeles reales. En el panel de 128 es 1; en el de
 * 240 es casi 2. Sirve para grosores que tienen que escalar con la cara. */
#define PX(c, n)     ((int32_t)((int64_t)(c)->u * 16 * (n) / 128))

#define COL_BLANCO  RK_RGB(255, 255, 255)
#define COL_LENGUA  RK_RGB(255, 112, 142)
#define COL_GOTA    RK_RGB(100, 190, 250)
#define COL_ORO     RK_RGB(255, 196,  48)
#define COL_ORO_OSC RK_RGB(206, 140,  18)

typedef struct {
    rk_fb_t            *fb;
    const rk_persona_t *p;
    int32_t  u;             /* lado de la cara en pixeles                  */
    int32_t  cx, cy;        /* centro de la cara, Q4                       */
    int32_t  oy;            /* altura de los ojos, Q4                      */
    int32_t  by;            /* altura de la boca, Q4                       */
    uint32_t t;
    /* Los colores de este cuadro, ya derivados de la piel y tratados con
     * el tinte y la penumbra del ánimo. */
    rk_color_t bg;          /* el fondo, y los párpados                    */
    rk_color_t trazo;       /* ojos, boca, cejas                           */
    rk_color_t blanco;      /* brillos, dientito, el blanco del susto      */
    rk_color_t iris;        /* la media luna clara de abajo del ojo        */
    rk_color_t rubor;       /* mejillas                                    */
    rk_color_t acento;      /* destellos, aura, luces                      */
    rk_color_t lengua;
    rk_color_t pecas;
} cara_t;

/* La expresión concreta de un cuadro: lo que el ánimo pidió, pasado por la
 * familia de ojos del personaje y por el parpadeo del instante.
 *
 * En dos partes: la geometría (rk_face_geom_t), que se puede interpolar
 * entre dos ánimos, y lo discreto, que no. */
typedef struct {
    rk_face_geom_t g;
    int  cerrado;     /* 0 abierto, 1 cerrado contento (^), 2 dormido (u)  */
    int  guino;       /* -1 o 1: ese ojo se abre aunque estén en ^ ^       */
    bool cruz;
    bool espiral;
    int  boca;        /* rk_boca_t: la del ánimo; la curva va en g         */
    bool gota;
    bool zzz;
    bool burbujas;
    bool nieve;       /* copos que caen: frio                              */
    bool vaho;        /* vapor que sube: calor                             */
    bool polvillo;    /* motas que flotan: aire seco                       */
    bool agua;        /* el visor inundado hasta la mitad: se ahoga        */
    bool grietas;     /* la cara cuarteada: sequía                         */
    bool arrugas;     /* tensión bajo el párpado: el ojo apretado          */
    bool frijol;      /* la pupila deja de ser un óvalo y se deforma       */
} expr_t;

/* ---------------------------------------------------------- geometría --- */
static int lerp_i(int a, int b, int t)
{
    return a + (b - a) * t / 100;
}

rk_face_geom_t rk_face_geom_lerp(const rk_face_geom_t *a,
                                 const rk_face_geom_t *b, uint8_t t_pct)
{
    rk_face_geom_t g;
    int t = t_pct > 100u ? 100 : (int)t_pct;

    g.abre       = lerp_i(a->abre,       b->abre,       t);
    g.pupila     = lerp_i(a->pupila,     b->pupila,     t);
    g.tapa_sup   = lerp_i(a->tapa_sup,   b->tapa_sup,   t);
    g.tapa_ang   = lerp_i(a->tapa_ang,   b->tapa_ang,   t);
    g.tapa_inf   = lerp_i(a->tapa_inf,   b->tapa_inf,   t);
    g.mira_x     = lerp_i(a->mira_x,     b->mira_x,     t);
    g.mira_y     = lerp_i(a->mira_y,     b->mira_y,     t);
    g.ceja_ang   = lerp_i(a->ceja_ang,   b->ceja_ang,   t);
    g.ceja_dy    = lerp_i(a->ceja_dy,    b->ceja_dy,    t);
    g.boca_curva = lerp_i(a->boca_curva, b->boca_curva, t);
    return g;
}

/* smoothstep: 3t^2 - 2t^3, con t en centésimas. */
uint8_t rk_face_ease(uint8_t t_pct)
{
    int32_t t = t_pct > 100u ? 100 : (int32_t)t_pct;
    int32_t t2 = t * t;
    return (uint8_t)((3 * t2 * 100 - 2 * t2 * t) / 10000);
}

/* El celeste del agua cuando el visor se inunda, y los tres colores de
 * los acabados épicos. */
#define COL_AGUA  RK_HEX(0x6EC6FF)
#define COL_HIELO RK_HEX(0xCFF3FF)
#define COL_FUEGO RK_HEX(0xFF7A1A)

/* -------------------------------------------------------------- color --- */
/* Tinte y penumbra se aplican a TODOS los colores por igual. Si se aplicaran
 * sólo al fondo, los párpados —que se pintan del color del fondo— dejarían de
 * coincidir con él y se verían como parches. */
static rk_color_t tratar(rk_color_t base, const rk_look_t *lk)
{
    rk_color_t c = base;
    if (lk->tint_amt > 0u) {
        c = rk_mix(c, lk->tint, (uint8_t)(lk->tint_amt / 3));
    }
    if (lk->dim > 0u) {
        c = rk_dim(c, (uint8_t)(lk->dim * 2 / 3));
    }
    return c;
}

/* Los colores de un cuadro salen de los cuatro de la piel. Los derivados
 * (la media luna del ojo, los destellos, la lengua) se mezclan en vez de
 * escribirse, así una piel nueva sólo tiene que elegir cuatro colores. */
static void pintura(cara_t *c, const rk_piel_t *pl, const rk_look_t *lk)
{
    rk_color_t fondo = tratar(pl->fondo, lk);
    rk_color_t ojos  = tratar(pl->ojos, lk);
    rk_color_t piel  = tratar(pl->piel, lk);
    rk_color_t rubor = tratar(pl->rubor, lk);

    c->bg     = fondo;
    c->trazo  = ojos;
    c->blanco = rk_mix(tratar(COL_BLANCO, lk), fondo, 20);
    c->iris   = rk_mix(ojos, piel, 105);
    c->rubor  = rubor;
    c->acento = rk_mix(rubor, ojos, 70);
    c->lengua = rk_mix(tratar(COL_LENGUA, lk), rubor, 80);
    c->pecas  = rk_mix(rubor, ojos, 120);
}

/* dst = a mezclado hacia b en `t` (0 = a, 255 = b), color por color. */
static void fundir(cara_t *dst, const cara_t *a, const cara_t *b, uint8_t t)
{
    cara_t r = *dst;
    r.bg     = rk_mix(a->bg,     b->bg,     t);
    r.trazo  = rk_mix(a->trazo,  b->trazo,  t);
    r.blanco = rk_mix(a->blanco, b->blanco, t);
    r.iris   = rk_mix(a->iris,   b->iris,   t);
    r.rubor  = rk_mix(a->rubor,  b->rubor,  t);
    r.acento = rk_mix(a->acento, b->acento, t);
    r.lengua = rk_mix(a->lengua, b->lengua, t);
    r.pecas  = rk_mix(a->pecas,  b->pecas,  t);
    *dst = r;
}

rk_color_t rk_face_fondo(const rk_persona_t *p, uint8_t rareza, rk_mood_t mood)
{
    if (p == NULL) {
        p = rk_persona_at(0);
    }
    return tratar(rk_persona_piel(p, rareza)->fondo, rk_look(mood));
}

/* ---------------------------------------------------------- expresión --- */
static expr_t expresion(const rk_persona_t *p, rk_mood_t mood,
                        const rk_look_t *lk, uint32_t t, uint8_t cierre)
{
    expr_t e;

    memset(&e, 0, sizeof e);
    e.g.abre = 100;
    e.g.pupila = 100;
    e.boca = lk->boca;
    /* La curva de la boca: sonrisa, recta o mueca. Los otros estilos (abierta,
     * jadeando, temblorosa) no tienen curva: son discretos. */
    e.g.boca_curva = lk->boca == RK_BOCA_SMILE ? 100
                   : lk->boca == RK_BOCA_FROWN ? -100 : 0;

    /* 1. Lo que pide el ánimo, en abstracto. */
    switch ((rk_ojo_t)lk->ojo) {
    case RK_OJO_BLINK:  e.cerrado = 2;                          break;
    case RK_OJO_HAPPY:  e.cerrado = 1;                          break;
    case RK_OJO_WIDE:   e.g.abre = 112; e.g.pupila = 62;        break;
    case RK_OJO_SLEEPY: e.g.tapa_sup = 44; e.g.tapa_ang = -10;  break;
    case RK_OJO_DEAD:   e.cruz = true;                          break;
    case RK_OJO_DIZZY:  e.espiral = true;                       break;
    case RK_OJO_GLITCH: e.g.tapa_sup = 30; e.g.pupila = 80;     break;
    default:                                                    break;
    }

    /* 2. Los gestos propios de cada ánimo, que no son sólo ojos. */
    switch (mood) {
    case RK_MOOD_THIRSTY:
        e.gota = true; e.g.mira_y = 45; e.g.ceja_ang = 10; break;
    case RK_MOOD_HOT:
        e.gota = true; e.vaho = true; e.g.ceja_ang = 8; break;
    case RK_MOOD_COLD:
        e.nieve = true;
        e.g.ceja_ang = 12; e.g.ceja_dy = 3; e.g.tapa_inf = 22; break;
    case RK_MOOD_DROWNING:
        /* No es sólo cara de susto: el visor se le llena de agua hasta la
           mitad, y eso se entiende de un vistazo desde el otro lado del
           cuarto, que es para lo que existe la cara.
           El acting es de caricatura tradicional, no de emoji: las cejas se
           levantan y se juntan en el medio de la frente, el párpado de abajo
           sube apretando el ojo (con su arruga de tensión), y la pupila se
           achica y mira ARRIBA, a la cámara de aire que queda en el techo
           de la escafandra. */
        e.burbujas = true; e.agua = true; e.arrugas = true; e.frijol = true;
        /* Las cejas, cerca del ojo: altas quedan de sorpresa, no de angustia.
           El párpado de abajo sube apenas —lo suficiente para apretar el ojo
           sin cerrarlo— porque el ojo también tiene que asomar sobre el agua. */
        e.g.ceja_dy = 3; e.g.ceja_ang = 30;
        e.g.tapa_inf = 12; e.g.pupila = 46; e.g.mira_y = -78;
        break;
    case RK_MOOD_DARK:
        /* En penumbra no mira fijo: barre despacio de un lado al otro,
           buscando de donde puede venir la luz. */
        e.g.ceja_dy = 4; e.g.ceja_ang = 8;
        e.g.mira_x = 55 * (int)rk_sin8((uint8_t)(t / 26u)) / 127;
        break;
    case RK_MOOD_SLEEPING:
        e.zzz = true;                                  break;
    case RK_MOOD_UNKNOWN:
        e.g.mira_y = -60; e.g.mira_x = -30; e.g.ceja_dy = 4; break;
    case RK_MOOD_PARCHED_AIR:
        e.grietas = true;
        e.polvillo = true;
        e.g.tapa_inf = 30; e.g.tapa_sup = 20;              break;
    case RK_MOOD_SCORCHED:
        e.g.ceja_ang = 12;                               break;
    case RK_MOOD_HAPPY:
        /* Contento con los ojos abiertos, y cada tanto el gesto de alegría:
         * los ojos se cierran en ^ durante un segundo. Siempre en ^ sería
         * una cara sin mirada; nunca, una cara sin alegría. */
        e.g.ceja_dy = 2; e.g.ceja_ang = 4;
        if (t % 9000u >= 6000u && t % 9000u < 7100u && cierre == 0u) {
            e.cerrado = 1;
        }
        break;
    default:
        break;
    }

    /* 3. La familia de ojos del personaje le pone su carácter encima. */
    switch (p->familia) {
    case RK_OJOS_MEDIALUNA:
        /* Musgo: los párpados nunca suben del todo. Relajado es media luna;
         * con sueño o con frío, bajan todavía más. */
        if (e.cerrado == 0 && !e.cruz && !e.espiral) {
            int k = e.g.tapa_sup > 0 ? e.g.tapa_sup + 18 : 46;
            e.g.tapa_sup = k > 64 ? 64 : (k < 46 ? 46 : k);
            /* Con los dos párpados a la vez no queda ojo: el de abajo cede. */
            if (e.g.tapa_sup + e.g.tapa_inf > 72) {
                e.g.tapa_inf = 72 - e.g.tapa_sup;
            }
        }
        break;
    case RK_OJOS_ARCO:
        /* Pinchito: contento, los ojos quedan en ^ ^. Cada 5,2 s guiña: un
         * ojo se abre 900 ms, y la vez siguiente el otro. */
        if (mood == RK_MOOD_HAPPY && cierre == 0u) {
            uint32_t f = t % 10400u;
            e.cerrado = 1;
            if (f % 5200u >= 4000u && f % 5200u < 4900u) {
                e.guino = f < 5200u ? -1 : 1;
            }
        }
        break;
    default:
        break;
    }

    /* 4. El parpadeo, suave: el párpado baja y sube en 180 ms. */
    if (lk->blinks && e.cerrado == 0 && !e.cruz) {
        uint32_t f = t % 3400u;
        if (f < 180u) {
            int k = (int)(f < 90u ? f : 180u - f) * 100 / 90;
            if (k > e.g.tapa_sup) {
                e.g.tapa_sup = k;
            }
        }
    }

    /* 5. El cierre forzado del despertar. */
    if (cierre > 0u && e.cerrado == 0) {
        int k = e.g.tapa_sup + cierre;
        e.g.tapa_sup = k > 100 ? 100 : k;
    }
    if (e.g.tapa_sup >= 96) {
        e.cerrado = 2;
    }

    /* 6. La mirada deriva sola, despacio. Un ojo perfectamente quieto se ve
     * de muñeco; uno que se mueve un poco se ve atento. */
    if (e.cerrado == 0 && !e.cruz && !e.espiral && mood != RK_MOOD_UNKNOWN) {
        e.g.mira_x += rk_sin8((uint8_t)(t / 60u)) * 22 / 127;
        e.g.mira_y += rk_sin8((uint8_t)(t / 97u + 40u)) * 10 / 127;
    }
    return e;
}

/* La expresión del mimo: contento, con los ojos en ^ ^ y las cejas altas. */
static expr_t expresion_mimo(const rk_persona_t *p, uint32_t t)
{
    expr_t e = expresion(p, RK_MOOD_HAPPY, rk_look(RK_MOOD_HAPPY), t, 0u);

    e.cerrado = 1;
    e.guino = 0;
    e.cruz = false;
    e.espiral = false;
    e.g.tapa_sup = 0;
    e.g.tapa_inf = 0;
    e.g.ceja_dy = 6;
    e.g.ceja_ang = 6;
    e.g.boca_curva = 100;
    e.boca = RK_BOCA_SMILE;
    return e;
}

/* Un punto intermedio entre dos expresiones. La geometría se interpola; lo
 * discreto lo pone el que domina, y si difiere, el ojo parpadea justo cuando
 * cambia: el párpado baja hasta cerrarse a mitad de camino y vuelve a abrir.
 * Es lo que hace un animador para esconder un corte. */
static expr_t mezclar(const expr_t *a, const expr_t *b, uint8_t t_pct)
{
    int t = t_pct > 100u ? 100 : (int)t_pct;
    expr_t e = t >= 50 ? *b : *a;

    e.g = rk_face_geom_lerp(&a->g, &b->g, (uint8_t)t);
    if (a->cerrado != b->cerrado || a->cruz != b->cruz ||
        a->espiral != b->espiral || a->boca != b->boca) {
        int d = t < 50 ? 50 - t : t - 50;        /* distancia a la mitad */
        int k = d >= 25 ? 0 : (25 - d) * 4;      /* 0 en 25 y 75, 100 en 50 */
        if (e.cerrado == 0 && !e.cruz && k > e.g.tapa_sup) {
            e.g.tapa_sup = k;
        }
        if (e.g.tapa_sup >= 96) {
            e.cerrado = 2;
        }
    }
    return e;
}

/* ------------------------------------------------------------- piezas --- */
static void pintar(cara_t *c, const rk_forma_t *f, int n, rk_color_t col,
                   uint8_t alfa)
{
    rk_aa_pintar(c->fb, f, n, col, alfa, 0, 0, c->fb->w, c->fb->h);
}

static void pintar_en(cara_t *c, const rk_forma_t *f, int n, rk_color_t col,
                      int x0, int y0, int x1, int y1)
{
    rk_aa_pintar(c->fb, f, n, col, 255, x0, y0, x1, y1);
}

static void pintar_rel_en(cara_t *c, const rk_forma_t *f, int n,
                          const rk_relleno_t *r, int x0, int y0, int x1, int y1)
{
    rk_aa_pintar_relleno(c->fb, f, n, r, 255, x0, y0, x1, y1);
}

/* Una ALMENDRA: la intersección de dos elipses altas, una corrida hacia
 * arriba y otra hacia abajo. Es la forma de un ojo dibujado —puntas afinadas
 * a los costados, panza en el medio— y no la de un ojo de compás. */
static void almendra(rk_forma_t *f, int32_t cx, int32_t cy, int32_t rx, int32_t ry)
{
    int32_t alto = ry * 155 / 100;
    int32_t corr = ry * 55 / 100;
    f[0] = rk_elipse_q4(cx, cy + corr, rx, alto);
    f[1] = rk_elipse_q4(cx, cy - corr, rx, alto);
}

static void capsula(cara_t *c, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                    int32_t r, rk_color_t col)
{
    rk_forma_t f = rk_capsula_q4(x0, y0, x1, y1, r);
    pintar(c, &f, 1, col, 255);
}

static void circulo(cara_t *c, int32_t x, int32_t y, int32_t r, rk_color_t col,
                    uint8_t alfa)
{
    rk_forma_t f = rk_circulo_q4(x, y, r);
    pintar(c, &f, 1, col, alfa);
}

static void elipse(cara_t *c, int32_t x, int32_t y, int32_t rx, int32_t ry,
                   rk_color_t col, uint8_t alfa)
{
    rk_forma_t f = rk_elipse_q4(x, y, rx, ry);
    pintar(c, &f, 1, col, alfa);
}

/* Arco grueso con puntas redondas. `abajo` = sonrisa (u), si no, ceño (n). */
static void arco(cara_t *c, int32_t x, int32_t y, int32_t r, int32_t grosor,
                 bool abajo, rk_color_t col)
{
    rk_aa_arco(c->fb, x, y, r, grosor, abajo, col);
}

/* Lágrima: un círculo y un triángulo cuyos lados son tangentes a él. */
static void gota(cara_t *c, int32_t x, int32_t y, int32_t r, rk_color_t col)
{
    rk_aa_triangulo(c->fb, x, y - r * 2,
                    x - r * 866 / 1000, y - r / 2,
                    x + r * 866 / 1000, y - r / 2, col, 255);
    circulo(c, x, y, r, col, 255);
    circulo(c, x - r / 3, y - r / 5, r / 3, COL_BLANCO, 200);
}

/* Destello de cuatro puntas: un rombo alto y uno ancho, cruzados. */
static void destello(cara_t *c, int32_t x, int32_t y, int32_t l, rk_color_t col,
                     uint8_t alfa)
{
    rk_aa_rombo(c->fb, x, y, l / 4, l, col, alfa);
    rk_aa_rombo(c->fb, x, y, l, l / 4, col, alfa);
}

/* ---------------------------------------------------------------- ojo --- */
/* Un ojo completo. El orden importa: primero el ojo, después la media luna y
 * los brillos recortados contra él, al final los párpados, pintados del
 * color del fondo encima de todo lo anterior. */
static void ojo(cara_t *c, const expr_t *e, int32_t ex, int32_t ey,
                int32_t rx, int32_t ry, int lado)
{
    int32_t ryv = ry * e->g.abre / 100;
    int32_t grosor = PX(c, 7);
    int cerrado = e->cerrado;
    int x0, y0, x1, y1;
    rk_forma_t f[4], alm[2], esc[2];
    rk_relleno_t rel;
    int32_t linea, irx, iry, ix, iy, pupr;

    /* El guiño: con los dos en ^, uno se abre. */
    if (cerrado == 1 && e->guino == lado) {
        cerrado = 0;
    }

    /* --- cerrado: una curva, más gruesa en el medio ---------------------- */
    if (cerrado == 1) {                    /* contento: ^ */
        arco(c, ex, ey + ry / 3, rx * 3 / 4, grosor, false, c->trazo);
        arco(c, ex, ey + ry / 3, rx * 5 / 10, grosor * 128 / 100, false, c->trazo);
        return;
    }
    if (cerrado == 2) {                    /* dormido: u */
        arco(c, ex, ey - ry / 5, rx * 3 / 4, grosor, true, c->trazo);
        arco(c, ex, ey - ry / 5, rx * 5 / 10, grosor * 128 / 100, true, c->trazo);
        return;
    }
    if (e->cruz) {
        int32_t k = rx * 3 / 5;
        capsula(c, ex - k, ey - k, ex + k, ey + k, grosor / 2, c->trazo);
        capsula(c, ex + k, ey - k, ex - k, ey + k, grosor / 2, c->trazo);
        return;
    }
    if (ryv < PX(c, 3)) {
        ryv = PX(c, 3);
    }

    /* La mirada corre el ojo entero un poco, y la pupila bastante más. */
    ex += rx * e->g.mira_x / 900;
    ey += ryv * e->g.mira_y / 900;
    x0 = (int)((ex - rx) / 16) - 2;
    x1 = (int)((ex + rx) / 16) + 3;
    y0 = (int)((ey - ryv) / 16) - 2;
    y1 = (int)((ey + ryv) / 16) + 3;

    /* --- 1. el contorno ---------------------------------------------------
     * La almendra entera en color trazo, y encima la esclerótica en una
     * almendra más chica CORRIDA HACIA ABAJO. Lo que queda a la vista del
     * trazo es grueso arriba y fino abajo: una línea con peso, dibujada, en
     * vez de un contorno parejo de programa. */
    linea = PX(c, 5);
    almendra(alm, ex, ey, rx, ryv);
    pintar(c, alm, 2, c->trazo, 255);

    almendra(esc, ex, ey + linea * 62 / 100, rx - linea, ryv - linea);
    /* La esclerótica no es blanca plana: tiene la sombra del párpado arriba. */
    rel = rk_lineal(ex, ey - ryv, ex, ey + ryv,
                    rk_mix(c->blanco, c->trazo, 58), c->blanco);
    pintar_rel_en(c, esc, 2, &rel, x0, y0, x1, y1);

    /* --- 2. el iris ------------------------------------------------------- */
    irx = rx * 62 / 100;
    iry = irx;
    if (iry > ryv * 86 / 100) {
        iry = ryv * 86 / 100;
        irx = iry;
    }
    ix = ex + (rx - irx) * e->g.mira_x / 150;
    iy = ey + (ryv - iry) * e->g.mira_y / 150;
    f[0] = rk_elipse_q4(ix, iy, irx, iry);
    f[1] = esc[0];
    f[2] = esc[1];
    /* Ámbar: claro abajo y oscuro arriba, que es como se ve un iris con la
     * luz viniendo de arriba y rebotando en el párpado inferior. */
    rel = rk_lineal(ix, iy - iry, ix, iy + iry,
                    rk_mix(c->iris, c->trazo, 120), rk_mix(c->iris, c->blanco, 60));
    pintar_rel_en(c, f, 3, &rel, x0, y0, x1, y1);
    /* El limbo: el aro oscuro del borde del iris. */
    f[0] = rk_anillo_q4(ix, iy, irx, PX(c, 2));
    pintar_en(c, f, 3, rk_mix(c->trazo, c->iris, 40), x0, y0, x1, y1);

    /* --- 3. la pupila ----------------------------------------------------- */
    pupr = irx * (e->g.pupila < 100 ? (e->g.pupila < 30 ? 30 : e->g.pupila) : 62) / 100;
    if (e->espiral) {
        int32_t k;
        for (k = irx; k > PX(c, 3); k -= PX(c, 6)) {
            f[0] = rk_anillo_q4(ix, iy, k, PX(c, 3));
            pintar_en(c, f, 3, c->trazo, x0, y0, x1, y1);
        }
    } else {
        f[0] = rk_elipse_q4(ix, iy, pupr, pupr);
        pintar_en(c, f, 3, c->trazo, x0, y0, x1, y1);
        if (e->frijol) {
            /* La pupila deformada: una segunda elipse corrida. Una pupila de
             * compás no actúa. */
            f[0] = rk_elipse_q4(ix + pupr * 40 / 100 * lado, iy + pupr * 34 / 100,
                                pupr * 66 / 100, pupr * 78 / 100);
            pintar_en(c, f, 3, c->trazo, x0, y0, x1, y1);
        }
        /* Dos brillos: uno grande arriba, contra la luz, y uno chico abajo
         * del otro lado. Con uno solo el ojo queda seco. */
        f[0] = rk_elipse_q4(ix - irx * 36 / 100, iy - iry * 40 / 100,
                            irx * 34 / 100, irx * 30 / 100);
        pintar_en(c, f, 3, COL_BLANCO, x0, y0, x1, y1);
        f[0] = rk_elipse_q4(ix + irx * 34 / 100, iy + iry * 42 / 100,
                            irx * 17 / 100, irx * 15 / 100);
        pintar_en(c, f, 3, rk_mix(COL_BLANCO, c->iris, 70), x0, y0, x1, y1);
    }

    /* --- 4. los párpados, del color del fondo ----------------------------- */
    if (e->g.tapa_sup > 0) {
        int ang = e->g.tapa_ang * -lado;
        int32_t ly = ey - ryv + (2 * ryv) * e->g.tapa_sup / 100;
        f[0] = rk_semiplano_arriba_q4(ex, ly, ang);
        f[1] = rk_elipse_q4(ex, ey, rx + PX(c, 2), ryv + PX(c, 2));
        pintar_en(c, f, 2, c->bg, x0, y0, x1, y1);
        /* Y su línea de pestaña, que es lo que lo vuelve un párpado y no un
         * recorte. */
        arco(c, ex, ly - rx * 30 / 100, rx * 86 / 100, PX(c, 5), false, c->trazo);
    }
    if (e->g.tapa_inf > 0) {
        int32_t ly = ey + ryv - (2 * ryv) * e->g.tapa_inf / 100;
        f[0] = rk_semiplano_abajo_q4(ex, ly, 0);
        f[1] = rk_elipse_q4(ex, ey, rx + PX(c, 2), ryv + PX(c, 2));
        pintar_en(c, f, 2, c->bg, x0, y0, x1, y1);
        arco(c, ex, ly + rx * 34 / 100, rx * 80 / 100, PX(c, 4), true, c->trazo);
    }

    /* --- 5. la arruga de tensión ------------------------------------------ */
    if (e->arrugas) {
        int32_t ax = ex + rx * 94 / 100 * lado;
        int32_t ay = ey + ryv * 30 / 100;
        capsula(c, ax, ay, ax + rx * 30 / 100 * lado, ay + ryv * 6 / 100,
                PX(c, 2), c->trazo);
    }

    /* --- 6. el corte de la familia ---------------------------------------- */
    if (c->p->familia == RK_OJOS_RASGADOS) {
        int corte = 16;
        f[0] = rk_semiplano_arriba_q4(ex, ey - ryv * 74 / 100, corte * lado);
        f[1] = rk_elipse_q4(ex, ey, rx + PX(c, 2), ryv + PX(c, 2));
        pintar_en(c, f, 2, c->bg, x0, y0, x1, y1);
    }
}

/* ----------------------------------------------------------- mejillas --- */
static void mejillas(cara_t *c, int32_t ex_izq, int32_t ex_der, int32_t rx,
                     int32_t ry)
{
    int lado;

    for (lado = -1; lado <= 1; lado += 2) {
        int32_t ex = lado < 0 ? ex_izq : ex_der;
        int32_t x = ex + lado * rx * 40 / 100;
        int32_t y = c->oy + ry + PQ(c, 6);

        switch (c->p->mejilla) {
        case RK_MEJILLA_HORIZONTAL:
            elipse(c, x, y + PQ(c, 1), PQ(c, 9), PQ(c, 3), c->rubor, 205);
            break;
        case RK_MEJILLA_BRILLO:
            elipse(c, x, y, PQ(c, 7), PQ(c, 5), c->rubor, 225);
            circulo(c, x - lado * PQ(c, 3), y - PQ(c, 2), PX(c, 3), c->blanco, 235);
            break;
        case RK_MEJILLA_PECAS:
            elipse(c, x, y, PQ(c, 7), PQ(c, 4), c->rubor, 110);
            circulo(c, x - lado * PQ(c, 3), y - PQ(c, 1), PX(c, 2), c->pecas, 255);
            circulo(c, x + lado * PQ(c, 1), y - PQ(c, 2), PX(c, 2), c->pecas, 255);
            circulo(c, x - lado * PQ(c, 1), y + PQ(c, 2), PX(c, 2), c->pecas, 255);
            break;
        case RK_MEJILLA_SUAVE:
            elipse(c, x, y, PQ(c, 6), PQ(c, 4), c->rubor, 175);
            break;
        default:
            circulo(c, x, y, PQ(c, 6), c->rubor, 215);
            break;
        }
    }
}

/* -------------------------------------------------------------- cejas --- */
static void cejas(cara_t *c, const expr_t *e, int32_t ex_izq, int32_t ex_der,
                  int32_t ry)
{
    const rk_persona_t *p = c->p;
    int32_t alto = PQ(c, p->ceja_alto + e->g.ceja_dy);
    int ang = p->ceja_angulo + e->g.ceja_ang;
    bool flotante = p->ceja == RK_CEJA_FLOTANTE;
    bool gruesa = p->ceja == RK_CEJA_GRUESA;
    int32_t hl = PQ(c, p->ojo_rx) * (flotante ? 34 : gruesa ? 82 : 68) / 100;
    /* NINGUNA ceja es una línea: hasta la fina tiene peso. Una ceja delgada y
       pareja no sostiene una expresión —se lee como un accesorio—, y todo el
       acting de una cara de dibujo pasa por las cejas. */
    int32_t r = flotante ? PX(c, 7) : gruesa ? PX(c, 8) : PX(c, 5);
    int lado;

    if (p->ceja == RK_CEJA_NINGUNA || e->cerrado != 0) {
        return;
    }
    if (p->familia == RK_OJOS_UNICO) {
        /* Con un ojo solo, una ceja sola: centrada y más larga. Inclinarla
           entera es lo único que le queda al cíclope para fruncir el ceño. */
        int32_t by = c->oy - ry - alto;
        int32_t largo = PQ(c, p->ojo_rx) * 90 / 100;
        int32_t k = (int32_t)((int64_t)largo * ang / 40);
        capsula(c, c->cx - largo, by + k, c->cx + largo, by - k, PX(c, 7), c->trazo);
        return;
    }
    for (lado = -1; lado <= 1; lado += 2) {
        int32_t ex = lado < 0 ? ex_izq : ex_der;
        int32_t by = c->oy - ry - alto;
        /* Negativo baja la punta de adentro (enojo); positivo la sube
         * (preocupación). */
        int32_t k = (int32_t)((int64_t)hl * ang / 40);
        int32_t xin = ex - lado * hl, xout = ex + lado * hl;
        int32_t mx = xin + (xout - xin) * 45 / 100;
        int32_t my = (by - k) + ((by + k) - (by - k)) * 45 / 100;
        capsula(c, xin, by - k, xout, by + k, r * 62 / 100, c->trazo);
        /* La ceja se dibuja en dos pasadas: gorda del lado de adentro y fina
           hacia la sien. Es lo que en un dibujo a mano hace el pincel al
           levantarse, y acá es la diferencia entre una ceja y un palito. */
        capsula(c, xin, by - k, mx, my, r, c->trazo);
        if (gruesa) {
            capsula(c, xin, by - k, xin + (mx - xin) / 2, by - k + (my - (by - k)) / 2,
                    r * 118 / 100, c->trazo);
        }
    }
}

/* --------------------------------------------------------------- boca --- */
/* Una boca curva por los extremos (cx +- w, ye), con flecha proporcional a
 * la curva: hacia abajo si sonríe, hacia arriba si hace mueca. Es un arco de
 * círculo por los mismos extremos cuyo radio crece a medida que la curva se
 * acerca a cero, y eso hace continua la transición entre sonrisa, boca recta
 * y mueca. La flecha llega a tres cuartos del ancho: una sonrisa suave. */
static void boca_curva(cara_t *c, int32_t w, int32_t y, int32_t gr, int curva)
{
    int32_t mag = curva < 0 ? -curva : curva;
    int32_t s = w * mag * 75 / 10000;                /* la flecha           */
    int32_t ye, yc, top, bot;
    int64_t R;
    rk_forma_t f[2];

    if (s < 1) {
        s = 1;
    }
    ye = curva > 0 ? y - s / 2 : y + s / 2;          /* altura de las puntas */
    R = ((int64_t)w * w + (int64_t)s * s) / (2 * s);
    yc = curva > 0 ? ye - (int32_t)(R - s) : ye + (int32_t)(R - s);
    top = curva > 0 ? ye - gr : ye - s - gr;
    bot = curva > 0 ? ye + s + gr : ye + gr;

    f[0] = rk_anillo_q4(c->cx, yc, (int32_t)R, gr);
    f[1] = curva > 0 ? rk_semiplano_abajo_q4(c->cx, ye, 0)
                     : rk_semiplano_arriba_q4(c->cx, ye, 0);
    rk_aa_pintar(c->fb, f, 2, c->trazo, 255,
                 (int)((c->cx - w - gr) / 16) - 2, (int)(top / 16) - 2,
                 (int)((c->cx + w + gr) / 16) + 3, (int)(bot / 16) + 3);
    /* Una segunda pasada más gruesa en el centro: el trazo de una boca
     * dibujada engorda en el medio y se afina en las comisuras. */
    f[0] = rk_anillo_q4(c->cx, yc, (int32_t)R, gr * 168 / 100);
    f[1] = rk_elipse_q4(c->cx, ye, w * 52 / 100, (int32_t)(s + gr) * 3);
    rk_aa_pintar(c->fb, f, 2, c->trazo, 255,
                 (int)((c->cx - w - gr) / 16) - 2, (int)(top / 16) - 3,
                 (int)((c->cx + w + gr) / 16) + 3, (int)(bot / 16) + 4);
    rk_aa_circulo(c->fb, c->cx - w, ye, gr / 2, c->trazo);
    rk_aa_circulo(c->fb, c->cx + w, ye, gr / 2, c->trazo);
}

static void boca(cara_t *c, const expr_t *e)
{
    const rk_persona_t *p = c->p;
    int32_t w = PQ(c, p->boca_ancho);
    int32_t y = c->by;
    int32_t gr = PX(c, 6);
    rk_forma_t f[2], d[3];

    if (e->agua && (e->boca == RK_BOCA_OPEN || e->boca == RK_BOCA_PANT)) {
        /* Bajo el agua la boca no es una "O" de compás: es un grito sordo,
           torcido y sin lengua —la lengua abajo del agua no se lee—, con dos
           burbujitas que se le escapan. */
        /* Vista a través del agua: el trazo se destiñe hacia el celeste y
           pierde filo. Es lo que hace que se lea COMO sumergida y no como una
           boca dibujada encima del charco. */
        rk_color_t hundido = rk_mix(c->trazo, COL_AGUA, 58);
        int32_t bw = w * 84 / 100;
        int32_t off = w * 18 / 100;
        f[0] = rk_elipse_q4(c->cx - off, y, bw, bw * 86 / 100);
        pintar(c, f, 1, hundido, 235);
        f[0] = rk_elipse_q4(c->cx + off, y + bw * 16 / 100, bw * 52 / 100, bw * 62 / 100);
        pintar(c, f, 1, hundido, 235);
        /* Y las dos burbujitas que se le escapan, ya del lado del vidrio. */
        circulo(c, c->cx + w, y - w * 80 / 100, PX(c, 5),
                rk_mix(COL_BLANCO, COL_AGUA, 40), 235);
        circulo(c, c->cx + w * 140 / 100, y - w * 150 / 100, PX(c, 3),
                rk_mix(COL_BLANCO, COL_AGUA, 40), 215);
        return;
    }

    switch ((rk_boca_t)e->boca) {
    case RK_BOCA_OPEN:
    case RK_BOCA_PANT: {
        /* Una boca abierta de dibujo tiene CUATRO capas: el labio que la
         * rodea, la sombra de la garganta (que es más oscura arriba, porque
         * ahí entra menos luz), los dientes de arriba y la lengua. Con una
         * elipse oscura sola, la cara se queda en emoji. */
        rk_relleno_t dentro;
        int32_t bw = w * 74 / 100, bh = w * 62 / 100;
        rk_forma_t hueco;

        f[0] = rk_elipse_q4(c->cx, y, bw, bh);
        pintar(c, f, 1, c->trazo, 255);
        hueco = rk_elipse_q4(c->cx, y, bw - PX(c, 4), bh - PX(c, 4));
        dentro = rk_lineal(c->cx, y - bh, c->cx, y + bh,
                           rk_mix(c->trazo, COL_LENGUA, 30),
                           rk_mix(c->trazo, COL_LENGUA, 96));
        f[0] = hueco;
        pintar_rel_en(c, f, 1, &dentro,
                      (int)((c->cx - bw) / 16) - 2, (int)((y - bh) / 16) - 2,
                      (int)((c->cx + bw) / 16) + 3, (int)((y + bh) / 16) + 3);
        /* Los dientes de arriba: una franja clara pegada al labio superior. */
        f[0] = rk_elipse_q4(c->cx, y - bh * 62 / 100, bw * 86 / 100, bh * 40 / 100);
        f[1] = hueco;
        pintar(c, f, 2, rk_mix(COL_BLANCO, c->bg, 26), 255);
        /* Y la lengua, redonda, abajo. */
        f[0] = rk_elipse_q4(c->cx, y + bh * 52 / 100, bw * 62 / 100, bh * 52 / 100);
        f[1] = hueco;
        pintar(c, f, 2, c->lengua, 255);
        if (e->boca == RK_BOCA_PANT) {
            f[0] = rk_elipse_q4(c->cx, y + bh * 96 / 100, bw * 36 / 100, bh * 42 / 100);
            pintar(c, f, 1, c->lengua, 255);
            f[0] = rk_elipse_q4(c->cx, y + bh * 96 / 100, bw * 36 / 100, bh * 42 / 100);
            f[1] = rk_semiplano_abajo_q4(c->cx, y + bh * 96 / 100, 0);
            pintar(c, f, 2, rk_mix(c->lengua, c->trazo, 60), 255);
        }
        return;
    }

    case RK_BOCA_WAVY: {
        /* Zigzag de cuatro tramos: temblor o asco, según los ojos. */
        int32_t paso = w / 2;
        int32_t alto = w / 4;
        int k;
        for (k = 0; k < 4; k++) {
            int32_t xa = c->cx - w + paso * k;
            int32_t ya = (k % 2 == 0) ? y : y - alto;
            int32_t yb = (k % 2 == 0) ? y - alto : y;
            capsula(c, xa, ya, xa + paso, yb, gr / 2, c->trazo);
        }
        return;
    }

    default:
        break;
    }

    /* Sonrisa, recta y mueca son UNA curva, de 100 a -100. Las bocas con
     * forma propia valen a partir de media sonrisa; por debajo, la curva
     * genérica; cerca de cero, la recta. */
    {
        int curva = e->g.boca_curva;
        if (curva >= 50 && p->boca == RK_BOCA_GATO) {
            arco(c, c->cx - w / 2, y - w / 3, w / 2, gr, true, c->trazo);
            arco(c, c->cx + w / 2, y - w / 3, w / 2, gr, true, c->trazo);
        } else if (curva >= 50 && p->boca == RK_BOCA_D) {
            /* ":D": la mitad de abajo de una elipse, con la lengua. */
            int32_t my = y - w * 30 / 100;
            f[0] = rk_elipse_q4(c->cx, my, w, w * 90 / 100);
            f[1] = rk_semiplano_abajo_q4(c->cx, my, 0);
            pintar(c, f, 2, c->trazo, 255);
            d[0] = f[0];
            d[1] = f[1];
            d[2] = rk_elipse_q4(c->cx, my + w * 88 / 100, w * 56 / 100, w * 36 / 100);
            pintar(c, d, 3, c->lengua, 255);
        } else if (curva >= 50 && p->boca == RK_BOCA_DIENTECITO) {
            /* Una sonrisa abierta chica con un dientito que cuelga de
             * arriba, apenas corrido del centro. */
            int32_t my = y - w * 22 / 100;
            f[0] = rk_elipse_q4(c->cx, my, w * 80 / 100, w * 72 / 100);
            f[1] = rk_semiplano_abajo_q4(c->cx, my, 0);
            pintar(c, f, 2, c->trazo, 255);
            d[0] = f[0];
            d[1] = f[1];
            d[2] = rk_elipse_q4(c->cx, my + w * 64 / 100, w * 42 / 100, w * 24 / 100);
            pintar(c, d, 3, c->lengua, 255);
            d[2] = rk_elipse_q4(c->cx + w * 22 / 100, my, w * 17 / 100, w * 32 / 100);
            pintar(c, d, 3, c->blanco, 255);
        } else if (curva >= 40 && p->boca == RK_BOCA_LADEADA) {
            /* La media sonrisa del piloto: sube de un lado nomás. Un arco
               entero se lee como ternura; medio arco, como picardía. */
            int32_t xa = c->cx - w * 70 / 100, xb = c->cx + w * 85 / 100;
            capsula(c, xa, y, c->cx, y + w * 16 / 100, gr / 2, c->trazo);
            capsula(c, c->cx, y + w * 16 / 100, xb, y - w * 26 / 100, gr / 2, c->trazo);
        } else if (p->boca == RK_BOCA_SOBRIA && curva > -40) {
            /* Corta y casi recta: la aprobación de quien no va a decirlo. */
            int32_t k = w * curva * 22 / 10000;
            capsula(c, c->cx - w * 46 / 100, y - k, c->cx + w * 46 / 100, y + k,
                    gr / 2, c->trazo);
        } else if (curva <= -40 && p->boca == RK_BOCA_SIERRA) {
            /* Dientes de sierra: la mueca del cíclope cuando algo no le
               cierra. Es fea a propósito, y dura poco. */
            int32_t paso = w * 2 / 5;
            int k;
            for (k = 0; k < 5; k++) {
                int32_t xa = c->cx - w + paso * k;
                capsula(c, xa, y, xa + paso / 2, y - w * 34 / 100, gr / 3, c->trazo);
                capsula(c, xa + paso / 2, y - w * 34 / 100, xa + paso, y, gr / 3, c->trazo);
            }
        } else if (curva > -12 && curva < 12) {
            capsula(c, c->cx - w * 2 / 3, y, c->cx + w * 2 / 3, y, gr / 2, c->trazo);
        } else {
            boca_curva(c, w, y, gr, curva);
        }
    }
}


/* --------------------------------------------------------- el clima --- */
/* El visor inundado: agua celeste translúcida hasta la mitad de la cara, con
 * la superficie ondulando. Va ENCIMA de los ojos y la boca —está delante, no
 * detrás— y por eso se dibuja al final; con alfa, para que los ojos se sigan
 * viendo abajo, asustados. */
static void agua(cara_t *c)
{
    /* El celeste va casi opaco. Un azul transparente sobre un cuerpo naranja
       da verde oliva —son complementarios— y lo que tiene que leerse a un
       metro es AGUA, no un filtro. Con 190 de alfa todavía se adivina la boca
       abajo, que es justo lo que se quiere ver. */
    rk_color_t azul = rk_mix(COL_AGUA, c->bg, 18);
    rk_color_t claro = rk_mix(COL_BLANCO, azul, 70);
    int32_t u = (int32_t)c->u * 8;
    /* Un poco por debajo del medio: los ojos tienen que quedar afuera,
       asomando, y la boca adentro. */
    int32_t nivel = c->cy + u * 16 / 100
                  + (int32_t)rk_sin8((uint8_t)(c->t / 18u)) * PX(c, 3) / 127;
    rk_forma_t f[2];
    int i;

    /* La pecera no es un rectángulo: se recorta contra una elipse grande, y
       así en el cuerpo 3D el agua se ve como un charco dentro de la cara y no
       como una calcomanía cuadrada pegada en la panza. */
    f[0] = rk_semiplano_abajo_q4(c->cx, nivel, 0);
    f[1] = rk_elipse_q4(c->cx, c->cy, u * 96 / 100, u * 96 / 100);
    pintar(c, f, 2, azul, 232);

    (void)claro;
    (void)i;
}

/* La superficie del agua, aparte: va DESPUÉS de la boca, porque es lo que
 * está más cerca del vidrio. */
static void agua_superficie(cara_t *c)
{
    rk_color_t claro = rk_mix(COL_BLANCO, rk_mix(COL_AGUA, c->bg, 18), 70);
    int32_t u = (int32_t)c->u * 8;
    int32_t nivel = c->cy + u * 16 / 100
                  + (int32_t)rk_sin8((uint8_t)(c->t / 18u)) * PX(c, 3) / 127;
    int i;
    for (i = -3; i <= 3; i++) {
        int32_t x = c->cx + PQ(c, 12) * i;
        int32_t dy = (int32_t)rk_sin8((uint8_t)(c->t / 14u + (uint32_t)(i * 40)))
                   * PX(c, 2) / 127;
        capsula(c, x, nivel + dy, x + PQ(c, 12), nivel - dy, PX(c, 3), claro);
    }
}

/* La cara cuarteada de la sequía: unas pocas rayas finas, del color de los
 * ojos aguado contra el fondo. Pocas y quietas: si se movieran parecerían
 * suciedad de la pantalla. */
static void grietas(cara_t *c)
{
    rk_color_t col = rk_mix(c->trazo, c->bg, 150);
    int32_t r = PX(c, 2);
    /* Medio lado en Q4: desde el centro al borde hay esto, no el lado entero
       (con el lado entero las grietas caían fuera de la pantalla). */
    int32_t u = (int32_t)c->u * 8;
    capsula(c, c->cx - u * 34 / 100, c->cy - u * 30 / 100,
            c->cx - u * 26 / 100, c->cy - u * 6 / 100, r, col);
    capsula(c, c->cx - u * 26 / 100, c->cy - u * 6 / 100,
            c->cx - u * 34 / 100, c->cy + u * 14 / 100, r, col);
    capsula(c, c->cx + u * 30 / 100, c->cy - u * 16 / 100,
            c->cx + u * 22 / 100, c->cy + u * 6 / 100, r, col);
    capsula(c, c->cx + u * 22 / 100, c->cy + u * 6 / 100,
            c->cx + u * 31 / 100, c->cy + u * 24 / 100, r, col);
    capsula(c, c->cx - u * 6 / 100, c->cy + u * 30 / 100,
            c->cx + u * 2 / 100, c->cy + u * 40 / 100, r, col);
}


/* --------------------------------------------------------- acabados --- */
/* Lo que hace que una piel épica se note SIN cambiarle el color al
 * personaje. Cada uno va con el carácter de su Rooti, y todos son
 * movimiento: el premio se ve cuando la cara está viva, no en una captura.
 *
 * Los cuatro comparten la misma idea barata: una franja diagonal que cruza
 * la cara cada tantos segundos. Lo que cambia es el color, el ancho y qué
 * deja atrás. */
static void barrido(cara_t *c, rk_color_t col, uint8_t alfa, int32_t ancho,
                    uint32_t periodo, int32_t desfase)
{
    int32_t u = (int32_t)c->u * 8;            /* medio lado, en Q4 */
    /* De abajo a la izquierda hasta arriba a la derecha, y vuelve a empezar. */
    int32_t k = (int32_t)(((c->t + (uint32_t)desfase) % periodo) * 300u / periodo) - 100;
    int32_t x = c->cx + u * k / 100;
    rk_forma_t f = rk_capsula_q4(x - u * 40 / 100, c->cy + u * 70 / 100,
                                 x + u * 40 / 100, c->cy - u * 70 / 100, ancho);
    pintar(c, &f, 1, col, alfa);
}

static void acabados(cara_t *c, uint8_t set)
{
    int32_t u = (int32_t)c->u * 8;            /* medio lado, en Q4 */
    int i;

    if (set & RK_ADORNO_METAL) {
        /* Dos filos duros y juntos: chapa pulida. */
        barrido(c, rk_mix(COL_BLANCO, c->bg, 40), 150, PX(c, 7), 4200u, 0);
        barrido(c, rk_mix(COL_BLANCO, c->bg, 90), 110, PX(c, 3), 4200u, 260);
    }
    if (set & RK_ADORNO_ORO) {
        /* Ancho, tibio y lento: oro, no acero. */
        barrido(c, rk_mix(COL_ORO, c->bg, 60), 150, PX(c, 14), 5200u, 0);
    }
    if (set & RK_ADORNO_CRISTAL) {
        /* Frío y facetado: el destello cruza y deja tres esquirlas quietas. */
        barrido(c, rk_mix(COL_HIELO, c->bg, 70), 130, PX(c, 6), 3800u, 0);
        for (i = -1; i <= 1; i++) {
            int32_t x = c->cx + u * i * 34 / 100;
            int32_t y = c->cy - u * 52 / 100 + (i == 0 ? u * 12 / 100 : 0);
            destello(c, x, y, PQ(c, 5), rk_mix(COL_HIELO, c->bg, 40), 190);
        }
    }
    if (set & RK_ADORNO_FUEGO) {
        /* Llamitas lamiendo el borde de abajo. Cada una con su ritmo, o
           parecerían una sola cosa que late. */
        for (i = -2; i <= 2; i++) {
            uint8_t ph = (uint8_t)(c->t / 9u + (uint32_t)(i * 51));
            int32_t alto = PQ(c, 12) + (int32_t)rk_sin8(ph) * PQ(c, 7) / 127;
            int32_t x = c->cx + u * i * 26 / 100;
            int32_t base = c->cy + u * 82 / 100;
            elipse(c, x, base - alto / 2, PQ(c, 5), alto / 2,
                   rk_mix(COL_FUEGO, c->bg, 30), 210);
            elipse(c, x, base - alto / 3, PQ(c, 3), alto / 3,
                   rk_mix(COL_ORO, c->bg, 20), 230);
        }
    }
}

/* ------------------------------------------------------------ adornos --- */
static void adornos(cara_t *c, const expr_t *e, uint8_t set, int32_t ex_izq,
                    int32_t ex_der, int32_t ry)
{
    int i;

    (void)ex_izq;
    if (set & RK_ADORNO_AURA) {
        /* Un anillo que respira en el borde de la pantalla. Suave a
         * propósito: es un premio, no una alarma. */
        int32_t r = (int32_t)c->u * 8 - PX(c, 5)
                  + (int32_t)rk_sin8((uint8_t)(c->t / 12u)) * PX(c, 2) / 127;
        rk_forma_t f = rk_anillo_q4(c->cx, c->cy, r, PX(c, 4));
        pintar(c, &f, 1, c->acento, 105);
    }
    if (set & RK_ADORNO_BRILLOS) {
        for (i = 0; i < 3; i++) {
            uint8_t ph = (uint8_t)(c->t / 20u + (uint32_t)i * 85u);
            int32_t r = PQ(c, 40) + (int32_t)rk_sin8(ph) * PQ(c, 3) / 127;
            int32_t x = c->cx + (int32_t)rk_sin8((uint8_t)(i * 85 + 30)) * r / 127;
            int32_t y = c->cy + (int32_t)rk_sin8((uint8_t)(i * 85 + 94)) * r / 127;
            int32_t l = PQ(c, 4) + (rk_sin8(ph) > 0 ? PX(c, 2) : 0);
            destello(c, x, y, l, c->acento, 235);
        }
    }
    if (set & RK_ADORNO_LUCES) {
        /* Luces que suben despacio, con núcleo claro: bioluminiscencia. */
        int32_t alto = (int32_t)c->fb->h * 16;
        for (i = 0; i < 6; i++) {
            uint16_t h = rk_hash((uint16_t)(i * 733 + 5));
            int32_t x = (int32_t)(h % (uint16_t)c->fb->w) * 16;
            int32_t sube = (int32_t)((c->t / 24u + (h >> 4)) % 256u);
            int32_t y = alto - sube * alto / 256;
            int32_t r = PX(c, 3) + PX(c, i % 2);
            circulo(c, x, y, r, c->acento, 170);
            circulo(c, x, y, r / 2, c->blanco, 220);
        }
    }
    if (set & RK_ADORNO_CORONA) {
        /* Tres puntas y una base, en oro con una línea más oscura para que se
         * lea sobre los fondos claros. */
        int32_t y = c->oy - ry - PQ(c, 15);
        int32_t w = PQ(c, 7);
        int k;
        capsula(c, c->cx - w * 19 / 10, y + PX(c, 2), c->cx + w * 19 / 10, y + PX(c, 2),
                PX(c, 5), COL_ORO_OSC);
        for (k = -1; k <= 1; k++) {
            int32_t x = c->cx + k * w * 13 / 10;
            int32_t h = (k == 0) ? PQ(c, 9) : PQ(c, 7);
            rk_aa_triangulo(c->fb, x - w * 6 / 10, y, x + w * 6 / 10, y, x, y - h,
                            COL_ORO, 255);
            circulo(c, x, y - h, PX(c, 3), COL_ORO, 255);
        }
        capsula(c, c->cx - w * 19 / 10, y, c->cx + w * 19 / 10, y, PX(c, 4), COL_ORO);
    }

    /* --- los gestos de ánimo que no son ojos ni boca -------------------- */
    if (e->gota) {
        int32_t x = ex_der + PQ(c, 16);
        int32_t y = c->oy - ry - PQ(c, 2)
                  + (int32_t)((c->t / 12u) % 60u) * PX(c, 1) / 6;
        gota(c, x, y, PQ(c, 4), COL_GOTA);
    }
    if (e->zzz) {
        for (i = 0; i < 3; i++) {
            uint32_t ph = (c->t / 11u + (uint32_t)i * 110u) % 330u;
            int32_t s = PQ(c, 3) + (int32_t)ph * PX(c, 1) / 60;
            int32_t x = c->cx + PQ(c, 24) + (int32_t)ph * PX(c, 1) / 12;
            int32_t y = c->oy - PQ(c, 10) - (int32_t)ph * PX(c, 1) / 5;
            uint8_t alfa = (uint8_t)(ph > 250u ? (330u - ph) * 3u : 240u);
            int32_t g = PX(c, 2);
            rk_forma_t f[1];
            if (ph > 300u) {
                continue;
            }
            f[0] = rk_capsula_q4(x - s, y - s, x + s, y - s, g);
            pintar(c, f, 1, c->trazo, alfa);
            f[0] = rk_capsula_q4(x + s, y - s, x - s, y + s, g);
            pintar(c, f, 1, c->trazo, alfa);
            f[0] = rk_capsula_q4(x - s, y + s, x + s, y + s, g);
            pintar(c, f, 1, c->trazo, alfa);
        }
    }
    if (e->nieve) {
        /* Copos que bajan despacio y se van de costado: el aparato tiene
           frio, y se ve antes de leer la cara. */
        for (i = 0; i < 5; i++) {
            uint16_t h = rk_hash((uint16_t)(i * 1597 + 11));
            int32_t baja = (int32_t)((c->t / 26u + (h >> 6)) % 256u);
            int32_t x = (int32_t)(h % (uint16_t)c->fb->w) * 16
                      + (int32_t)rk_sin8((uint8_t)(c->t / 18u + i * 51u)) * PX(c, 4) / 127;
            int32_t y = baja * (int32_t)c->fb->h * 16 / 256;
            int32_t r = PX(c, 2) + PX(c, i % 2);
            int k;
            for (k = 0; k < 3; k++) {
                int32_t a = (int32_t)rk_sin8((uint8_t)(k * 85 + 64)) * r * 2 / 127;
                int32_t b = (int32_t)rk_sin8((uint8_t)(k * 85)) * r * 2 / 127;
                capsula(c, x - a, y - b, x + a, y + b, PX(c, 1), c->blanco);
            }
        }
    }
    if (e->vaho) {
        /* Dos hilos de vapor que suben ondulando, arriba de la cabeza. */
        for (i = 0; i < 2; i++) {
            int32_t x0 = c->cx + (i ? PQ(c, 14) : -PQ(c, 14));
            uint32_t ph = (c->t / 14u + (uint32_t)i * 128u) % 256u;
            int k;
            for (k = 0; k < 4; k++) {
                int32_t y = c->oy - ry - PQ(c, 14) - (int32_t)k * PQ(c, 7)
                          - (int32_t)ph * PQ(c, 7) / 256;
                int32_t x = x0 + (int32_t)rk_sin8((uint8_t)(ph + (uint32_t)k * 40u)) * PX(c, 3) / 127;
                uint8_t alfa = (uint8_t)(110u - (uint32_t)k * 22u);
                circulo(c, x, y, PX(c, 2) + PX(c, k % 2), c->blanco, alfa);
            }
        }
    }
    if (e->polvillo) {
        /* El aire seco: motas que flotan sin subir ni bajar del todo. */
        for (i = 0; i < 4; i++) {
            uint16_t h = rk_hash((uint16_t)(i * 911 + 23));
            int32_t x = (int32_t)(h % (uint16_t)c->fb->w) * 16
                      + (int32_t)rk_sin8((uint8_t)(c->t / 22u + i * 64u)) * PQ(c, 5) / 127;
            int32_t y = (int32_t)((h >> 7) % (uint16_t)c->fb->h) * 16
                      + (int32_t)rk_sin8((uint8_t)(c->t / 29u + i * 40u + 64u)) * PQ(c, 4) / 127;
            circulo(c, x, y, PX(c, 1) + PX(c, i % 2), c->pecas, 150);
        }
    }
    if (e->burbujas) {
        for (i = 0; i < 6; i++) {
            uint16_t h = rk_hash((uint16_t)(i * 2654 + 17));
            int32_t x = (int32_t)(h % (uint16_t)c->fb->w) * 16;
            int32_t sube = (int32_t)((c->t / 9u + (h >> 5)) % 256u);
            int32_t y = (int32_t)c->fb->h * 16 - sube * c->fb->h * 16 / 256;
            rk_forma_t f = rk_anillo_q4(x, y, PQ(c, 3) + PX(c, i % 3), PX(c, 2));
            pintar(c, &f, 1, c->iris, 190);
        }
    }
}

/* ---------------------------------------------------------------- cara --- */
/* La respiración mueve la cara entera, de a fracciones de pixel. */
static int32_t respiracion(const cara_t *c, const rk_look_t *lk, uint32_t t)
{
    return lk->bob_amp
        ? (int32_t)rk_sin8((uint8_t)(t * lk->bob_speed / 1000u)) * PX(c, lk->bob_amp) / 127
        : 0;
}

static int acotar(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* `desde` y `hacia` son el mismo ánimo salvo durante una transición; `t_pct`
 * dice en qué punto de ella estamos. Con desde == hacia, sin mimo y sin
 * mirada, el camino es exactamente el de una cara sola. */
static void dibujar(rk_fb_t *fb, const rk_persona_t *p, const rk_piel_t *piel,
                    rk_mood_t desde, rk_mood_t hacia, uint8_t t_pct,
                    uint8_t adornos_extra, uint8_t cierre, uint8_t mimo_pct,
                    const rk_face_mirada_t *mirada, uint32_t t)
{
    const rk_look_t *lk = rk_look(hacia);
    const rk_look_t *lka = rk_look(desde);
    bool mezcla = desde != hacia && t_pct < 100u;
    expr_t e;
    cara_t c;
    int32_t rx, ry, dx, bob, temblor;
    uint8_t set;

    if (fb == NULL || fb->px == NULL || fb->w <= 0 || fb->h <= 0) {
        return;
    }
    if (p == NULL) {
        p = rk_persona_at(0);
    }
    if (piel == NULL) {
        piel = rk_persona_piel(p, RK_RAREZA_COMUN);
    }
    if (mezcla && t_pct == 0u) {
        /* El principio es exactamente el ánimo de origen. */
        lk = lka;
        mezcla = false;
        hacia = desde;
    }

    memset(&c, 0, sizeof c);
    c.fb = fb;
    c.p = p;
    c.t = t;
    c.u = fb->w < fb->h ? fb->w : fb->h;
    pintura(&c, piel, lk);
    if (mezcla) {
        /* Tinte y penumbra se funden entre los dos ánimos. */
        cara_t ca = c;
        pintura(&ca, piel, lka);
        fundir(&c, &ca, &c, (uint8_t)((uint32_t)t_pct * 255u / 100u));
    }
    if (mimo_pct > 0u) {
        /* El mimo trae los colores de contento: la penumbra de una cara
         * triste se levanta mientras la acarician. */
        cara_t cm = c;
        pintura(&cm, piel, rk_look(RK_MOOD_HAPPY));
        fundir(&c, &c, &cm, (uint8_t)((uint32_t)mimo_pct * 255u / 100u));
    }

    rk_fb_clear(fb, c.bg);
    /* El rostro no es un fondo plano: es una cabeza, y una cabeza tiene luz
       arriba y sombra abajo. Un radial suave sobre toda la pantalla es lo que
       la saca de "dibujo vectorial" y la mete en "ilustración", y cuesta una
       multiplicación por pixel. */
    {
        int32_t u = (int32_t)c.u * 16;
        rk_forma_t todo = rk_elipse_q4(c.cx, c.cy, u, u);
        rk_relleno_t vol = rk_radial(c.cx - u * 22 / 100, c.cy - u * 30 / 100,
                                     u * 210 / 100,
                                     rk_mix(c.bg, COL_BLANCO, 34),
                                     rk_mix(c.bg, c.trazo, 46));
        rk_aa_pintar_relleno(fb, &todo, 1, &vol, 255, 0, 0, fb->w, fb->h);
    }

    bob = respiracion(&c, lk, t);
    temblor = lk->shiver ? (((t / 60u) % 2u) ? PX(&c, 2) : -PX(&c, 2)) : 0;
    if (mezcla) {
        int32_t bob_a = respiracion(&c, lka, t);
        bob = bob_a + (bob - bob_a) * (int32_t)t_pct / 100;
        if (t_pct < 50u) {
            temblor = lka->shiver ? (((t / 60u) % 2u) ? PX(&c, 2) : -PX(&c, 2)) : 0;
        }
    }
    if (mimo_pct > 0u) {
        /* El ronroneo: un vaivén de un pixel de cara, ocho veces por
         * segundo, que reemplaza a la respiración. Y el temblor se va. */
        int32_t ronroneo = (int32_t)rk_sin8((uint8_t)(t * 2u)) * PX(&c, 1) / 127;
        bob = bob + (ronroneo - bob) * (int32_t)mimo_pct / 100;
        temblor = temblor * (100 - (int32_t)mimo_pct) / 100;
    }

    c.cx = (int32_t)fb->w * 8 + temblor;
    c.cy = (int32_t)fb->h * 8 + bob;
    if (mirada != NULL) {
        /* Mirar al costado gira un poco la cara entera. */
        c.cx += mirada->mira_x * PX(&c, 3) / 100;
    }
    c.oy = c.cy + PQ(&c, p->ojo_dy);
    c.by = c.cy + PQ(&c, p->boca_dy);

    if (mezcla) {
        expr_t ea = expresion(p, desde, lka, t, cierre);
        expr_t eb = expresion(p, hacia, lk, t, cierre);
        e = mezclar(&ea, &eb, t_pct);
    } else {
        e = expresion(p, hacia, lk, t, cierre);
    }
    if (mimo_pct > 0u) {
        expr_t em = expresion_mimo(p, t);
        e = mezclar(&e, &em, mimo_pct);
    }
    if (mirada != NULL) {
        if (e.cerrado == 0 && !e.cruz && !e.espiral) {
            e.g.mira_x = acotar(e.g.mira_x + mirada->mira_x, -100, 100);
            e.g.mira_y = acotar(e.g.mira_y + mirada->mira_y, -100, 100);
        }
        if (mirada->preocupado > 0u) {
            int k = mirada->preocupado > 100u ? 100 : (int)mirada->preocupado;
            /* Cejas altas por el lado de adentro, y la sonrisa se afloja: no
             * se puede sonreír mirando a un vecino con sed. */
            e.g.ceja_dy += 5 * k / 100;
            e.g.ceja_ang += 14 * k / 100;
            if (e.g.boca_curva > 0) {
                e.g.boca_curva -= e.g.boca_curva * 7 * k / 1000;
            }
        }
    }

    rx = PQ(&c, p->ojo_rx);
    ry = PQ(&c, p->ojo_ry);
    dx = PQ(&c, p->ojo_dx);
    set = (uint8_t)(adornos_extra | piel->adornos);

    /* El aura va detrás de todo. */
    if (set & RK_ADORNO_AURA) {
        adornos(&c, &e, RK_ADORNO_AURA, c.cx - dx, c.cx + dx, ry);
    }
    if (p->familia == RK_OJOS_UNICO) {
        /* Un ojo solo, en el medio. Las mejillas se van a los costados de la
           cara, porque si no quedarían debajo del mismo ojo. */
        ojo(&c, &e, c.cx, c.oy, rx, ry, -1);
        mejillas(&c, c.cx - rx * 102 / 100, c.cx + rx * 102 / 100,
                 rx * 46 / 100, ry * 62 / 100);
    } else {
        ojo(&c, &e, c.cx - dx, c.oy, rx, ry, -1);
        ojo(&c, &e, c.cx + dx, c.oy, rx, ry, +1);
        mejillas(&c, c.cx - dx, c.cx + dx, rx, ry);
    }
    cejas(&c, &e, c.cx - dx, c.cx + dx, ry);
    /* El agua va DEBAJO de la boca: así el agua se ve limpia y la boca se ve
       a través de ella, que es lo que pasa en una pecera. Pintándola encima,
       el celeste se mezclaba con el naranja del cuerpo y daba verde. */
    if (e.agua) {
        agua(&c);
    }
    boca(&c, &e);
    if (e.agua) {
        agua_superficie(&c);
    }
    if (e.grietas) {
        grietas(&c);
    }
    adornos(&c, &e, (uint8_t)(set & (uint8_t)~RK_ADORNO_AURA), c.cx - dx, c.cx + dx, ry);
    acabados(&c, set);
}

static const rk_piel_t *piel_de(const rk_persona_t *p, uint8_t rareza)
{
    return rk_persona_piel(p != NULL ? p : rk_persona_at(0), rareza);
}

void rk_face_draw(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                  rk_mood_t mood, rk_severity_t sev, uint8_t adornos_extra,
                  uint32_t t_ms)
{
    (void)sev;
    dibujar(fb, p, piel_de(p, rareza), mood, mood, 100u, adornos_extra, 0u, 0u,
            NULL, t_ms);
}

void rk_face_draw_cierre(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                         rk_mood_t mood, rk_severity_t sev,
                         uint8_t adornos_extra, uint8_t cierre, uint32_t t_ms)
{
    (void)sev;
    dibujar(fb, p, piel_de(p, rareza), mood, mood, 100u, adornos_extra,
            cierre > 100u ? 100u : cierre, 0u, NULL, t_ms);
}

void rk_face_draw_mezcla(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                         rk_mood_t desde, rk_mood_t hacia, uint8_t t_pct,
                         rk_severity_t sev, uint8_t adornos_extra,
                         uint8_t cierre, uint32_t t_ms)
{
    (void)sev;
    dibujar(fb, p, piel_de(p, rareza), desde, hacia, t_pct > 100u ? 100u : t_pct,
            adornos_extra, cierre > 100u ? 100u : cierre, 0u, NULL, t_ms);
}

void rk_face_draw_mimo(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                       rk_mood_t mood, uint8_t mimo_pct,
                       rk_severity_t sev, uint8_t adornos_extra,
                       uint32_t t_ms)
{
    (void)sev;
    dibujar(fb, p, piel_de(p, rareza), mood, mood, 100u, adornos_extra, 0u,
            mimo_pct > 100u ? 100u : mimo_pct, NULL, t_ms);
}

void rk_face_draw_mirada(rk_fb_t *fb, const rk_persona_t *p, uint8_t rareza,
                         rk_mood_t mood, rk_severity_t sev,
                         uint8_t adornos_extra, const rk_face_mirada_t *m,
                         uint32_t t_ms)
{
    rk_face_mirada_t mm;

    if (m == NULL) {
        rk_face_draw(fb, p, rareza, mood, sev, adornos_extra, t_ms);
        return;
    }
    mm.mira_x = acotar(m->mira_x, -100, 100);
    mm.mira_y = acotar(m->mira_y, -100, 100);
    mm.preocupado = m->preocupado > 100u ? 100u : m->preocupado;
    dibujar(fb, p, piel_de(p, rareza), mood, mood, 100u, adornos_extra, 0u, 0u,
            &mm, t_ms);
}

void rk_face_draw_con_piel(rk_fb_t *fb, const rk_persona_t *p,
                           const rk_piel_t *piel, rk_mood_t mood,
                           rk_severity_t sev, uint8_t adornos_extra,
                           uint32_t t_ms)
{
    (void)sev;
    dibujar(fb, p, piel, mood, mood, 100u, adornos_extra, 0u, 0u, NULL, t_ms);
}

uint8_t rk_face_adornos_etapa(int etapa)
{
    /* La piel te toca en el cofre; esto se gana cuidando la planta. */
    if (etapa >= 4) { return RK_ADORNO_AURA | RK_ADORNO_CORONA; }
    if (etapa >= 3) { return RK_ADORNO_AURA; }
    if (etapa >= 2) { return RK_ADORNO_BRILLOS; }
    return 0u;
}
