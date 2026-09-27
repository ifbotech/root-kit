/* Kip, dibujado con la lámina de Rocío (art/kip.c).
 *
 * Esta suite no mira pixeles —eso lo hace la regresión visual de
 * test_render—: mira la animación. Protege las dos promesas de su lámina:
 *
 *   1. QUE SEA FLUIDA. Ningún ciclo salta: ni entre clave y clave, ni al
 *      dar la vuelta, ni en la transición de un ánimo a cualquier otro. La
 *      boca sólo cambia de forma cerrada. Las cejas saltan, se pasan un poco
 *      y vuelven.
 *   2. QUE SEA SU LÁMINA. Cada ánimo que ella dibujó tiene en su pista lo
 *      que tiene en el papel: el guiño, la boqueada bajo el agua, la lengua
 *      afuera del calor, la mirada que busca la luz, las rendijas de reojo.
 */
#include <stddef.h>
#include "rk_test.h"
#include "../art/kip.h"
#include "../art/face.h"
#include "../core/persona.h"

#define PASO_MS 10u

static int absi(int v)
{
    return v < 0 ? -v : v;
}

static void pose(rk_kip_pose_t *p, rk_mood_t desde, rk_mood_t hacia, uint8_t mezcla,
                 uint32_t t)
{
    rk_kip_escena_t e;
    memset(&e, 0, sizeof e);
    e.desde = desde;
    e.hacia = hacia;
    e.mezcla = mezcla;
    e.t_ms = t;
    rk_kip_pose(p, &e);
}

static bool cerrado(const rk_kip_ojo_t *o)
{
    return o->a + o->b <= 0;
}

/* Lo más que se movió cada parte entre dos poses. */
typedef struct {
    int ojo, ceja, boca, mira, fx;
    bool estilo_abierto;    /* la boca cambió de forma estando abierta */
} salto_t;

static void medir(salto_t *s, const rk_kip_pose_t *a, const rk_kip_pose_t *b)
{
    int i, k;

    memset(s, 0, sizeof *s);
    for (i = 0; i < 2; i++) {
        const rk_kip_ojo_t *x = &a->ojo[i], *y = &b->ojo[i];
        const rk_kip_ceja_t *c = &a->ceja[i], *d = &b->ceja[i];
        int v[] = { x->a - y->a, x->b - y->b, x->incl - y->incl, x->dy - y->dy };
        int w[] = { c->dy - d->dy, c->ang - d->ang, c->arco - d->arco, c->grosor - d->grosor };
        for (k = 0; k < 4; k++) {
            if (absi(v[k]) > s->ojo) { s->ojo = absi(v[k]); }
            if (absi(w[k]) > s->ceja) { s->ceja = absi(w[k]); }
        }
    }
    {
        int v[] = { a->boca.abre - b->boca.abre, a->boca.curva - b->boca.curva,
                    a->boca.ancho - b->boca.ancho, a->boca.dy - b->boca.dy };
        for (k = 0; k < 4; k++) {
            if (absi(v[k]) > s->boca) { s->boca = absi(v[k]); }
        }
    }
    s->mira = absi(a->mira_x - b->mira_x) > absi(a->mira_y - b->mira_y)
            ? absi(a->mira_x - b->mira_x) : absi(a->mira_y - b->mira_y);
    for (k = 0; k < RK_KIP_FX_COUNT; k++) {
        if (absi(a->fx[k] - b->fx[k]) > s->fx) { s->fx = absi(a->fx[k] - b->fx[k]); }
    }
    s->estilo_abierto = a->boca.estilo != b->boca.estilo
                     && (a->boca.abre > 15 || b->boca.abre > 15);
}

/* ------------------------------------------------------- fluidez --- */
/* Cada ánimo, dos vueltas enteras de a 10 ms. Los topes son generosos a
 * propósito —un parpadeo es rápido y una mirada salta—: lo que buscan es
 * un corte, que es de otro orden (una clave que no empalma con la
 * siguiente, un ciclo que no vuelve a donde empezó). */
