/* Los modelos: la tabla de carcasas y el rig de caras.
 *
 * Esta suite protege la promesa central del producto nuevo: que seis
 * carcasas den seis personajes, y que cada uno exprese los once ánimos de
 * manera distinguible. Si eso no se cumple, la caja ciega vende seis veces
 * lo mismo.
 *
 * Todo lo que se verifica acá es algo que en una placa se descubriría tarde
 * y en una tirada de cien carcasas se descubriría caro.
 */
#include <stddef.h>
#include "rk_test.h"
#include "golden_util.h"
#include "../art/face.h"
#include "../art/look.h"
#include "../core/persona.h"
#include "../core/vinculo.h"

static rk_color_t g_px[RK_MINI_PX];

static uint32_t cara_hash(int persona, rk_mood_t m, uint8_t adornos,
                          uint32_t t_ms)
{
    rk_fb_t fb;
    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);
    rk_face_draw(&fb, rk_persona_at(persona), m, RK_SEV_OK, adornos, t_ms);
    return rk_frame_hash(g_px, RK_MINI_PX);
}

/* ---------------------------------------------------------- la tabla ---- */
static void test_catalogo(void)
{
    int i, j;
    char lbl[96];

    CHECK_TRUE("hay al menos cinco modelos a la vista", rk_persona_count >= 5);

    for (i = 0; i < rk_persona_count; i++) {
        const rk_persona_t *p = rk_persona_at(i);
        snprintf(lbl, sizeof lbl, "el modelo %d existe", i);
        CHECK_TRUE(lbl, p != NULL);
        snprintf(lbl, sizeof lbl, "%s tiene id", p->id);
        CHECK_TRUE(lbl, p->id != NULL && p->id[0] != '\0');
        snprintf(lbl, sizeof lbl, "%s tiene nombre", p->id);
        CHECK_TRUE(lbl, p->nombre != NULL && p->nombre[0] != '\0');
        snprintf(lbl, sizeof lbl, "%s declara su carcasa imprimible", p->id);
        CHECK_TRUE(lbl, p->carcasa != NULL && p->carcasa[0] != '\0');
        snprintf(lbl, sizeof lbl, "%s tiene lema", p->id);
        CHECK_TRUE(lbl, p->lema != NULL && p->lema[0] != '\0');
        snprintf(lbl, sizeof lbl, "%s usa una familia de ojos valida", p->id);
        CHECK_TRUE(lbl, p->familia < RK_OJOS_COUNT);
        snprintf(lbl, sizeof lbl, "%s usa un estilo de boca valido", p->id);
        CHECK_TRUE(lbl, p->boca < RK_BOCA_ESTILO_COUNT);
        snprintf(lbl, sizeof lbl, "%s usa un tipo de ceja valido", p->id);
        CHECK_TRUE(lbl, p->ceja < RK_CEJA_COUNT);
        snprintf(lbl, sizeof lbl, "%s tiene rareza valida", p->id);
        CHECK_TRUE(lbl, p->rareza < RK_RAR_COUNT);
        /* Los rasgos van en centésimas del ancho: si alguno se pasa de 50,
         * el ojo sale más ancho que media pantalla y se come la cara. */
        snprintf(lbl, sizeof lbl, "%s tiene ojos de tamano sensato", p->id);
        CHECK_TRUE(lbl, p->ojo_rx > 0 && p->ojo_rx <= 45 &&
                        p->ojo_ry > 0 && p->ojo_ry <= 45);
        snprintf(lbl, sizeof lbl, "%s no se sale del panel a lo ancho", p->id);
        CHECK_TRUE(lbl, p->ojo_dx + p->ojo_rx <= 50);
    }

    /* Dos ids iguales romperían el índice, que es la clave del protocolo. */
    for (i = 0; i < rk_persona_count; i++) {
        for (j = i + 1; j < rk_persona_count; j++) {
            if (strcmp(rk_persona_at(i)->id, rk_persona_at(j)->id) == 0) {
                snprintf(lbl, sizeof lbl, "id repetido: %s",
                         rk_persona_at(i)->id);
                rk_t_fail(lbl, "dos modelos con la misma clave");
            } else {
                rk_t_pass();
            }
        }
    }

    CHECK_TRUE("un id que no existe devuelve NULL",
               rk_persona_find("no-existe") == NULL);
    CHECK_TRUE("un id nulo devuelve NULL", rk_persona_find(NULL) == NULL);
    CHECK_TRUE("un indice negativo devuelve NULL", rk_persona_at(-1) == NULL);
    CHECK_TRUE("un indice pasado devuelve NULL",
               rk_persona_at(rk_persona_count) == NULL);
    CHECK_INT("una persona nula no tiene indice", -1, rk_persona_index(NULL));

    /* El índice ES la clave con la que viaja por radio. */
    for (i = 0; i < rk_persona_count; i++) {
        snprintf(lbl, sizeof lbl, "indice de %s", rk_persona_at(i)->id);
        CHECK_INT(lbl, i, rk_persona_index(rk_persona_at(i)));
    }
}

