#include "face.h"
#include "look.h"
#include "../gfx/aa.h"
#include <stddef.h>

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
 * familia de ojos del modelo y por el parpadeo del instante. */
typedef struct {
    int  abre;        /* apertura vertical del ojo, 100 = normal           */
    int  pupila;      /* tamaño de pupila, 100 = normal                    */
    int  tapa_sup;    /* cuánto cubre el párpado de arriba, 0..100         */
    int  tapa_ang;    /* inclinación del párpado (unidades de rk_sin8)     */
    int  tapa_inf;    /* cuánto sube el párpado de abajo, 0..100           */
    int  cerrado;     /* 0 abierto, 1 cerrado contento (^), 2 dormido (u)  */
    bool cruz;
    bool espiral;
    int  mira_x;      /* hacia dónde mira la pupila, -100..100             */
    int  mira_y;
    int  ceja_ang;    /* se suma a la inclinación de fábrica               */
    int  ceja_dy;     /* sube (+) o baja (-) las cejas, centésimas de U    */
    int  boca;        /* rk_boca_t                                         */
    bool gota;
    bool zzz;
    bool burbujas;
} expr_t;

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
    expr_t e = { 100, 100, 0, 0, 0, 0, false, false, 0, 0, 0, 0,
                 RK_BOCA_FLAT, false, false, false };

    e.boca = lk->boca;

    /* 1. Lo que pide el ánimo, en abstracto. */
    switch ((rk_ojo_t)lk->ojo) {
    case RK_OJO_BLINK:  e.cerrado = 2;                          break;
    case RK_OJO_HAPPY:  e.cerrado = 1;                          break;
    case RK_OJO_WIDE:   e.abre = 112; e.pupila = 62;            break;
    case RK_OJO_SLEEPY: e.tapa_sup = 44; e.tapa_ang = -10;      break;
    case RK_OJO_DEAD:   e.cruz = true;                          break;
    case RK_OJO_DIZZY:  e.espiral = true;                       break;
    case RK_OJO_GLITCH: e.tapa_sup = 30; e.pupila = 80;         break;
    default:                                                    break;
    }

    /* 2. Los gestos propios de cada ánimo, que no son sólo ojos. */
    switch (mood) {
    case RK_MOOD_THIRSTY:
        e.gota = true; e.mira_y = 45; e.ceja_ang = 10; break;
    case RK_MOOD_HOT:
        e.gota = true; e.ceja_ang = 8;                break;
    case RK_MOOD_COLD:
        e.ceja_ang = 12; e.ceja_dy = 3; e.tapa_inf = 22; break;
    case RK_MOOD_DROWNING:
        e.burbujas = true; e.ceja_dy = 5; e.ceja_ang = 10; break;
    case RK_MOOD_DARK:
        e.ceja_dy = 4; e.ceja_ang = 8; e.mira_x = 55;  break;
    case RK_MOOD_SLEEPING:
        e.zzz = true;                                  break;
    case RK_MOOD_UNKNOWN:
        e.mira_y = -60; e.mira_x = -30; e.ceja_dy = 4; break;
    case RK_MOOD_PARCHED_AIR:
        e.tapa_inf = 30; e.tapa_sup = 20;              break;
    case RK_MOOD_SCORCHED:
        e.ceja_ang = 12;                               break;
    case RK_MOOD_HAPPY:
        /* Contento con los ojos abiertos, y cada tanto el gesto de alegría:
         * los ojos se cierran en ^ durante un segundo. Siempre en ^ sería
         * una cara sin mirada; nunca, una cara sin alegría. */
        e.ceja_dy = 2; e.ceja_ang = 4;
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
            if (e.tapa_sup < 30) { e.tapa_sup = 30; }
            e.tapa_ang = p->ojo_inclina;
        }
        break;
    case RK_OJOS_PESADOS:
        if (e.cerrado == 0 && !e.cruz && !e.espiral) {
            e.tapa_sup = e.tapa_sup + 42 > 72 ? 72 : e.tapa_sup + 42;
        }
        break;
    case RK_OJOS_RASGADOS:
        e.pupila = e.pupila * 118 / 100;
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
            if (k > e.tapa_sup) {
                e.tapa_sup = k;
            }
        }
    }

    /* 5. El cierre forzado del despertar. */
    if (cierre > 0u && e.cerrado == 0) {
        int k = e.tapa_sup + cierre;
        e.tapa_sup = k > 100 ? 100 : k;
    }
    if (e.tapa_sup >= 96) {
        e.cerrado = 2;
    }

    /* 6. La mirada deriva sola, despacio. Un ojo perfectamente quieto se ve
     * de muñeco; uno que se mueve un poco se ve atento. */
    if (e.cerrado == 0 && !e.cruz && !e.espiral && mood != RK_MOOD_UNKNOWN) {
        e.mira_x += rk_sin8((uint8_t)(t / 60u)) * 22 / 127;
        e.mira_y += rk_sin8((uint8_t)(t / 97u + 40u)) * 10 / 127;
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
    int32_t ryv = ry * e->abre / 100;
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
        int32_t pr = (rx < ry ? rx : ry) * 56 / 100 * e->pupila / 100;
        int32_t px = ex + (rx - pr) * e->mira_x / 140;
        int32_t py = ey + (ryv - pr) * e->mira_y / 140;

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
    if (e->tapa_sup > 0) {
        /* Recta que baja desde el borde de arriba del ojo. La inclinación se
         * espeja según el lado para que "positivo" siempre signifique que la
         * punta de adentro baja: eso se lee como enojo en los dos ojos. */
        int ang = (lado == 0) ? 0 : e->tapa_ang * -lado;
        int32_t ly = ey - ryv + (2 * ryv) * e->tapa_sup / 100;
        f[0] = rk_semiplano_arriba_q4(ex, ly, ang);
        f[1] = rk_elipse_q4(ex, ey, rx + PX(c, 2), ryv + PX(c, 2));
        pintar_en(c, f, 2, c->bg, x0, y0, x1, y1);
    }
    if (e->tapa_inf > 0) {
        int32_t ly = ey + ryv - (2 * ryv) * e->tapa_inf / 100;
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
        alto = alto * e->abre / 100;
        alto -= alto * e->tapa_sup / 100;
        if (e->tapa_sup >= 60) {
            capsula(c, x - r, y, x + r, y, r / 3, glow);
            continue;
        }
        {
            int32_t dy = (e->mira_y * bh) / 400;
            int32_t dx = (e->mira_x * bh) / 400;
            int32_t rr = r * e->pupila / 100;
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
    int32_t alto = PQ(c, p->ceja_alto + e->ceja_dy);
    int32_t hl = PQ(c, p->ojo_rx) * 72 / 100;
    int32_t r = (p->ceja == RK_CEJA_GRUESA) ? PX(c, 6)
              : (p->ceja == RK_CEJA_DESPEINADA) ? PX(c, 5) : PX(c, 4);
    int ang = p->ceja_angulo + e->ceja_ang;
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

    case RK_BOCA_SMILE:
        if (p->boca == RK_BOCA_GATO) {
            arco(c, c->cx - w / 2, y - w / 3, w / 2, gr, true, c->trazo);
            arco(c, c->cx + w / 2, y - w / 3, w / 2, gr, true, c->trazo);
        } else if (p->boca == RK_BOCA_DIENTES) {
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
        } else {
            arco(c, c->cx, y - w / 2, w, gr, true, c->trazo);
        }
        break;

    case RK_BOCA_FROWN:
        arco(c, c->cx, y + w / 2, w, gr, false, c->trazo);
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

    default:  /* RK_BOCA_FLAT */
        if (p->boca == RK_BOCA_CHICA) {
            circulo(c, c->cx, y, w / 2, c->trazo, 255);
        } else {
            capsula(c, c->cx - w * 2 / 3, y, c->cx + w * 2 / 3, y, gr / 2, c->trazo);
        }
        break;
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

/* ---------------------------------------------------------------- cara --- */
static void dibujar(rk_fb_t *fb, const rk_persona_t *p, rk_mood_t mood,
                    rk_severity_t sev, uint8_t adornos_extra, uint8_t cierre,
                    uint32_t t)
{
    const rk_look_t *lk = rk_look(mood);
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

    rk_fb_clear(fb, c.bg);

    /* La respiración mueve la cara entera, de a fracciones de pixel. Con
     * antialiasing eso se ve como un vaivén suave; sin él, como un salto. */
    bob = lk->bob_amp
        ? (int32_t)rk_sin8((uint8_t)(t * lk->bob_speed / 1000u)) * PX(&c, lk->bob_amp) / 127
        : 0;
    temblor = lk->shiver ? (((t / 60u) % 2u) ? PX(&c, 2) : -PX(&c, 2)) : 0;

    c.cx = (int32_t)fb->w * 8 + temblor;
    c.cy = (int32_t)fb->h * 8 + bob;
    c.oy = c.cy + PQ(&c, OJOS_DY);

    e = expresion(p, mood, lk, t, cierre);

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
    adornos(&c, &e,
            (uint8_t)((p->adornos | adornos_extra) & (uint8_t)~RK_ADORNO_AURA),
            c.cx - dx, c.cx + dx, ry);
}

void rk_face_draw(rk_fb_t *fb, const rk_persona_t *p, rk_mood_t mood,
                  rk_severity_t sev, uint8_t adornos_extra, uint32_t t_ms)
{
    dibujar(fb, p, mood, sev, adornos_extra, 0u, t_ms);
}

void rk_face_draw_cierre(rk_fb_t *fb, const rk_persona_t *p, rk_mood_t mood,
                         rk_severity_t sev, uint8_t adornos_extra,
                         uint8_t cierre, uint32_t t_ms)
{
    dibujar(fb, p, mood, sev, adornos_extra, cierre > 100u ? 100u : cierre, t_ms);
}

uint8_t rk_face_adornos_etapa(int etapa)
{
    /* El modelo te toca por azar; cómo se ve se gana cuidando la planta. */
    if (etapa >= 4) { return RK_ADORNO_AURA | RK_ADORNO_CORONA; }
    if (etapa >= 3) { return RK_ADORNO_AURA; }
    if (etapa >= 2) { return RK_ADORNO_BRILLOS; }
    return 0u;
}