static void test_ciclos_fluidos(void)
{
    int m;
    char lbl[112];

    for (m = 0; m < RK_MOOD_COUNT; m++) {
        uint32_t per = rk_kip_periodo((rk_mood_t)m), t;
        rk_kip_pose_t a, b;
        salto_t peor, s;
        bool boca_ok = true;

        memset(&peor, 0, sizeof peor);
        pose(&a, (rk_mood_t)m, (rk_mood_t)m, 100u, 0u);
        for (t = PASO_MS; t <= 2u * per; t += PASO_MS) {
            pose(&b, (rk_mood_t)m, (rk_mood_t)m, 100u, t);
            medir(&s, &a, &b);
            if (s.ojo > peor.ojo) { peor.ojo = s.ojo; }
            if (s.ceja > peor.ceja) { peor.ceja = s.ceja; }
            if (s.boca > peor.boca) { peor.boca = s.boca; }
            if (s.mira > peor.mira) { peor.mira = s.mira; }
            if (s.estilo_abierto) { boca_ok = false; }
            a = b;
        }
        snprintf(lbl, sizeof lbl, "%s: los ojos no saltan (%d en 10 ms)", rk_mood_name((rk_mood_t)m), peor.ojo);
        CHECK_TRUE(lbl, peor.ojo <= 420);
        snprintf(lbl, sizeof lbl, "%s: las cejas no saltan (%d en 10 ms)", rk_mood_name((rk_mood_t)m), peor.ceja);
        CHECK_TRUE(lbl, peor.ceja <= 60);
        snprintf(lbl, sizeof lbl, "%s: la boca no salta (%d en 10 ms)", rk_mood_name((rk_mood_t)m), peor.boca);
        CHECK_TRUE(lbl, peor.boca <= 45);
        snprintf(lbl, sizeof lbl, "%s: la mirada no se teletransporta (%d en 10 ms)", rk_mood_name((rk_mood_t)m), peor.mira);
        CHECK_TRUE(lbl, peor.mira <= 1000);
        snprintf(lbl, sizeof lbl, "%s: la boca cambia de forma solo cerrada", rk_mood_name((rk_mood_t)m));
        CHECK_TRUE(lbl, boca_ok);
        snprintf(lbl, sizeof lbl, "%s: el ciclo vuelve a donde empezo", rk_mood_name((rk_mood_t)m));
        {
            rk_kip_pose_t x, y;
            pose(&x, (rk_mood_t)m, (rk_mood_t)m, 100u, 5u);
            pose(&y, (rk_mood_t)m, (rk_mood_t)m, 100u, per + 5u);
            CHECK_TRUE(lbl, memcmp(&x.ojo, &y.ojo, sizeof x.ojo) == 0 &&
                            memcmp(&x.boca, &y.boca, sizeof x.boca) == 0);
        }
    }
}

/* De cualquier ánimo a cualquier otro, de a un por ciento. */
static void test_transiciones_fluidas(void)
{
    int a, b, k, malas = 0, abiertas = 0;
    char lbl[112];

    for (a = 0; a < RK_MOOD_COUNT; a++) {
        for (b = 0; b < RK_MOOD_COUNT; b++) {
            rk_kip_pose_t p, q;
            pose(&p, (rk_mood_t)a, (rk_mood_t)b, 0u, 1200u);
            for (k = 1; k <= 100; k++) {
                salto_t s;
                pose(&q, (rk_mood_t)a, (rk_mood_t)b, (uint8_t)k, 1200u);
                medir(&s, &p, &q);
                if (s.ojo > 60 || s.ceja > 45 || s.boca > 25 || s.mira > 40 || s.fx > 20) {
                    malas++;
                    if (malas < 4) {
                        snprintf(lbl, sizeof lbl, "%s -> %s salta en %d%%", rk_mood_name((rk_mood_t)a),
                                 rk_mood_name((rk_mood_t)b), k);
                        rk_t_fail(lbl, "la transicion tiene un corte");
                    }
                }
                if (s.estilo_abierto) {
                    abiertas++;
                }
                p = q;
            }
        }
    }
    CHECK_INT("ninguna transicion entre animos tiene un corte", 0, malas);
    CHECK_INT("y en ninguna la boca cambia de forma abierta", 0, abiertas);

    /* Los extremos son los ánimos puros. */
    {
        rk_kip_pose_t p, q;
        pose(&p, RK_MOOD_HAPPY, RK_MOOD_DROWNING, 100u, 700u);
        pose(&q, RK_MOOD_DROWNING, RK_MOOD_DROWNING, 100u, 700u);
        CHECK_TRUE("en 100 es el destino", memcmp(&p, &q, sizeof p) == 0);
        pose(&p, RK_MOOD_HAPPY, RK_MOOD_DROWNING, 50u, 700u);
        CHECK_TRUE("a mitad de camino el agua va por la mitad",
                   p.fx[RK_KIP_FX_AGUA] > 300 && p.fx[RK_KIP_FX_AGUA] < 700);
    }
}