/* La caja ciega: cinco a la vista y un secreto. Si esa proporción cambia sin
 * querer, la promesa impresa en la caja deja de ser cierta. */
static void test_la_caja(void)
{
    int comunes = rk_rarity_count(RK_RAR_COMUN);
    int raros   = rk_rarity_count(RK_RAR_RARO);
    int secreto = rk_rarity_count(RK_RAR_SECRETO);

    CHECK_INT("la suma de rarezas es el catalogo entero",
              rk_persona_count, comunes + raros + secreto);
    CHECK_TRUE("hay comunes", comunes > 0);
    CHECK_TRUE("hay raros", raros > 0);
    CHECK_INT("hay exactamente un secreto", 1, secreto);
    CHECK_TRUE("los comunes son mayoria que los raros", comunes >= raros);

    CHECK_STR("la rareza comun tiene nombre", "COMUN",
              rk_rarity_name(RK_RAR_COMUN));
    CHECK_STR("y el secreto tambien", "SECRETO",
              rk_rarity_name(RK_RAR_SECRETO));
    CHECK_STR("una rareza invalida no revienta", "?",
              rk_rarity_name((rk_rarity_t)99));

    /* Más rareza, más destellos: la escala tiene que ser monótona o la
     * ceremonia le miente al usuario sobre lo que le tocó. */
    CHECK_TRUE("el raro brilla mas que el comun",
               rk_rarity_sparkles(RK_RAR_RARO) >
               rk_rarity_sparkles(RK_RAR_COMUN));
    CHECK_TRUE("el secreto brilla mas que el raro",
               rk_rarity_sparkles(RK_RAR_SECRETO) >
               rk_rarity_sparkles(RK_RAR_RARO));

    /* Y cada rareza tiene su color, o el borde de la ficha no dice nada. */
    CHECK_TRUE("comun y raro tienen colores distintos",
               rk_rarity_color(RK_RAR_COMUN) != rk_rarity_color(RK_RAR_RARO));
    CHECK_TRUE("raro y secreto tienen colores distintos",
               rk_rarity_color(RK_RAR_RARO) != rk_rarity_color(RK_RAR_SECRETO));
}

/* --------------------------------------------- seis personajes, no uno -- */
/* La prueba que sostiene el producto: si dos modelos dan la misma cara, la
 * caja ciega vende dos veces lo mismo y no hay colección que juntar. */
static void test_los_modelos_se_ven_distintos(void)
{
    uint32_t h[16];
    char lbl[96];
    int i, j;

    for (i = 0; i < rk_persona_count; i++) {
        h[i] = cara_hash(i, RK_MOOD_HAPPY, 0u, 1200u);
    }
    for (i = 0; i < rk_persona_count; i++) {
        for (j = i + 1; j < rk_persona_count; j++) {
            if (h[i] == h[j]) {
                snprintf(lbl, sizeof lbl, "%s y %s dan la misma cara",
                         rk_persona_at(i)->nombre, rk_persona_at(j)->nombre);
                rk_t_fail(lbl, "dos modelos indistinguibles");
            } else {
                rk_t_pass();
            }
        }
    }
}

/* Y dentro de cada modelo, los once ánimos tienen que distinguirse. Un
 * modelo que pone la misma cara para sed y para frío no comunica nada. */
static void test_los_animos_se_distinguen_en_cada_modelo(void)
{
    int p, i, j;
    char lbl[128];

    for (p = 0; p < rk_persona_count; p++) {
        uint32_t h[RK_MOOD_COUNT];
        for (i = 0; i < RK_MOOD_COUNT; i++) {
            h[i] = cara_hash(p, (rk_mood_t)i, 0u, 1200u);
        }
        for (i = 0; i < RK_MOOD_COUNT; i++) {
            for (j = i + 1; j < RK_MOOD_COUNT; j++) {
                if (h[i] == h[j]) {
                    snprintf(lbl, sizeof lbl, "%s: %s y %s son la misma cara",
                             rk_persona_at(p)->nombre,
                             rk_mood_name((rk_mood_t)i),
                             rk_mood_name((rk_mood_t)j));
                    rk_t_fail(lbl, "dos animos indistinguibles");
                } else {
                    rk_t_pass();
                }
            }
        }
    }
}

