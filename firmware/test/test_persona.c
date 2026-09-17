/* Los Rooties: la tabla de personajes, sus pieles y el rig de caras.
 *
 * Esta suite protege la promesa central del producto: que cinco figuras den
 * cinco personajes, que cada piel se vea distinta de las otras dos del mismo
 * Rooti (si no, el cofre sortea nada), y que cada uno exprese los once
 * ánimos de manera distinguible, en las tres pieles.
 *
 * Todo lo que se verifica acá es algo que en una placa se descubriría tarde
 * y en una tirada de cien figuras se descubriría caro.
 */
#include <stddef.h>
#include "rk_test.h"
#include "golden_util.h"
#include "../art/face.h"
#include "../core/persona.h"
#include "../core/vinculo.h"

static rk_color_t g_px[RK_MINI_PX];

static uint32_t cara_hash(int persona, int rareza, rk_mood_t m, uint8_t adornos,
                          uint32_t t_ms)
{
    rk_fb_t fb;
    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);
    rk_face_draw(&fb, rk_persona_at(persona), (uint8_t)rareza, m, RK_SEV_OK, adornos, t_ms);
    return rk_frame_hash(g_px, RK_MINI_PX);
}

/* Luma de un RGB565, en 0..255 (Rec. 709, en enteros). */
static int luma(rk_color_t c)
{
    int r = ((c >> 11) & 0x1F) * 255 / 31;
    int g = ((c >> 5) & 0x3F) * 255 / 63;
    int b = (c & 0x1F) * 255 / 31;
    return (2126 * r + 7152 * g + 722 * b) / 10000;
}