/* ------------------------------------------------ las cejas saltan --- */
static void test_cejas_saltonas(void)
{
    uint32_t t;
    int minimo = 0, final;
    rk_kip_pose_t p;

    /* En el guiño la ceja derecha sube de 0 a -70: tiene que pasarse de
     * largo y volver, no llegar justo. */
    for (t = 5300u; t <= 5800u; t += 5u) {
        pose(&p, RK_MOOD_HAPPY, RK_MOOD_HAPPY, 100u, t);
        if (p.ceja[1].dy < minimo) {
            minimo = p.ceja[1].dy;
        }
    }
    pose(&p, RK_MOOD_HAPPY, RK_MOOD_HAPPY, 100u, 6500u);
    final = p.ceja[1].dy;
    CHECK_TRUE("la ceja del guino se pasa de largo al saltar", minimo < final - 3);
    CHECK_NEAR("y vuelve a su lugar", -70, final, 3);

    /* Los cejazos: en contento, a los dos segundos las dos suben un
     * instante y bajan. */
    {
        rk_kip_pose_t antes, arriba, despues;
        pose(&antes, RK_MOOD_HAPPY, RK_MOOD_HAPPY, 100u, 1990u);
        pose(&arriba, RK_MOOD_HAPPY, RK_MOOD_HAPPY, 100u, 2150u);
        pose(&despues, RK_MOOD_HAPPY, RK_MOOD_HAPPY, 100u, 2600u);
        CHECK_TRUE("en un cejazo las cejas suben",
                   arriba.ceja[0].dy < antes.ceja[0].dy - 30 && arriba.ceja[1].dy < antes.ceja[1].dy - 30);
        CHECK_TRUE("y despues bajan", absi(despues.ceja[0].dy - antes.ceja[0].dy) <= 2);
    }
}

/* --------------------------------------------- la lámina, en la pista --- */
typedef struct {
    bool guino, teal, dientes, linea, grito_grande, lengua, o, beso, onda, rojo;
    bool siempre_agua, siempre_sumergida, siempre_mira_abajo, siempre_cerrados;
    bool cerrado_alguna_vez;
    int abre_min, abre_max, mira_x_min, mira_x_max, mira_y_max, a_max, rendija_max;
    int tupido_min;
} recorrido_t;

static void recorrer(recorrido_t *r, rk_mood_t m)
{
    uint32_t per = rk_kip_periodo(m), t;

    memset(r, 0, sizeof *r);
    r->siempre_agua = r->siempre_sumergida = r->siempre_mira_abajo = r->siempre_cerrados = true;
    r->abre_min = 10000;
    r->mira_x_min = 10000;
    r->mira_x_max = -10000;
    r->mira_y_max = -10000;
    r->a_max = -10000;
    r->tupido_min = 10000;
    for (t = 0u; t < per; t += PASO_MS) {
        rk_kip_pose_t p;
        const rk_kip_boca_t *b = &p.boca;
        int i;
        pose(&p, m, m, 100u, t);
        if (cerrado(&p.ojo[0]) != cerrado(&p.ojo[1])) { r->guino = true; }
        if (cerrado(&p.ojo[0]) || cerrado(&p.ojo[1])) { r->cerrado_alguna_vez = true; }
        if (!cerrado(&p.ojo[0]) || !cerrado(&p.ojo[1])) { r->siempre_cerrados = false; }
        if (b->estilo == RK_KIP_BOCA_TEAL && b->abre >= 80) { r->teal = true; }
        if (b->estilo == RK_KIP_BOCA_DIENTES && b->abre >= 40) { r->dientes = true; }
        if (b->estilo == RK_KIP_BOCA_LINEA) { r->linea = true; }
        if (b->estilo == RK_KIP_BOCA_GRITO && b->abre >= 180) { r->grito_grande = true; }
        if (b->estilo == RK_KIP_BOCA_GRITO && b->lengua >= 700) { r->lengua = true; }
        if (b->estilo == RK_KIP_BOCA_O && b->abre >= 60) { r->o = true; }
        if (b->estilo == RK_KIP_BOCA_BESO && b->abre >= 60) { r->beso = true; }
        if (b->estilo == RK_KIP_BOCA_ONDA) { r->onda = true; }
        if (p.ojo[0].rojo >= 900) { r->rojo = true; }
        if (p.fx[RK_KIP_FX_AGUA] < 1000) { r->siempre_agua = false; }
        if (b->estilo != RK_KIP_BOCA_SUMERGIDA) { r->siempre_sumergida = false; }
        if (p.mira_y < 500) { r->siempre_mira_abajo = false; }
        if (b->abre < r->abre_min) { r->abre_min = b->abre; }
        if (b->abre > r->abre_max) { r->abre_max = b->abre; }
        if (p.mira_x < r->mira_x_min) { r->mira_x_min = p.mira_x; }
        if (p.mira_x > r->mira_x_max) { r->mira_x_max = p.mira_x; }
        if (p.mira_y > r->mira_y_max) { r->mira_y_max = p.mira_y; }
        for (i = 0; i < 2; i++) {
            if (!cerrado(&p.ojo[i])) {
                if (p.ojo[i].a > r->a_max) { r->a_max = p.ojo[i].a; }
                if (p.ojo[i].a + p.ojo[i].b > r->rendija_max) {
                    r->rendija_max = p.ojo[i].a + p.ojo[i].b;
                }
            }
            if (p.ceja[i].tupido < r->tupido_min) { r->tupido_min = p.ceja[i].tupido; }
        }
    }
}