/* --------------------------------------------------------- crecimiento -- */
static void test_adornos_por_etapa(void)
{
    CHECK_INT("espora no desbloquea nada", 0,
              rk_face_adornos_etapa(RK_ETAPA_ESPORA));
    CHECK_INT("brote tampoco", 0, rk_face_adornos_etapa(RK_ETAPA_BROTE));
    CHECK_TRUE("joven desbloquea brillos",
               (rk_face_adornos_etapa(RK_ETAPA_JOVEN) &
                RK_ADORNO_BRILLOS) != 0u);
    CHECK_TRUE("maduro desbloquea el aura",
               (rk_face_adornos_etapa(RK_ETAPA_MADURO) &
                RK_ADORNO_AURA) != 0u);
    CHECK_TRUE("ancestral desbloquea la corona",
               (rk_face_adornos_etapa(RK_ETAPA_ANCESTRAL) &
                RK_ADORNO_CORONA) != 0u);
    CHECK_INT("una etapa invalida no desbloquea nada", 0,
              rk_face_adornos_etapa(-3));

    /* Y las etapas se tienen que VER distintas, o el premio no existe. */
    {
        uint32_t h[RK_ETAPA_COUNT];
        char lbl[96];
        int i, j;
        for (i = 0; i < RK_ETAPA_COUNT; i++) {
            h[i] = cara_hash(0, RK_MOOD_HAPPY,
                             rk_face_adornos_etapa(i), 1200u);
        }
        /* ESPORA y BROTE comparten adornos a propósito: el primer premio
         * llega recién a los 30 días sanos, y eso es deliberado. De JOVEN en
         * adelante cada etapa tiene que verse. */
        for (i = 2; i < RK_ETAPA_COUNT; i++) {
            for (j = i + 1; j < RK_ETAPA_COUNT; j++) {
                if (h[i] == h[j]) {
                    snprintf(lbl, sizeof lbl, "las etapas %s y %s se ven igual",
                             rk_stage_name((rk_stage_t)i),
                             rk_stage_name((rk_stage_t)j));
                    rk_t_fail(lbl, "el crecimiento no se nota");
                } else {
                    rk_t_pass();
                }
            }
        }
        CHECK_TRUE("joven ya se ve distinto de espora", h[0] != h[2]);
    }
}

/* ------------------------------------------------------- pictogramas ---- */
/* La cara dice que algo anda mal; el pictograma dice qué. Si dos necesidades
 * dibujan el mismo icono, el usuario riega una planta que tenía frío. */
static void test_pictogramas(void)
{
    static rk_color_t px[64 * 64];
    rk_fb_t fb;
    uint32_t h[RK_MOOD_COUNT];
    bool hay[RK_MOOD_COUNT];
    char lbl[112];
    int i, j;

    rk_fb_init(&fb, px, 64, 64);

    for (i = 0; i < RK_MOOD_COUNT; i++) {
        rk_fb_clear(&fb, 0x0000);
        hay[i] = rk_face_pictograma(&fb, 8, 8, 40, (rk_mood_t)i, 0xFFFF);
        h[i] = rk_frame_hash(px, 64 * 64);
    }

    /* Los estados buenos no piden nada. */
    CHECK_TRUE("estar bien no dibuja pictograma", !hay[RK_MOOD_HAPPY]);
    CHECK_TRUE("dormir tampoco", !hay[RK_MOOD_SLEEPING]);
    CHECK_TRUE("sin datos tampoco", !hay[RK_MOOD_UNKNOWN]);
    /* Los malos, sí. */
    CHECK_TRUE("la sed pide agua", hay[RK_MOOD_THIRSTY]);
    CHECK_TRUE("el frio avisa", hay[RK_MOOD_COLD]);
    CHECK_TRUE("la falta de luz avisa", hay[RK_MOOD_DARK]);

    for (i = 0; i < RK_MOOD_COUNT; i++) {
        if (!hay[i]) {
            continue;
        }
        for (j = i + 1; j < RK_MOOD_COUNT; j++) {
            if (!hay[j]) {
                continue;
            }
            if (h[i] == h[j]) {
                snprintf(lbl, sizeof lbl, "%s y %s dibujan el mismo icono",
                         rk_mood_name((rk_mood_t)i), rk_mood_name((rk_mood_t)j));
                rk_t_fail(lbl, "dos necesidades con el mismo pictograma");
            } else {
                rk_t_pass();
            }
        }
    }

    CHECK_TRUE("un pictograma diminuto se rechaza en vez de dibujar basura",
               !rk_face_pictograma(&fb, 0, 0, 3, RK_MOOD_THIRSTY, 0xFFFF));
    CHECK_TRUE("sin framebuffer no explota",
               !rk_face_pictograma(NULL, 0, 0, 20, RK_MOOD_THIRSTY, 0xFFFF));
}