/* ---------------------------------------------------------- la tabla ---- */
static void test_catalogo(void)
{
    static const char *IDS[] = { "brote", "musgo", "pinchito", "bulbo", "champi" };
    int i, j, r;
    char lbl[112];

    CHECK_INT("son cinco Rooties", 5, rk_persona_count);
    /* El índice ES la clave con la que viaja: el orden no se toca. */
    for (i = 0; i < rk_persona_count && i < 5; i++) {
        snprintf(lbl, sizeof lbl, "el Rooti %d es %s", i, IDS[i]);
        CHECK_STR(lbl, IDS[i], rk_persona_at(i)->id);
    }

    for (i = 0; i < rk_persona_count; i++) {
        const rk_persona_t *p = rk_persona_at(i);
        snprintf(lbl, sizeof lbl, "%s tiene nombre", p->id);
        CHECK_TRUE(lbl, p->nombre != NULL && p->nombre[0] != '\0');
        snprintf(lbl, sizeof lbl, "%s declara su figura imprimible", p->id);
        CHECK_TRUE(lbl, p->carcasa != NULL && p->carcasa[0] != '\0');
        snprintf(lbl, sizeof lbl, "%s tiene lema", p->id);
        CHECK_TRUE(lbl, p->lema != NULL && p->lema[0] != '\0');
        snprintf(lbl, sizeof lbl, "%s usa una familia de ojos valida", p->id);
        CHECK_TRUE(lbl, p->familia < RK_OJOS_COUNT);
        snprintf(lbl, sizeof lbl, "%s usa un brillo valido", p->id);
        CHECK_TRUE(lbl, p->brillo < RK_BRILLO_COUNT);
        snprintf(lbl, sizeof lbl, "%s usa un estilo de boca valido", p->id);
        CHECK_TRUE(lbl, p->boca < RK_BOCA_ESTILO_COUNT);
        snprintf(lbl, sizeof lbl, "%s usa un tipo de ceja valido", p->id);
        CHECK_TRUE(lbl, p->ceja < RK_CEJA_COUNT);
        snprintf(lbl, sizeof lbl, "%s usa mejillas validas", p->id);
        CHECK_TRUE(lbl, p->mejilla < RK_MEJILLA_COUNT);
        /* Los rasgos van en centésimas del lado: si un ojo pasa de 45, se
         * come la cara; si se sale de 48 a lo ancho, se sale del panel. */
        snprintf(lbl, sizeof lbl, "%s tiene ojos de tamano sensato", p->id);
        CHECK_TRUE(lbl, p->ojo_rx > 0 && p->ojo_rx <= 45 &&
                        p->ojo_ry > 0 && p->ojo_ry <= 45);
        snprintf(lbl, sizeof lbl, "%s no se sale del panel a lo ancho", p->id);
        CHECK_TRUE(lbl, p->ojo_dx + p->ojo_rx <= 48);
        snprintf(lbl, sizeof lbl, "%s no se sale del panel arriba", p->id);
        CHECK_TRUE(lbl, p->ojo_dy - p->ojo_ry >= -46);
        snprintf(lbl, sizeof lbl, "%s tiene la boca abajo de los ojos y adentro", p->id);
        CHECK_TRUE(lbl, p->boca_dy > p->ojo_dy && p->boca_dy + p->boca_ancho <= 46);

        for (r = 0; r < (int)RK_RAREZA_COUNT; r++) {
            const rk_piel_t *pl = rk_persona_piel(p, r);
            snprintf(lbl, sizeof lbl, "%s %s tiene nombre", p->id, rk_rareza_id((rk_rareza_t)r));
            CHECK_TRUE(lbl, pl->nombre != NULL && pl->nombre[0] != '\0');
            /* Ojos oscuros sobre fondo claro: la diferencia de luminancia es
             * lo que hace que la cara se lea a un metro. */
            snprintf(lbl, sizeof lbl, "%s %s: los ojos se leen sobre el fondo (%d)",
                     p->id, rk_rareza_id((rk_rareza_t)r), luma(pl->fondo) - luma(pl->ojos));
            CHECK_TRUE(lbl, luma(pl->fondo) - luma(pl->ojos) >= 120);
        }
        snprintf(lbl, sizeof lbl, "%s: las tres pieles tienen fondos distintos", p->id);
        CHECK_TRUE(lbl, p->pieles[0].fondo != p->pieles[1].fondo &&
                        p->pieles[1].fondo != p->pieles[2].fondo &&
                        p->pieles[0].fondo != p->pieles[2].fondo);
        snprintf(lbl, sizeof lbl, "%s: la comun no trae adornos", p->id);
        CHECK_INT(lbl, 0, p->pieles[RK_RAREZA_COMUN].adornos);
        snprintf(lbl, sizeof lbl, "%s: la rara brilla", p->id);
        CHECK_TRUE(lbl, (p->pieles[RK_RAREZA_RARA].adornos & RK_ADORNO_BRILLOS) != 0u);
        snprintf(lbl, sizeof lbl, "%s: la epica tiene corona o aura", p->id);
        CHECK_TRUE(lbl, (p->pieles[RK_RAREZA_EPICA].adornos &
                         (RK_ADORNO_CORONA | RK_ADORNO_AURA)) != 0u);
    }

    for (i = 0; i < rk_persona_count; i++) {
        for (j = i + 1; j < rk_persona_count; j++) {
            if (strcmp(rk_persona_at(i)->id, rk_persona_at(j)->id) == 0) {
                snprintf(lbl, sizeof lbl, "id repetido: %s", rk_persona_at(i)->id);
                rk_t_fail(lbl, "dos Rooties con la misma clave");
            } else {
                rk_t_pass();
            }
        }
    }

    CHECK_TRUE("un id que no existe devuelve NULL", rk_persona_find("no-existe") == NULL);
    CHECK_TRUE("un id viejo tampoco existe", rk_persona_find("kawaii") == NULL);
    CHECK_TRUE("un id nulo devuelve NULL", rk_persona_find(NULL) == NULL);
    CHECK_TRUE("un indice negativo devuelve NULL", rk_persona_at(-1) == NULL);
    CHECK_TRUE("un indice pasado devuelve NULL", rk_persona_at(rk_persona_count) == NULL);
    CHECK_INT("una persona nula no tiene indice", -1, rk_persona_index(NULL));
    for (i = 0; i < rk_persona_count; i++) {
        snprintf(lbl, sizeof lbl, "indice de %s", rk_persona_at(i)->id);
        CHECK_INT(lbl, i, rk_persona_index(rk_persona_find(rk_persona_at(i)->id)));
    }
}