static void test_la_lamina_esta_en_la_pista(void)
{
    recorrido_t r;

    /* Feliz, cuadros 1 a 10. */
    recorrer(&r, RK_MOOD_HAPPY);
    CHECK_TRUE("contento: guina un ojo (feliz 4 y 5)", r.guino);
    CHECK_TRUE("contento: la sonrisa teal (feliz 5 y 10)", r.teal);
    CHECK_TRUE("contento: la sonrisa con dientes (feliz 9)", r.dientes);
    CHECK_TRUE("contento: la sonrisa de linea (feliz 3, 4 y 8)", r.linea);

    /* El agua. */
    recorrer(&r, RK_MOOD_DROWNING);
    CHECK_TRUE("se ahoga: el agua le llega a los ojos todo el ciclo", r.siempre_agua);
    CHECK_TRUE("se ahoga: la boca queda abajo del agua", r.siempre_sumergida);
    CHECK_TRUE("se ahoga: boquea, la boca se abre", r.abre_max >= 180);
    CHECK_TRUE("se ahoga: y se cierra", r.abre_min <= 5);
    CHECK_TRUE("se ahoga: mira el agua todo el tiempo", r.siempre_mira_abajo);

    /* El calor, cuadros 1 a 9. */
    recorrer(&r, RK_MOOD_HOT);
    CHECK_TRUE("calor: la lengua afuera (calor 1)", r.lengua);
    CHECK_TRUE("calor: el jadeo grande (calor 3 y 4)", r.grito_grande);
    CHECK_TRUE("calor: cierra los ojos (calor 3)", r.cerrado_alguna_vez);
    CHECK_TRUE("calor: los ojos irritados (calor 4)", r.rojo);
    CHECK_TRUE("calor: la boca recta (calor 5 y 6)", r.linea);
    CHECK_TRUE("calor: sopla (calor 7)", r.o);
    CHECK_TRUE("calor: el beso al aire (calor 8)", r.beso);
    CHECK_TRUE("calor: la mueca ondulada (calor 9)", r.onda);

    /* A oscuras. */
    recorrer(&r, RK_MOOD_DARK);
    CHECK_TRUE("oscuro: busca la luz arriba, nunca mira abajo", r.mira_y_max < -300);
    CHECK_TRUE("oscuro: y a los dos costados", r.mira_x_min < -400 && r.mira_x_max > 400);
    CHECK_TRUE("oscuro: el parpado recto a media altura", r.a_max <= 120);
    CHECK_TRUE("oscuro: parpadea (oscuro 5)", r.cerrado_alguna_vez);
    CHECK_TRUE("oscuro: las cejas son mechones", r.tupido_min >= 900);

    /* El aire seco. */
    recorrer(&r, RK_MOOD_PARCHED_AIR);
    CHECK_TRUE("aire seco: mira de reojo a un lado", r.mira_x_min < -700);
    CHECK_TRUE("aire seco: y al otro", r.mira_x_max > 700);
    CHECK_TRUE("aire seco: los ojos son rendijas", r.rendija_max <= 450);
    CHECK_TRUE("aire seco: parpadea (seco 7)", r.cerrado_alguna_vez);

    /* Dormido: los ojos no se abren nunca. */
    recorrer(&r, RK_MOOD_SLEEPING);
    CHECK_TRUE("dormido: los ojos cerrados todo el ciclo", r.siempre_cerrados);
}