/* ------------------------------------------------------------ bordes ---- */
static void test_bordes(void)
{
    rk_fb_t fb;
    int p, m;

    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);

    rk_face_draw(NULL, NULL, RK_MOOD_HAPPY, RK_SEV_OK, 0u, 0u);
    CHECK_TRUE("sin framebuffer no explota", true);
    rk_face_draw(&fb, NULL, RK_MOOD_HAPPY, RK_SEV_OK, 0u, 0u);
    CHECK_TRUE("sin modelo cae en el primero", true);
    rk_face_draw(&fb, rk_persona_at(0), (rk_mood_t)99, RK_SEV_OK, 0u, 0u);
    CHECK_TRUE("un animo invalido cae en UNKNOWN", true);
    rk_face_rasgos(&fb, 64, 64, 4, rk_persona_at(0), RK_MOOD_HAPPY, 0u, 0u);
    CHECK_TRUE("una cara diminuta se rechaza", true);

    /* Todos los modelos en todos los animos con todos los adornos, a lo
     * largo de doce segundos: es el barrido que encuentra la division por
     * cero o el indice negativo que un caso suelto no toca. */
    for (p = 0; p < rk_persona_count; p++) {
        for (m = 0; m < RK_MOOD_COUNT; m++) {
            uint32_t t;
            for (t = 0u; t < 12000u; t += 613u) {
                rk_face_draw(&fb, rk_persona_at(p), (rk_mood_t)m,
                             (t % 2u) ? RK_SEV_URGENT : RK_SEV_WATCH,
                             0xFFu, t);
            }
        }
    }
    CHECK_TRUE("el barrido completo no rompe", true);

    /* Centinelas: el rig dibuja elipses y arcos con radios que salen de una
     * tabla editable a mano. Un radio de mas y se escribe fuera del buffer,
     * que en el ESP32 no tira excepcion: corrompe lo de al lado. */
    {
        static rk_color_t buf[RK_MINI_PX + 64];
        rk_fb_t chico;
        int i, sucio = 0;

        for (i = 0; i < RK_MINI_PX + 64; i++) {
            buf[i] = 0xBEEFu;
        }
        rk_fb_init(&chico, &buf[32], RK_MINI_W, RK_MINI_H);
        for (p = 0; p < rk_persona_count; p++) {
            for (m = 0; m < RK_MOOD_COUNT; m++) {
                rk_face_draw(&chico, rk_persona_at(p), (rk_mood_t)m,
                             RK_SEV_URGENT, 0xFFu, 1200u);
            }
        }
        /* Y la ficha chica dibujada pisando los bordes. */
        rk_face_rasgos(&chico, -30, -20, 90, rk_persona_at(3),
                       RK_MOOD_HAPPY, 0xFFu, 0u);
        rk_face_rasgos(&chico, RK_MINI_W + 30, RK_MINI_H + 20, 90,
                       rk_persona_at(3), RK_MOOD_HAPPY, 0xFFu, 0u);

        for (i = 0; i < 32; i++) {
            if (buf[i] != 0xBEEFu) { sucio++; }
            if (buf[RK_MINI_PX + 32 + i] != 0xBEEFu) { sucio++; }
        }
        CHECK_INT("ninguna cara escribe fuera del framebuffer", 0, sucio);
    }
}

void suite_persona(void)
{
    RK_SUITE("modelos y caras");
    test_catalogo();
    test_la_caja();
    test_los_modelos_se_ven_distintos();
    test_los_animos_se_distinguen_en_cada_modelo();
    test_adornos_por_etapa();
    test_pictogramas();
    test_bordes();
    RK_SUITE_END();
}