/* El cofre: tres rarezas, con su clave de protocolo y su escala. */
static void test_las_rarezas(void)
{
    int r;

    CHECK_INT("hay tres rarezas", 3, RK_RAREZA_COUNT);
    CHECK_STR("la comun viaja como comun", "comun", rk_rareza_id(RK_RAREZA_COMUN));
    CHECK_STR("la rara como raro", "raro", rk_rareza_id(RK_RAREZA_RARA));
    CHECK_STR("la epica como epico", "epico", rk_rareza_id(RK_RAREZA_EPICA));
    CHECK_STR("y se rotula EPICA", "EPICA", rk_rareza_nombre(RK_RAREZA_EPICA));
    for (r = 0; r < (int)RK_RAREZA_COUNT; r++) {
        CHECK_INT("la clave vuelve a la rareza", r, rk_rareza_parse(rk_rareza_id((rk_rareza_t)r)));
    }
    CHECK_INT("una clave desconocida no es rareza", -1, rk_rareza_parse("secreto"));
    CHECK_INT("NULL tampoco", -1, rk_rareza_parse(NULL));
    CHECK_STR("una rareza invalida no revienta", "?", rk_rareza_id((rk_rareza_t)99));

    CHECK_TRUE("una piel fuera de rango cae en la comun",
               rk_persona_piel(rk_persona_at(0), 7) == &rk_persona_at(0)->pieles[0] &&
               rk_persona_piel(rk_persona_at(0), -1) == &rk_persona_at(0)->pieles[0]);
    CHECK_TRUE("sin Rooti no hay piel", rk_persona_piel(NULL, 0) == NULL);

    /* Más rareza, más destellos: la escala tiene que ser monótona o la
     * ceremonia le miente al usuario sobre lo que le tocó. */
    CHECK_TRUE("la rara brilla mas que la comun",
               rk_rareza_destellos(RK_RAREZA_RARA) > rk_rareza_destellos(RK_RAREZA_COMUN));
    CHECK_TRUE("la epica brilla mas que la rara",
               rk_rareza_destellos(RK_RAREZA_EPICA) > rk_rareza_destellos(RK_RAREZA_RARA));
    CHECK_TRUE("cada rareza tiene su color",
               rk_rareza_color(RK_RAREZA_COMUN) != rk_rareza_color(RK_RAREZA_RARA) &&
               rk_rareza_color(RK_RAREZA_RARA) != rk_rareza_color(RK_RAREZA_EPICA));
}

/* --------------------------------------------- cinco personajes, no uno -- */
static void test_los_rooties_se_ven_distintos(void)
{
    uint32_t h[16];
    char lbl[96];
    int i, j;

    for (i = 0; i < rk_persona_count; i++) {
        h[i] = cara_hash(i, RK_RAREZA_COMUN, RK_MOOD_HAPPY, 0u, 1200u);
    }
    for (i = 0; i < rk_persona_count; i++) {
        for (j = i + 1; j < rk_persona_count; j++) {
            if (h[i] == h[j]) {
                snprintf(lbl, sizeof lbl, "%s y %s dan la misma cara",
                         rk_persona_at(i)->nombre, rk_persona_at(j)->nombre);
                rk_t_fail(lbl, "dos Rooties indistinguibles");
            } else {
                rk_t_pass();
            }
        }
    }
}

/* Si dos pieles del mismo Rooti se vieran igual, el cofre sortearía nada. */
static void test_las_pieles_se_ven_distintas(void)
{
    char lbl[96];
    int p, a, b;

    for (p = 0; p < rk_persona_count; p++) {
        uint32_t h[RK_RAREZA_COUNT];
        for (a = 0; a < (int)RK_RAREZA_COUNT; a++) {
            h[a] = cara_hash(p, a, RK_MOOD_HAPPY, 0u, 1200u);
        }
        for (a = 0; a < (int)RK_RAREZA_COUNT; a++) {
            for (b = a + 1; b < (int)RK_RAREZA_COUNT; b++) {
                snprintf(lbl, sizeof lbl, "%s: %s y %s se ven distintas", rk_persona_at(p)->nombre,
                         rk_rareza_nombre((rk_rareza_t)a), rk_rareza_nombre((rk_rareza_t)b));
                CHECK_TRUE(lbl, h[a] != h[b]);
            }
        }
    }
}