/* Cada efecto es de su ánimo y de ninguno más. */
static void test_efectos(void)
{
    static const struct { rk_mood_t m; int fx; } DE[] = {
        { RK_MOOD_DROWNING, RK_KIP_FX_AGUA }, { RK_MOOD_PARCHED_AIR, RK_KIP_FX_GRIETAS },
        { RK_MOOD_HOT, RK_KIP_FX_SUDOR }, { RK_MOOD_HOT, RK_KIP_FX_CALOR },
        { RK_MOOD_SLEEPING, RK_KIP_FX_ZZZ }, { RK_MOOD_COLD, RK_KIP_FX_NIEVE },
        { RK_MOOD_UNKNOWN, RK_KIP_FX_DUDA }, { RK_MOOD_OFFLINE, RK_KIP_FX_ESPERA },
        { RK_MOOD_SCORCHED, RK_KIP_FX_SOL }, { RK_MOOD_THIRSTY, RK_KIP_FX_SED },
    };
    size_t i;
    int m;
    char lbl[96];

    for (i = 0; i < sizeof DE / sizeof DE[0]; i++) {
        for (m = 0; m < RK_MOOD_COUNT; m++) {
            rk_kip_pose_t p;
            pose(&p, (rk_mood_t)m, (rk_mood_t)m, 100u, 1200u);
            if (m == (int)DE[i].m) {
                snprintf(lbl, sizeof lbl, "%s tiene su efecto %d", rk_mood_name((rk_mood_t)m), DE[i].fx);
                CHECK_INT(lbl, 1000, p.fx[DE[i].fx]);
            } else if (!(DE[i].fx == RK_KIP_FX_SUDOR || DE[i].fx == RK_KIP_FX_CALOR) ||
                       m != RK_MOOD_HOT) {
                if (p.fx[DE[i].fx] != 0) {
                    snprintf(lbl, sizeof lbl, "%s no tiene el efecto %d", rk_mood_name((rk_mood_t)m), DE[i].fx);
                    rk_t_fail(lbl, "un efecto se coló en otro animo");
                } else {
                    rk_t_pass();
                }
            }
        }
    }
}

/* ------------------------------------------ lo que viene de afuera --- */
static void test_escena(void)
{
    rk_kip_escena_t e;
    rk_kip_pose_t p, q;

    memset(&e, 0, sizeof e);
    e.desde = e.hacia = RK_MOOD_THIRSTY;
    e.t_ms = 1500u;
    rk_kip_pose(&p, &e);
    rk_kip_pose(&q, &e);
    CHECK_TRUE("la pose es funcion pura de la escena", memcmp(&p, &q, sizeof p) == 0);

    e.mimo = 100u;
    rk_kip_pose(&q, &e);
    CHECK_TRUE("acariciado cierra los ojos en ^ ^",
               cerrado(&q.ojo[0]) && cerrado(&q.ojo[1]) && q.ojo[0].b < 0 && q.ojo[1].b < 0);
    CHECK_INT("y sonrie en teal", RK_KIP_BOCA_TEAL, q.boca.estilo);
    CHECK_INT("y se le va la sed", 0, q.fx[RK_KIP_FX_SED]);
    e.mimo = 250u;
    rk_kip_pose(&p, &e);
    CHECK_TRUE("el mimo satura en 100", memcmp(&p, &q, sizeof p) == 0);

    memset(&e, 0, sizeof e);
    e.desde = e.hacia = RK_MOOD_HAPPY;
    e.t_ms = 1200u;
    e.cierre = 100u;
    rk_kip_pose(&p, &e);
    CHECK_TRUE("con el cierre del despertar los ojos estan cerrados",
               cerrado(&p.ojo[0]) && cerrado(&p.ojo[1]));
    e.cierre = 0u;
    rk_kip_pose(&q, &e);
    CHECK_TRUE("sin cierre, abiertos", !cerrado(&q.ojo[0]) && !cerrado(&q.ojo[1]));

    e.mira_x = -80;
    rk_kip_pose(&p, &e);
    CHECK_TRUE("mirar a un vecino corre la mirada", p.mira_x < q.mira_x - 700);
    e.mira_x = 0;
    e.preocupado = 100u;
    rk_kip_pose(&p, &e);
    CHECK_TRUE("preocupado sube las cejas por adentro", p.ceja[0].ang > q.ceja[0].ang + 200);
    CHECK_TRUE("y se le afloja la sonrisa", p.boca.curva < q.boca.curva);

    CHECK_TRUE("un animo fuera de rango no revienta", rk_kip_look((rk_mood_t)99) == rk_kip_look(RK_MOOD_UNKNOWN));
    CHECK_TRUE("y tiene periodo", rk_kip_periodo((rk_mood_t)99) == rk_kip_periodo(RK_MOOD_UNKNOWN));
    rk_kip_pose(NULL, &e);
    rk_kip_pose(&p, NULL);
    rk_kip_dibujar(NULL, NULL, NULL, NULL, 0, 0, 0, 0u);
    CHECK_TRUE("NULL no explota", true);
}

