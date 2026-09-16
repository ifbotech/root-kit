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

/* Alto de los ojos y de la boca respecto del centro, en centésimas de U. */
#define OJOS_DY   (-6)
#define BOCA_DY   (25)

#define COL_LENGUA  RK_RGB(255, 112, 142)
#define COL_GOTA    RK_RGB(120, 204, 255)
#define COL_ORO     RK_RGB(255, 200,  60)
#define COL_CURITA  RK_RGB(242, 196, 150)
#define COL_GASA    RK_RGB(214, 160, 116)

typedef struct {
    rk_fb_t            *fb;
    const rk_persona_t *p;
    int32_t  u;             /* lado de la cara en pixeles                  */
    int32_t  cx, cy;        /* centro de la cara, Q4                       */
    int32_t  oy;            /* altura de los ojos, Q4                      */
    uint32_t t;
    rk_color_t bg, sombra, trazo, blanco, iris, acento;
} cara_t;

/* La expresión concreta de un cuadro: lo que el ánimo pidió, pasado por la
 * familia de ojos del modelo y por el parpadeo del instante.
 *
 * En dos partes: la geometría (rk_face_geom_t), que se puede interpolar
 * entre dos ánimos, y lo discreto, que no. */
typedef struct {
    rk_face_geom_t g;
    int  cerrado;     /* 0 abierto, 1 cerrado contento (^), 2 dormido (u)  */
    bool cruz;
    bool espiral;
    int  boca;        /* rk_boca_t: el estilo; la curva va en g.boca_curva */
    bool gota;
    bool zzz;
    bool burbujas;
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

rk_color_t rk_face_fondo(const rk_persona_t *p, rk_mood_t mood)
{
    if (p == NULL) {
        p = rk_persona_at(0);
    }
    return tratar(p->fondo, rk_look(mood));
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
    case RK_OJO_WIDE:   e.g.abre = 112; e.g.pupila = 62;            break;
    case RK_OJO_SLEEPY: e.g.tapa_sup = 44; e.g.tapa_ang = -10;      break;
    case RK_OJO_DEAD:   e.cruz = true;                          break;
    case RK_OJO_DIZZY:  e.espiral = true;                       break;
    case RK_OJO_GLITCH: e.g.tapa_sup = 30; e.g.pupila = 80;         break;
    default:                                                    break;
    }

    /* 2. Los gestos propios de cada ánimo, que no son sólo ojos. */
    switch (mood) {
    case RK_MOOD_THIRSTY:
        e.gota = true; e.g.mira_y = 45; e.g.ceja_ang = 10; break;
    case RK_MOOD_HOT:
        e.gota = true; e.g.ceja_ang = 8;                break;
    case RK_MOOD_COLD:
        e.g.ceja_ang = 12; e.g.ceja_dy = 3; e.g.tapa_inf = 22; break;
    case RK_MOOD_DROWNING:
        e.burbujas = true; e.g.ceja_dy = 5; e.g.ceja_ang = 10; break;
    case RK_MOOD_DARK:
        e.g.ceja_dy = 4; e.g.ceja_ang = 8; e.g.mira_x = 55;  break;
    case RK_MOOD_SLEEPING:
        e.zzz = true;                                  break;
    case RK_MOOD_UNKNOWN:
        e.g.mira_y = -60; e.g.mira_x = -30; e.g.ceja_dy = 4; break;
    case RK_MOOD_PARCHED_AIR:
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

    /* 3. La familia de ojos del modelo le pone su carácter encima. */
    switch (p->familia) {
    case RK_OJOS_FIEROS:
        /* El cresta está enojado aunque esté contento: el párpado cortado
         * hacia la nariz no se va nunca, sólo se acentúa. */
        if (e.cerrado == 0 && !e.cruz && !e.espiral) {
            if (e.g.tapa_sup < 30) { e.g.tapa_sup = 30; }
            e.g.tapa_ang = p->ojo_inclina;
        }
        break;
    case RK_OJOS_PESADOS:
        if (e.cerrado == 0 && !e.cruz && !e.espiral) {
            e.g.tapa_sup = e.g.tapa_sup + 42 > 72 ? 72 : e.g.tapa_sup + 42;
        }
        break;
    case RK_OJOS_RASGADOS:
        e.g.pupila = e.g.pupila * 118 / 100;
        break;
    default:
        break;
    }

    /* 4. El parpadeo, suave: el párpado baja y sube en 180 ms. Con
     * antialiasing se puede animar la altura de a fracciones de pixel, y eso
     * es lo que hace que parezca un parpadeo y no un cambio de cuadro. */
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

/* La expresión del mimo: contento, con los ojos en ^ ^ y las cejas altas.
 * Parte de la cara de contento del mismo modelo, así que conserva lo que la
 * familia de ojos y la boca de cada uno le ponen. */
static expr_t expresion_mimo(const rk_persona_t *p, uint32_t t)
{
    expr_t e = expresion(p, RK_MOOD_HAPPY, rk_look(RK_MOOD_HAPPY), t, 0u);

    e.cerrado = 1;
    e.cruz = false;
    e.espiral = false;
    e.g.tapa_sup = 0;
    e.g.tapa_inf = 0;
    e.g.ceja_dy = 6;
    e.g.ceja_ang = 6;
    e.g.boca_curva = 100;
    if (e.boca != RK_BOCA_SMILE) {
        e.boca = RK_BOCA_SMILE;
    }
    return e;
}

/* Un punto intermedio entre dos expresiones. La geometría se interpola; lo
 * discreto (ojos en cruz, lengua afuera, el estilo de boca) lo pone el que
 * domina, y si difiere, el ojo parpadea justo cuando cambia: el párpado baja
 * hasta cerrarse a mitad de camino y vuelve a abrir. Es lo que hace un
 * animador para esconder un corte, y lo que hace una cara de verdad cuando
 * cambia de idea. */
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

/* Arco grueso con puntas redondas. `abajo` = sonrisa (u), si no, ceño (n). */
static void arco(cara_t *c, int32_t x, int32_t y, int32_t r, int32_t grosor,
                 bool abajo, rk_color_t col)
{
    rk_aa_arco(c->fb, x, y, r, grosor, abajo, col);
}

/* Lágrima: un círculo y un triángulo cuyos lados son tangentes a él. Las
 * tangentes desde un punto a distancia 2r salen a 30 grados, así que tocan el
 * círculo en (±0,866r, -0,5r): con esos vértices la unión no tiene quiebre. */
static void gota(cara_t *c, int32_t x, int32_t y, int32_t r, rk_color_t col)
{
    rk_aa_triangulo(c->fb, x, y - r * 2,
                    x - r * 866 / 1000, y - r / 2,
                    x + r * 866 / 1000, y - r / 2, col, 255);
    circulo(c, x, y, r, col, 255);
    circulo(c, x - r / 3, y - r / 5, r / 3, RK_RGB(255, 255, 255), 190);
}

/* Destello de cuatro puntas: un rombo alto y uno ancho, cruzados. */
static void destello(cara_t *c, int32_t x, int32_t y, int32_t l, rk_color_t col,
                     uint8_t alfa)
{
    rk_aa_rombo(c->fb, x, y, l / 4, l, col, alfa);
    rk_aa_rombo(c->fb, x, y, l, l / 4, col, alfa);
}

/* ---------------------------------------------------------------- ojo --- */
/* Un ojo completo: blanco, iris, pupila, brillos y párpados.
 *
 * El orden importa. Primero el blanco; después pupila y brillos recortados
 * contra el blanco, para que la mirada pueda moverse sin que la pupila se
 * salga del ojo; al final los párpados, pintados del color del fondo encima
 * de todo lo anterior. `lado` es -1 para el ojo izquierdo, +1 para el
 * derecho, 0 para el ojo único. */
static void ojo(cara_t *c, const expr_t *e, int32_t ex, int32_t ey,
                int32_t rx, int32_t ry, int lado)
{
    const rk_persona_t *p = c->p;
    int32_t ryv = ry * e->g.abre / 100;
    int32_t grosor = PX(c, 7);
    int x0 = (int)((ex - rx) / 16) - 2, x1 = (int)((ex + rx) / 16) + 3;
    int y0 = (int)((ey - ryv) / 16) - 2, y1 = (int)((ey + ryv) / 16) + 3;
    rk_forma_t f[3];

    /* --- cerrado: una curva y nada más ---------------------------------- */
    /* El ojo único es tan grande que un arco de su ancho se lee como boca:
     * cerrado, se achica. */
    if (e->cerrado == 1) {                 /* contento: ^ */
        arco(c, ex, ey + ry / 3, lado == 0 ? rx * 55 / 100 : rx * 3 / 4, grosor,
             false, c->trazo);
        return;
    }
    if (e->cerrado == 2) {                 /* dormido: u */
        arco(c, ex, ey - ry / 5, lado == 0 ? rx * 55 / 100 : rx * 3 / 4, grosor,
             true, c->trazo);
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

    /* --- el blanco ------------------------------------------------------ */
    if (p->adornos & RK_ADORNO_ESTATICA) {
        /* El secreto: el ojo se desdobla en cian y magenta, como una imagen
         * mal sintonizada. Es la única cara que no termina de enfocar. */
        int32_t off = PX(c, 2) + (int32_t)(rk_sin8((uint8_t)(c->t / 9u)) * PX(c, 1) / 127);
        f[0] = rk_elipse_q4(ex - off, ey, rx, ryv);
        pintar(c, f, 1, c->iris, 190);
        f[0] = rk_elipse_q4(ex + off, ey, rx, ryv);
        pintar(c, f, 1, c->acento, 190);
    }
    f[0] = rk_elipse_q4(ex, ey, rx, ryv);
    pintar(c, f, 1, c->blanco, 255);

    if (e->espiral) {
        int32_t k;
        for (k = rx * 2 / 3; k > PX(c, 3); k -= PX(c, 7)) {
            f[0] = rk_anillo_q4(ex, ey, k, PX(c, 3));
            f[1] = rk_elipse_q4(ex, ey, rx, ryv);
            pintar_en(c, f, 2, c->trazo, x0, y0, x1, y1);
        }
    } else {
        int32_t pr = (rx < ry ? rx : ry) * 56 / 100 * e->g.pupila / 100;
        int32_t px = ex + (rx - pr) * e->g.mira_x / 140;
        int32_t py = ey + (ryv - pr) * e->g.mira_y / 140;

        /* Iris de color: sólo el cíclope, cuyo único ojo es tan grande que
         * una pupila negra sola se vería vacía. */
        if (lado == 0) {
            f[0] = rk_circulo_q4(px, py, pr * 145 / 100);
            f[1] = rk_elipse_q4(ex, ey, rx, ryv);
            pintar_en(c, f, 2, c->iris, x0, y0, x1, y1);
        }
        f[0] = rk_circulo_q4(px, py, pr);
        f[1] = rk_elipse_q4(ex, ey, rx, ryv);
        pintar_en(c, f, 2, c->trazo, x0, y0, x1, y1);

        /* Dos brillos, siempre en el mismo lugar respecto de la pupila: el
         * grande arriba a la izquierda, el chico abajo a la derecha. Es el
         * detalle que hace que un círculo oscuro sea un ojo húmedo y vivo. */
        f[0] = rk_circulo_q4(px - pr * 36 / 100, py - pr * 38 / 100, pr * 36 / 100);
        pintar_en(c, f, 2, c->blanco, x0, y0, x1, y1);
        f[0] = rk_circulo_q4(px + pr * 36 / 100, py + pr * 34 / 100, pr * 15 / 100);
        pintar_en(c, f, 2, c->blanco, x0, y0, x1, y1);

        if (p->familia == RK_OJOS_RASGADOS) {
            destello(c, px + pr * 10 / 100, py - pr * 5 / 100, pr * 22 / 100,
                     c->blanco, 220);
        }
    }

    /* --- los párpados, del color de la piel ---------------------------- */
    if (e->g.tapa_sup > 0) {
        /* Recta que baja desde el borde de arriba del ojo. La inclinación se
         * espeja según el lado para que "positivo" siempre signifique que la
         * punta de adentro baja: eso se lee como enojo en los dos ojos. */
        int ang = (lado == 0) ? 0 : e->g.tapa_ang * -lado;
        int32_t ly = ey - ryv + (2 * ryv) * e->g.tapa_sup / 100;
        f[0] = rk_semiplano_arriba_q4(ex, ly, ang);
        f[1] = rk_elipse_q4(ex, ey, rx + PX(c, 2), ryv + PX(c, 2));
        pintar_en(c, f, 2, c->bg, x0, y0, x1, y1);
    }
    if (e->g.tapa_inf > 0) {
        int32_t ly = ey + ryv - (2 * ryv) * e->g.tapa_inf / 100;
        f[0] = rk_semiplano_abajo_q4(ex, ly, 0);
        f[1] = rk_elipse_q4(ex, ey, rx + PX(c, 2), ryv + PX(c, 2));
        pintar_en(c, f, 2, c->bg, x0, y0, x1, y1);
    }
}

/* --------------------------------------------------------------- visor --- */
/* El visor no tiene ojos: tiene una franja oscura con dos luces, y la forma
 * de esas luces es toda la expresión. */
static void visor(cara_t *c, const expr_t *e)
{
    int32_t bw = PQ(c, c->p->ojo_rx);
    int32_t bh = PQ(c, c->p->ojo_ry);
    int32_t y = c->oy;
    int lado;

    capsula(c, c->cx - bw + bh, y, c->cx + bw - bh, y, bh, c->trazo);

    for (lado = -1; lado <= 1; lado += 2) {
        int32_t x = c->cx + lado * bw * 46 / 100;
        int32_t r = bh * 30 / 100;
        int32_t alto = bh * 45 / 100;
        rk_color_t glow = c->iris;

        if (e->cruz) {
            capsula(c, x - r, y - r, x + r, y + r, r / 3, glow);
            capsula(c, x + r, y - r, x - r, y + r, r / 3, glow);
            continue;
        }
        if (e->cerrado == 1) {
            arco(c, x, y + r / 2, r, r * 2 / 3, false, glow);
            continue;
        }
        if (e->cerrado == 2) {
            capsula(c, x - r, y, x + r, y, r / 3, glow);
            continue;
        }
        if (e->espiral) {
            rk_forma_t f = rk_anillo_q4(x, y, r, r / 2);
            pintar(c, &f, 1, glow, 255);
            continue;
        }
        /* Luz vertical con núcleo claro. El párpado la achata desde arriba. */
        alto = alto * e->g.abre / 100;
        alto -= alto * e->g.tapa_sup / 100;
        if (e->g.tapa_sup >= 60) {
            capsula(c, x - r, y, x + r, y, r / 3, glow);
            continue;
        }
        {
            int32_t dy = (e->g.mira_y * bh) / 400;
            int32_t dx = (e->g.mira_x * bh) / 400;
            int32_t rr = r * e->g.pupila / 100;
            capsula(c, x + dx, y + dy - alto, x + dx, y + dy + alto, rr, glow);
            capsula(c, x + dx, y + dy - alto / 2, x + dx, y + dy + alto / 2,
                    rr / 2, c->blanco);
        }
    }
}

/* -------------------------------------------------------------- cejas --- */
static void cejas(cara_t *c, const expr_t *e, int32_t ex_izq, int32_t ex_der,
                  int32_t ry)
{
    const rk_persona_t *p = c->p;
    int32_t alto = PQ(c, p->ceja_alto + e->g.ceja_dy);
    int32_t hl = PQ(c, p->ojo_rx) * 72 / 100;
    int32_t r = (p->ceja == RK_CEJA_GRUESA) ? PX(c, 6)
              : (p->ceja == RK_CEJA_DESPEINADA) ? PX(c, 5) : PX(c, 4);
    int ang = p->ceja_angulo + e->g.ceja_ang;
    int lado;

    if (p->ceja == RK_CEJA_NINGUNA || e->cerrado != 0) {
        return;
    }
    for (lado = -1; lado <= 1; lado += 2) {
        int32_t ex = (p->familia == RK_OJOS_UNICO) ? c->cx
                   : (lado < 0 ? ex_izq : ex_der);
        int32_t by = c->oy - ry - alto;
        /* Negativo baja la punta de adentro (enojo); positivo la sube
         * (preocupación). */
        int32_t k = (int32_t)((int64_t)hl * ang / 40);
        int32_t xin = ex - lado * hl, xout = ex + lado * hl;

        if (p->familia == RK_OJOS_UNICO) {
            /* Una sola ceja, larga, arriba del único ojo. */
            if (lado > 0) {
                break;
            }
            hl = PQ(c, p->ojo_rx) * 80 / 100;
            capsula(c, c->cx - hl, by - k / 2, c->cx + hl, by - k / 2, r, c->trazo);
            break;
        }
        if (p->ceja == RK_CEJA_DESPEINADA) {
            /* El mechón: tres cápsulas cortas en abanico. Desplazamientos
             * fijos, no aleatorios, para que no titile entre cuadros. */
            int j;
            for (j = -1; j <= 1; j++) {
                int32_t bx = ex + lado * j * hl * 55 / 100;
                int32_t yin = by - k * j / 2 + (j == 0 ? -PX(c, 2) : 0);
                capsula(c, bx - lado * hl / 3, yin + k / 2,
                           bx + lado * hl / 3, yin - k / 2, r, c->trazo);
            }
        } else {
            capsula(c, xin, by - k, xout, by + k, r, c->trazo);
        }
    }
}

/* --------------------------------------------------------------- boca --- */
/* Una boca curva por los extremos (cx +- w, ye) con flecha proporcional a la
 * curva: hacia abajo si sonríe, hacia arriba si hace mueca. A |curva| = 100
 * es exactamente el arco de rk_aa_arco de radio w (la mitad de un círculo);
 * a curvas más chicas, un arco de círculo más grande por los mismos
 * extremos, que es lo que hace continua la transición entre sonrisa, boca
 * recta y mueca. */
static void boca_curva(cara_t *c, int32_t w, int32_t y, int32_t gr, int curva)
{
    int32_t mag = curva < 0 ? -curva : curva;
    int32_t s = w * mag / 100;                       /* la flecha           */
    int32_t ye = y - (w / 2) * curva / 100;          /* altura de las puntas */
    int64_t R = ((int64_t)w * w + (int64_t)s * s) / (2 * s);
    int32_t yc = curva > 0 ? ye - (int32_t)(R - s) : ye + (int32_t)(R - s);
    int32_t medio = gr / 2;
    int32_t top = curva > 0 ? ye - gr : ye - s - gr;
    int32_t bot = curva > 0 ? ye + s + gr : ye + gr;
    rk_forma_t f[2];

    f[0] = rk_anillo_q4(c->cx, yc, (int32_t)R, gr);
    f[1] = curva > 0 ? rk_semiplano_abajo_q4(c->cx, ye, 0)
                     : rk_semiplano_arriba_q4(c->cx, ye, 0);
    rk_aa_pintar(c->fb, f, 2, c->trazo, 255,
                 (int)((c->cx - w - gr) / 16) - 2, (int)(top / 16) - 2,
                 (int)((c->cx + w + gr) / 16) + 3, (int)(bot / 16) + 3);
    rk_aa_circulo(c->fb, c->cx - w, ye, medio, c->trazo);
    rk_aa_circulo(c->fb, c->cx + w, ye, medio, c->trazo);
}

static void boca(cara_t *c, const expr_t *e)
{
    const rk_persona_t *p = c->p;
    int32_t w = PQ(c, p->boca_ancho);
    int32_t y = c->cy + PQ(c, BOCA_DY);
    int32_t gr = PX(c, 6);
    rk_forma_t f[2];

    if (p->boca == RK_BOCA_NINGUNA) {
        return;
    }

    switch ((rk_boca_t)e->boca) {
    case RK_BOCA_OPEN:
    case RK_BOCA_PANT:
        /* Boca abierta con lengua: una elipse oscura y, adentro, la lengua
         * recortada contra ella. */
        f[0] = rk_elipse_q4(c->cx, y, w * 7 / 10, w * 6 / 10);
        pintar(c, f, 1, c->trazo, 255);
        f[1] = f[0];
        f[0] = rk_elipse_q4(c->cx, y + w * 5 / 10, w * 55 / 100, w * 4 / 10);
        pintar(c, f, 2, COL_LENGUA, 255);
        if (e->boca == RK_BOCA_PANT) {
            /* La lengua afuera, cayendo por debajo del labio. */
            f[0] = rk_elipse_q4(c->cx, y + w * 7 / 10, w * 32 / 100, w * 38 / 100);
            pintar(c, f, 1, COL_LENGUA, 255);
        }
        break;

    case RK_BOCA_WAVY: {
        /* Zigzag de cuatro tramos: temblor o asco, según con qué ojos venga. */
        int32_t paso = w / 2;
        int32_t alto = w / 4;
        int k;
        for (k = 0; k < 4; k++) {
            int32_t xa = c->cx - w + paso * k;
            int32_t ya = (k % 2 == 0) ? y : y - alto;
            int32_t yb = (k % 2 == 0) ? y - alto : y;
            capsula(c, xa, ya, xa + paso, yb, gr / 2, c->trazo);
        }
        break;
    }

    default: {
        /* Sonrisa, recta y mueca son UNA curva, de 100 a -100. Las bocas con
         * forma propia (la de gato, la dentada) valen a partir de media
         * sonrisa; por debajo, la curva genérica; cerca de cero, la recta. */
        int curva = e->g.boca_curva;
        if (curva >= 50 && p->boca == RK_BOCA_GATO) {
            arco(c, c->cx - w / 2, y - w / 3, w / 2, gr, true, c->trazo);
            arco(c, c->cx + w / 2, y - w / 3, w / 2, gr, true, c->trazo);
        } else if (curva >= 50 && p->boca == RK_BOCA_DIENTES) {
            /* Sonrisa grande en D, con una franja de dientes arriba: la
             * mitad de abajo de una elipse, y adentro la parte alta en
             * blanco. */
            int32_t my = y - w / 3;
            f[0] = rk_elipse_q4(c->cx, my, w, w * 85 / 100);
            f[1] = rk_semiplano_abajo_q4(c->cx, my, 0);
            pintar(c, f, 2, c->trazo, 255);
            {
                rk_forma_t d[3];
                d[0] = f[0];
                d[1] = f[1];
                d[2] = rk_semiplano_arriba_q4(c->cx, my + w * 30 / 100, 0);
                pintar(c, d, 3, c->blanco, 255);
                d[2] = rk_elipse_q4(c->cx, my + w * 80 / 100, w * 55 / 100, w * 30 / 100);
                pintar(c, d, 3, COL_LENGUA, 255);
            }
        } else if (curva > -12 && curva < 12) {
            if (p->boca == RK_BOCA_CHICA) {
                circulo(c, c->cx, y, w / 2, c->trazo, 255);
            } else {
                capsula(c, c->cx - w * 2 / 3, y, c->cx + w * 2 / 3, y, gr / 2, c->trazo);
            }
        } else {
            boca_curva(c, w, y, gr, curva);
        }
        break;
    }
    }

    if ((p->adornos & RK_ADORNO_COLMILLO) && e->boca != RK_BOCA_FROWN
        && e->boca != RK_BOCA_SMILE) {
        /* El colmillo: un triangulito blanco que asoma bajo la comisura. En
         * la sonrisa grande no hace falta: ya se ven los dientes. */
        int32_t fx = c->cx + w * 45 / 100;
        rk_aa_triangulo(c->fb, fx - w / 6, y, fx + w / 6, y, fx, y + w * 40 / 100,
                        c->blanco, 255);
    }
}

/* ------------------------------------------------------------ adornos --- */
static void adornos(cara_t *c, const expr_t *e, uint8_t set, int32_t ex_izq,
                    int32_t ex_der, int32_t ry)
{
    int32_t u16 = (int32_t)c->u * 16;
    int i;

    if (set & RK_ADORNO_RUBOR) {
        int32_t by = c->oy + ry + PQ(c, 6);
        rk_forma_t f = rk_elipse_q4(ex_izq - PQ(c, 3), by, PQ(c, 9), PQ(c, 5));
        pintar(c, &f, 1, c->sombra, 200);
        f = rk_elipse_q4(ex_der + PQ(c, 3), by, PQ(c, 9), PQ(c, 5));
        pintar(c, &f, 1, c->sombra, 200);
    }
    if (set & RK_ADORNO_BRILLOS) {
        for (i = 0; i < 3; i++) {
            uint8_t ph = (uint8_t)(c->t / 20u + (uint32_t)i * 85u);
            int32_t r = PQ(c, 40) + (int32_t)rk_sin8(ph) * PQ(c, 3) / 127;
            int32_t x = c->cx + (int32_t)rk_sin8((uint8_t)(i * 85 + 30)) * r / 127;
            int32_t y = c->cy + (int32_t)rk_sin8((uint8_t)(i * 85 + 94)) * r / 127;
            int32_t l = PQ(c, 4) + (rk_sin8(ph) > 0 ? PX(c, 2) : 0);
            destello(c, x, y, l, c->acento, 230);
        }
    }
    if (set & RK_ADORNO_ESPORAS) {
        for (i = 0; i < 7; i++) {
            uint16_t h = rk_hash((uint16_t)(i * 733 + 5));
            int32_t x = (int32_t)(h % (uint16_t)c->u) * 16;
            int32_t sube = (int32_t)((c->t / 22u + (h >> 4)) % 256u);
            int32_t y = u16 - sube * u16 / 256 + (c->cy - u16 / 2);
            circulo(c, x, y, PX(c, 2) + PX(c, i % 2), c->acento, 170);
        }
    }
    if (set & RK_ADORNO_SCANLINE) {
        int32_t y = (int32_t)((c->t / 14u) % (uint32_t)c->fb->h);
        rk_hline(c->fb, 0, y, c->fb->w, rk_mix(c->bg, c->iris, 40));
    }
    if (set & RK_ADORNO_ESTATICA) {
        for (i = 0; i < 4; i++) {
            uint16_t h = rk_hash((uint16_t)(i * 911 + c->t / 110u));
            int y = (int)(h % (uint16_t)c->fb->h);
            rk_fill_rect(c->fb, 0, y, c->fb->w, 1 + (int)((h >> 5) % 2u),
                         rk_mix(c->bg, (i & 1) ? c->iris : c->acento, 36));
        }
    }
    if (set & RK_ADORNO_AURA) {
        /* Un anillo que respira en el borde de la pantalla, del color de
         * acento. Suave a propósito: es un premio, no una alarma. */
        int32_t r = u16 / 2 - PX(c, 5) + (int32_t)rk_sin8((uint8_t)(c->t / 12u)) * PX(c, 2) / 127;
        rk_forma_t f = rk_anillo_q4(c->cx, c->cy, r, PX(c, 5));
        pintar(c, &f, 1, c->acento, 110);
    }
    if (set & RK_ADORNO_CORONA) {
        /* Tres puntas y una base. Es el premio de los 180 días sanos, así que
         * va arriba de todo y en oro: tiene que verse desde la otra punta de
         * la habitación. */
        int32_t y = c->oy - ry - PQ(c, 20);
        int32_t w = PQ(c, 7);
        int k;
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
    if (e->burbujas) {
        for (i = 0; i < 6; i++) {
            uint16_t h = rk_hash((uint16_t)(i * 2654 + 17));
            int32_t x = (int32_t)(h % (uint16_t)c->fb->w) * 16;
            int32_t sube = (int32_t)((c->t / 9u + (h >> 5)) % 256u);
            int32_t y = (int32_t)c->fb->h * 16 - sube * c->fb->h * 16 / 256;
            rk_forma_t f = rk_anillo_q4(x, y, PQ(c, 3) + PX(c, i % 3), PX(c, 2));
            pintar(c, &f, 1, c->blanco, 170);
        }
    }
}

/* ---------------------------------------------------------- accesorios --- */
/* Van después de ojos, cejas y boca, y antes de los adornos: los anteojos
 * tienen que quedar encima de los párpados y los destellos encima de todo. */
static void accesorio(cara_t *c, const rk_persona_t *p, int32_t ex_izq,
                      int32_t ex_der, int32_t rx, int32_t ry)
{
    switch (p->accesorio) {
    case RK_ACC_LENTES: {
        /* Dos aros alrededor de los ojos y un puente. El aro es más grande
         * que el ojo para no taparle el párpado: la expresión la siguen
         * haciendo los ojos. Sólo para modelos de dos ojos. */
        int32_t r = (rx > ry ? rx : ry) + PQ(c, 4);
        int32_t g = PX(c, 4);
        int32_t brillo = r * 6 / 10;
        rk_forma_t f;

        if (p->familia == RK_OJOS_VISOR || p->familia == RK_OJOS_UNICO) {
            return;
        }
        f = rk_anillo_q4(ex_izq, c->oy, r, g);
        pintar(c, &f, 1, c->trazo, 255);
        f = rk_anillo_q4(ex_der, c->oy, r, g);
        pintar(c, &f, 1, c->trazo, 255);
        capsula(c, ex_izq + r, c->oy - PQ(c, 2), ex_der - r, c->oy - PQ(c, 2), g / 2, c->trazo);
        capsula(c, ex_izq - r - PQ(c, 3), c->oy - PQ(c, 3), ex_izq - r, c->oy - PQ(c, 2), g / 2, c->trazo);
        capsula(c, ex_der + r, c->oy - PQ(c, 2), ex_der + r + PQ(c, 3), c->oy - PQ(c, 3), g / 2, c->trazo);
        /* Un reflejo en cada vidrio, arriba a la derecha. */
        f = rk_capsula_q4(ex_izq + brillo / 3, c->oy - brillo, ex_izq + brillo, c->oy - brillo / 3, PX(c, 2));
        pintar(c, &f, 1, c->blanco, 120);
        f = rk_capsula_q4(ex_der + brillo / 3, c->oy - brillo, ex_der + brillo, c->oy - brillo / 3, PX(c, 2));
        pintar(c, &f, 1, c->blanco, 120);
        break;
    }
    case RK_ACC_CURITA: {
        /* Una curita cruzada en el cachete derecho, con su gasa al medio. */
        int32_t x = ex_der + PQ(c, 10);
        int32_t y = c->oy + ry + PQ(c, 6);
        capsula(c, x - PQ(c, 8), y + PQ(c, 3), x + PQ(c, 8), y - PQ(c, 3), PQ(c, 3), COL_CURITA);
        capsula(c, x - PQ(c, 2), y + PQ(c, 1), x + PQ(c, 2), y - PQ(c, 1), PQ(c, 2), COL_GASA);
        circulo(c, x - PQ(c, 5), y + PQ(c, 2), PX(c, 2), COL_GASA, 255);
        circulo(c, x + PQ(c, 5), y - PQ(c, 2), PX(c, 2), COL_GASA, 255);
        break;
    }
    default:
        break;
    }
}

/* ---------------------------------------------------------------- cara --- */
/* La respiración mueve la cara entera, de a fracciones de pixel. Con
 * antialiasing eso se ve como un vaivén suave; sin él, como un salto. */
static int32_t respiracion(const cara_t *c, const rk_look_t *lk, uint32_t t)
{
    return lk->bob_amp
        ? (int32_t)rk_sin8((uint8_t)(t * lk->bob_speed / 1000u)) * PX(c, lk->bob_amp) / 127
        : 0;
}

/* `desde` y `hacia` son el mismo ánimo salvo durante una transición; `t_pct`
 * dice en qué punto de ella estamos. Con desde == hacia o t_pct == 100 el
 * camino es exactamente el de una cara sola. */
static int acotar(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void dibujar(rk_fb_t *fb, const rk_persona_t *p, rk_mood_t desde,
                    rk_mood_t hacia, uint8_t t_pct, rk_severity_t sev,
                    uint8_t adornos_extra, uint8_t cierre, uint8_t mimo_pct,
                    const rk_face_mirada_t *mirada, uint32_t t)
{
    const rk_look_t *lk = rk_look(hacia);
    const rk_look_t *lka = rk_look(desde);
    bool mezcla = desde != hacia && t_pct < 100u;
    uint8_t t255 = (uint8_t)((uint32_t)t_pct * 255u / 100u);
    expr_t e;
    cara_t c;
    int32_t rx, ry, dx, bob, temblor;

    (void)sev;
    if (fb == NULL || fb->px == NULL) {
        return;
    }
    if (p == NULL) {
        p = rk_persona_at(0);
    }
    if (mezcla && t_pct == 0u) {
        /* El principio es exactamente el ánimo de origen. */
        lk = lka;
        mezcla = false;
        hacia = desde;
    }

    c.fb = fb;
    c.p = p;
    c.t = t;
    c.u = fb->w < fb->h ? fb->w : fb->h;
    c.bg     = tratar(p->fondo, lk);
    c.sombra = tratar(p->sombra, lk);
    c.trazo  = tratar(p->trazo, lk);
    c.blanco = tratar(p->blanco, lk);
    c.iris   = tratar(p->iris, lk);
    c.acento = tratar(p->acento, lk);
    if (mezcla) {
        /* Tinte y penumbra se funden entre los dos ánimos. */
        c.bg     = rk_mix(tratar(p->fondo, lka),  c.bg,     t255);
        c.sombra = rk_mix(tratar(p->sombra, lka), c.sombra, t255);
        c.trazo  = rk_mix(tratar(p->trazo, lka),  c.trazo,  t255);
        c.blanco = rk_mix(tratar(p->blanco, lka), c.blanco, t255);
        c.iris   = rk_mix(tratar(p->iris, lka),   c.iris,   t255);
        c.acento = rk_mix(tratar(p->acento, lka), c.acento, t255);
    }
    if (mimo_pct > 0u) {
        /* El mimo trae los colores de contento: la penumbra de una cara
         * triste se levanta mientras la acarician. */
        const rk_look_t *lkm = rk_look(RK_MOOD_HAPPY);
        uint8_t m255 = (uint8_t)((uint32_t)mimo_pct * 255u / 100u);
        c.bg     = rk_mix(c.bg,     tratar(p->fondo, lkm),  m255);
        c.sombra = rk_mix(c.sombra, tratar(p->sombra, lkm), m255);
        c.trazo  = rk_mix(c.trazo,  tratar(p->trazo, lkm),  m255);
        c.blanco = rk_mix(c.blanco, tratar(p->blanco, lkm), m255);
        c.iris   = rk_mix(c.iris,   tratar(p->iris, lkm),   m255);
        c.acento = rk_mix(c.acento, tratar(p->acento, lkm), m255);
    }

    rk_fb_clear(fb, c.bg);

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
    c.oy = c.cy + PQ(&c, OJOS_DY);

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
        /* La mirada dirigida se suma a la del ánimo (que ya deriva sola). */
        if (e.cerrado == 0 && !e.cruz && !e.espiral) {
            e.g.mira_x = acotar(e.g.mira_x + mirada->mira_x, -100, 100);
            e.g.mira_y = acotar(e.g.mira_y + mirada->mira_y, -100, 100);
        }
        if (mirada->preocupado > 0u) {
            int k = mirada->preocupado > 100u ? 100 : (int)mirada->preocupado;
            /* Cejas altas por el lado de adentro: preocupación. Y la sonrisa
             * se afloja: no se puede sonreír mirando a un vecino con sed. */
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

    /* El aura va detrás de todo. */
    if ((uint8_t)(p->adornos | adornos_extra) & RK_ADORNO_AURA) {
        adornos(&c, &e, RK_ADORNO_AURA, c.cx - dx, c.cx + dx, ry);
    }

    switch (p->familia) {
    case RK_OJOS_VISOR:
        visor(&c, &e);
        break;
    case RK_OJOS_UNICO:
        ojo(&c, &e, c.cx, c.oy, rx, ry, 0);
        break;
    default:
        ojo(&c, &e, c.cx - dx, c.oy, rx, ry, -1);
        ojo(&c, &e, c.cx + dx, c.oy, rx, ry, +1);
        break;
    }

    cejas(&c, &e, c.cx - dx, c.cx + dx, ry);
    boca(&c, &e);
    accesorio(&c, p, c.cx - dx, c.cx + dx, rx, ry);
    adornos(&c, &e,
            (uint8_t)((p->adornos | adornos_extra) & (uint8_t)~RK_ADORNO_AURA),
            c.cx - dx, c.cx + dx, ry);
}

void rk_face_draw(rk_fb_t *fb, const rk_persona_t *p, rk_mood_t mood,
                  rk_severity_t sev, uint8_t adornos_extra, uint32_t t_ms)
{
    dibujar(fb, p, mood, mood, 100u, sev, adornos_extra, 0u, 0u, NULL, t_ms);
}

void rk_face_draw_cierre(rk_fb_t *fb, const rk_persona_t *p, rk_mood_t mood,
                         rk_severity_t sev, uint8_t adornos_extra,
                         uint8_t cierre, uint32_t t_ms)
{
    dibujar(fb, p, mood, mood, 100u, sev, adornos_extra,
            cierre > 100u ? 100u : cierre, 0u, NULL, t_ms);
}

void rk_face_draw_mezcla(rk_fb_t *fb, const rk_persona_t *p,
                         rk_mood_t desde, rk_mood_t hacia, uint8_t t_pct,
                         rk_severity_t sev, uint8_t adornos_extra,
                         uint8_t cierre, uint32_t t_ms)
{
    dibujar(fb, p, desde, hacia, t_pct > 100u ? 100u : t_pct, sev, adornos_extra,
            cierre > 100u ? 100u : cierre, 0u, NULL, t_ms);
}

void rk_face_draw_mimo(rk_fb_t *fb, const rk_persona_t *p,
                       rk_mood_t mood, uint8_t mimo_pct,
                       rk_severity_t sev, uint8_t adornos_extra,
                       uint32_t t_ms)
{
    dibujar(fb, p, mood, mood, 100u, sev, adornos_extra, 0u,
            mimo_pct > 100u ? 100u : mimo_pct, NULL, t_ms);
}

void rk_face_draw_mirada(rk_fb_t *fb, const rk_persona_t *p,
                         rk_mood_t mood, rk_severity_t sev,
                         uint8_t adornos_extra, const rk_face_mirada_t *m,
                         uint32_t t_ms)
{
    rk_face_mirada_t mm;
    if (m == NULL) {
        rk_face_draw(fb, p, mood, sev, adornos_extra, t_ms);
        return;
    }
    mm.mira_x = acotar(m->mira_x, -100, 100);
    mm.mira_y = acotar(m->mira_y, -100, 100);
    mm.preocupado = m->preocupado > 100u ? 100u : m->preocupado;
    dibujar(fb, p, mood, mood, 100u, sev, adornos_extra, 0u, 0u, &mm, t_ms);
}

uint8_t rk_face_adornos_etapa(int etapa)
{
    /* El modelo te toca por azar; cómo se ve se gana cuidando la planta. */
    if (etapa >= 4) { return RK_ADORNO_AURA | RK_ADORNO_CORONA; }
    if (etapa >= 3) { return RK_ADORNO_AURA; }
    if (etapa >= 2) { return RK_ADORNO_BRILLOS; }
    return 0u;
}