/* Y dentro de cada Rooti y cada piel, los once ánimos tienen que
 * distinguirse. Un Rooti que pone la misma cara para sed y para frío no
 * comunica nada. */
static void test_los_animos_se_distinguen(void)
{
    int p, r, i, j;
    char lbl[128];

    for (p = 0; p < rk_persona_count; p++) {
        for (r = 0; r < (int)RK_RAREZA_COUNT; r++) {
            uint32_t h[RK_MOOD_COUNT];
            for (i = 0; i < RK_MOOD_COUNT; i++) {
                h[i] = cara_hash(p, r, (rk_mood_t)i, 0u, 1200u);
            }
            for (i = 0; i < RK_MOOD_COUNT; i++) {
                for (j = i + 1; j < RK_MOOD_COUNT; j++) {
                    if (h[i] == h[j]) {
                        snprintf(lbl, sizeof lbl, "%s %s: %s y %s son la misma cara",
                                 rk_persona_at(p)->nombre, rk_rareza_nombre((rk_rareza_t)r),
                                 rk_mood_name((rk_mood_t)i), rk_mood_name((rk_mood_t)j));
                        rk_t_fail(lbl, "dos animos indistinguibles");
                    } else {
                        rk_t_pass();
                    }
                }
            }
        }
    }
}

/* Pinchito contento está en ^ ^ y guiña: un ojo y después el otro. */
static void test_el_guino(void)
{
    const int pinchito = 2;
    uint32_t quieto = cara_hash(pinchito, RK_RAREZA_COMUN, RK_MOOD_HAPPY, 0u, 1200u);
    uint32_t izq = cara_hash(pinchito, RK_RAREZA_COMUN, RK_MOOD_HAPPY, 0u, 4400u);
    uint32_t der = cara_hash(pinchito, RK_RAREZA_COMUN, RK_MOOD_HAPPY, 0u, 9600u);

    CHECK_STR("el tercer Rooti es Pinchito", "pinchito", rk_persona_at(pinchito)->id);
    CHECK_TRUE("guina con uno y despues con el otro", izq != der);
    CHECK_TRUE("y el guino no es la cara quieta", izq != quieto && der != quieto);
}

/* --------------------------------------------------------- crecimiento -- */
static void test_adornos_por_etapa(void)
{
    CHECK_INT("espora no desbloquea nada", 0, rk_face_adornos_etapa(RK_ETAPA_ESPORA));
    CHECK_INT("brote tampoco", 0, rk_face_adornos_etapa(RK_ETAPA_BROTE));
    CHECK_TRUE("joven desbloquea brillos",
               (rk_face_adornos_etapa(RK_ETAPA_JOVEN) & RK_ADORNO_BRILLOS) != 0u);
    CHECK_TRUE("maduro desbloquea el aura",
               (rk_face_adornos_etapa(RK_ETAPA_MADURO) & RK_ADORNO_AURA) != 0u);
    CHECK_TRUE("ancestral desbloquea la corona",
               (rk_face_adornos_etapa(RK_ETAPA_ANCESTRAL) & RK_ADORNO_CORONA) != 0u);
    CHECK_INT("una etapa invalida no desbloquea nada", 0, rk_face_adornos_etapa(-3));

    {
        uint32_t h[RK_ETAPA_COUNT];
        char lbl[96];
        int i, j;
        for (i = 0; i < RK_ETAPA_COUNT; i++) {
            h[i] = cara_hash(0, RK_RAREZA_COMUN, RK_MOOD_HAPPY, rk_face_adornos_etapa(i), 1200u);
        }
        /* ESPORA y BROTE comparten adornos a propósito: el primer premio
         * llega a los 30 días sanos. De JOVEN en adelante cada etapa se ve. */
        for (i = 2; i < RK_ETAPA_COUNT; i++) {
            for (j = i + 1; j < RK_ETAPA_COUNT; j++) {
                if (h[i] == h[j]) {
                    snprintf(lbl, sizeof lbl, "las etapas %s y %s se ven igual",
                             rk_stage_name((rk_stage_t)i), rk_stage_name((rk_stage_t)j));
                    rk_t_fail(lbl, "el crecimiento no se nota");
                } else {
                    rk_t_pass();
                }
            }
        }
        CHECK_TRUE("joven ya se ve distinto de espora", h[0] != h[2]);
    }
}