/* ---------------------------------------------------------- paleta --- */
static int luma(rk_color_t c)
{
    int r = ((c >> 11) & 0x1F) * 255 / 31;
    int g = ((c >> 5) & 0x3F) * 255 / 63;
    int b = (c & 0x1F) * 255 / 31;
    return (2126 * r + 7152 * g + 722 * b) / 10000;
}

static void test_paleta(void)
{
    const rk_persona_t *kip = rk_persona_find("kip");
    rk_kip_colores_t k;
    int r, i, grises = 0;
    char lbl[96];

    CHECK_TRUE("Kip esta en la tabla", kip != NULL);
    for (r = 0; r < (int)RK_RAREZA_COUNT; r++) {
        const rk_piel_t *pl = rk_persona_piel(kip, r);
        rk_kip_paleta(&k, pl, true);
        snprintf(lbl, sizeof lbl, "%s: el cuerpo es el fondo de la piel", rk_rareza_id((rk_rareza_t)r));
        CHECK_HEX(lbl, pl->fondo, k.c[RK_KIP_CUERPO]);
        /* Las cejas son de tinta sobre el rojo: tienen que leerse igual. */
        snprintf(lbl, sizeof lbl, "%s: las cejas se leen sobre el rojo (%d)", rk_rareza_id((rk_rareza_t)r),
                 luma(pl->fondo) - luma(pl->ojos));
        CHECK_TRUE(lbl, luma(pl->fondo) - luma(pl->ojos) >= 80);
        snprintf(lbl, sizeof lbl, "%s: el rojo es rojo", rk_rareza_id((rk_rareza_t)r));
        CHECK_TRUE(lbl, ((pl->fondo >> 11) & 0x1F) * 255 / 31 > ((pl->fondo >> 5) & 0x3F) * 255 / 63 + 80);
    }
    /* Con una piel ajena, lo de Kip pasa a gris: la cara dormida no adelanta
     * ni el ámbar ni el teal. */
    rk_kip_paleta(&k, &rk_persona_find("nori")->pieles[0], false);
    for (i = RK_KIP_BLANCO; i < RK_KIP_COLORES; i++) {
        rk_color_t c = k.c[i];
        int rr = ((c >> 11) & 0x1F) * 255 / 31, gg = ((c >> 5) & 0x3F) * 255 / 63, bb = (c & 0x1F) * 255 / 31;
        if (i == RK_KIP_OJERA || i == RK_KIP_RUBOR) {
            continue;
        }
        if (absi(rr - gg) > 10 || absi(gg - bb) > 10) {
            grises++;
        }
    }
    CHECK_INT("con una piel ajena los colores de Kip son grises", 0, grises);
    rk_kip_paleta(NULL, NULL, true);
    CHECK_TRUE("sin paleta no explota", true);
}

void suite_kip(void)
{
    RK_SUITE("kip");
    test_ciclos_fluidos();
    test_transiciones_fluidas();
    test_cejas_saltonas();
    test_la_lamina_esta_en_la_pista();
    test_efectos();
    test_escena();
    test_paleta();
    RK_SUITE_END();
}