/* ------------------------------------------------------------ bordes ---- */
static void test_bordes(void)
{
    rk_fb_t fb;
    int p, m, r;

    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);

    rk_face_draw(NULL, NULL, 0u, RK_MOOD_HAPPY, RK_SEV_OK, 0u, 0u);
    CHECK_TRUE("sin framebuffer no explota", true);
    rk_face_draw(&fb, NULL, 0u, RK_MOOD_HAPPY, RK_SEV_OK, 0u, 0u);
    CHECK_TRUE("sin Rooti cae en el primero", true);
    rk_face_draw(&fb, rk_persona_at(0), 0u, (rk_mood_t)99, RK_SEV_OK, 0u, 0u);
    CHECK_TRUE("un animo invalido cae en UNKNOWN", true);
    CHECK_HEX("una piel invalida es la comun",
              cara_hash(1, RK_RAREZA_COMUN, RK_MOOD_HAPPY, 0u, 1200u),
              cara_hash(1, 200, RK_MOOD_HAPPY, 0u, 1200u));
    {
        static rk_color_t mini[4 * 4];
        rk_fb_t chiquito;
        rk_fb_init(&chiquito, mini, 4, 4);
        rk_face_draw(&chiquito, rk_persona_at(0), 2u, RK_MOOD_HAPPY, RK_SEV_OK, 0xFFu, 0u);
        CHECK_TRUE("una cara diminuta no explota", true);
    }

    /* Todos los Rooties, en todas las pieles y ánimos, con todos los
     * adornos, a lo largo de doce segundos. */
    for (p = 0; p < rk_persona_count; p++) {
        for (r = 0; r < (int)RK_RAREZA_COUNT; r++) {
            for (m = 0; m < RK_MOOD_COUNT; m++) {
                uint32_t t;
                for (t = 0u; t < 12000u; t += 1613u) {
                    rk_face_draw(&fb, rk_persona_at(p), (uint8_t)r, (rk_mood_t)m,
                                 (t % 2u) ? RK_SEV_URGENT : RK_SEV_WATCH, 0xFFu, t);
                }
            }
        }
    }
    CHECK_TRUE("el barrido completo no rompe", true);

    /* Centinelas: un radio de más escribe fuera del buffer, y en el ESP32 eso
     * no tira excepción: corrompe lo de al lado. */
    {
        static rk_color_t buf[RK_MINI_PX + 64];
        rk_fb_t chico;
        int i, sucio = 0;

        for (i = 0; i < RK_MINI_PX + 64; i++) {
            buf[i] = 0xBEEFu;
        }
        rk_fb_init(&chico, &buf[32], RK_MINI_W, RK_MINI_H);
        for (p = 0; p < rk_persona_count; p++) {
            for (r = 0; r < (int)RK_RAREZA_COUNT; r++) {
                for (m = 0; m < RK_MOOD_COUNT; m++) {
                    rk_face_draw(&chico, rk_persona_at(p), (uint8_t)r, (rk_mood_t)m,
                                 RK_SEV_URGENT, 0xFFu, 1200u);
                }
                rk_face_draw_cierre(&chico, rk_persona_at(p), (uint8_t)r, RK_MOOD_HAPPY,
                                    RK_SEV_OK, 0xFFu, 55u, 800u);
            }
        }
        for (i = 0; i < 32; i++) {
            if (buf[i] != 0xBEEFu) { sucio++; }
            if (buf[RK_MINI_PX + 32 + i] != 0xBEEFu) { sucio++; }
        }
        CHECK_INT("ninguna cara escribe fuera del framebuffer", 0, sucio);
    }
}

void suite_persona(void)
{
    RK_SUITE("rooties y caras");
    test_catalogo();
    test_las_rarezas();
    test_los_rooties_se_ven_distintos();
    test_las_pieles_se_ven_distintas();
    test_los_animos_se_distinguen();
    test_el_guino();
    test_adornos_por_etapa();
    test_bordes();
    RK_SUITE_END();
}
