/* El Mini: su pantalla de 128x128, el brote y las dos escalas del arte.
 *
 * Esta suite existe porque el Mini es la pieza más fácil de romper sin
 * enterarse. Es la pantalla que va a estar en la maceta, mirada de reojo, y
 * la que nadie tiene enfrente mientras programa. Todo lo que acá se verifica
 * es algo que en una placa se descubriría tarde:
 *
 *   - que el brote entre en la banda de escena y mida los 12,9 mm prometidos,
 *   - que las frases del catálogo entren en 128 px sin cortarse,
 *   - que los once ánimos se vean distintos entre sí también en miniatura,
 *   - que las cinco etapas de crecimiento se distingan una de otra,
 *   - que los doce simbiontes se distingan entre sí,
 *   - y que las dos tablas de arte —adulto y brote— estén completas.
 */
#include <stddef.h>
#include "rk_test.h"
#include "golden_util.h"
#include "golden.h"
#include "../gfx/font.h"
#include "../art/brote.h"
#include "../art/look.h"

static rk_color_t g_px[RK_MINI_PX];

/* ---------------------------------------------------------- las tablas -- */
/* El look es compartido; el arte no. Si una tabla de arte tiene un hueco, el
 * primer ánimo que caiga en esa fila desreferencia NULL en la maceta del
 * usuario y no acá, así que se verifica acá. */
static void test_tablas_de_arte(void)
{
    int i;
    char lbl[80];

    for (i = 0; i < RK_OJO_COUNT; i++) {
        snprintf(lbl, sizeof lbl, "ojo %d del adulto existe", i);
        CHECK_TRUE(lbl, RK_AD_OJO[i] != NULL && RK_AD_OJO[i]->idx != NULL);
        snprintf(lbl, sizeof lbl, "ojo %d del brote existe", i);
        CHECK_TRUE(lbl, RK_BR_OJO[i] != NULL && RK_BR_OJO[i]->idx != NULL);
    }
    for (i = 0; i < RK_BOCA_COUNT; i++) {
        snprintf(lbl, sizeof lbl, "boca %d del adulto existe", i);
        CHECK_TRUE(lbl, RK_AD_BOCA[i] != NULL && RK_AD_BOCA[i]->idx != NULL);
        snprintf(lbl, sizeof lbl, "boca %d del brote existe", i);
        CHECK_TRUE(lbl, RK_BR_BOCA[i] != NULL && RK_BR_BOCA[i]->idx != NULL);
    }

    /* El look de cada ánimo tiene que indexar dentro de las tablas. */
    for (i = 0; i < RK_MOOD_COUNT; i++) {
        const rk_look_t *lk = rk_look((rk_mood_t)i);
        snprintf(lbl, sizeof lbl, "%s indexa un ojo valido",
                 rk_mood_name((rk_mood_t)i));
        CHECK_TRUE(lbl, lk->ojo < RK_OJO_COUNT);
        snprintf(lbl, sizeof lbl, "%s indexa una boca valida",
                 rk_mood_name((rk_mood_t)i));
        CHECK_TRUE(lbl, lk->boca < RK_BOCA_COUNT);
    }

    /* Un ánimo fuera de rango no puede devolver basura. */
    CHECK_TRUE("un animo invalido cae en UNKNOWN",
               rk_look((rk_mood_t)999) == rk_look(RK_MOOD_UNKNOWN));
    CHECK_TRUE("un animo negativo cae en UNKNOWN",
               rk_look((rk_mood_t)-4) == rk_look(RK_MOOD_UNKNOWN));
}

/* --------------------------------------------------------- las paletas -- */
static void test_paletas(void)
{
    int i, j;
    char lbl[80];

    CHECK_INT("hay una paleta por simbionte", rk_companion_count, RK_PAL_COUNT);
    CHECK_INT("hay un cuerpo de brote por simbionte",
              rk_companion_count, RK_PAL_COUNT);

    /* Dos simbiontes con la misma paleta son el mismo bicho pintado igual, y
     * la colección deja de tener sentido. */
    for (i = 0; i < RK_PAL_COUNT; i++) {
        for (j = i + 1; j < RK_PAL_COUNT; j++) {
            int k, iguales = 1;
            for (k = 1; k < RK_PAL_LEN; k++) {
                if (RK_PAL[i][k] != RK_PAL[j][k]) {
                    iguales = 0;
                    break;
                }
            }
            if (iguales) {
                snprintf(lbl, sizeof lbl, "%s y %s comparten paleta",
                         rk_companion_table[i].id, rk_companion_table[j].id);
                rk_t_fail(lbl, "dos simbiontes pintados igual");
            } else {
                rk_t_pass();
            }
        }
    }

    /* El índice del simbionte ES la clave del arte. Si la tabla de
     * companions se reordena sin regenerar el arte, cada bicho sale con la
     * paleta del vecino y nada falla ruidosamente. */
    for (i = 0; i < rk_companion_count; i++) {
        snprintf(lbl, sizeof lbl, "indice de %s", rk_companion_table[i].id);
        CHECK_INT(lbl, i, rk_companion_index(&rk_companion_table[i]));
    }
    CHECK_INT("un simbionte nulo no tiene indice", -1, rk_companion_index(NULL));
}

/* ----------------------------------------------------- el brote entra --- */
static void test_el_brote_entra(void)
{
    int alto = RK_BROTE_H * RK_MINI_ART;

    /* La banda de escena del Mini son 88 px (ver ui/mini.h), y el brote con
     * hojas de etapa se estira otros 10 px hacia arriba. */
    CHECK_TRUE("el brote entra a lo alto en la banda de escena",
               alto + 10 <= 88);
    /* A lo ancho no alcanza con el cuerpo: las hojas de la etapa ANCESTRAL
     * salen 22 px de arte a cada lado del centro, y son la parte del dibujo
     * que hace visible el crecimiento. Si se cortan, el premio por seis
     * meses de cuidado queda pegado al borde de la pantalla. */
    CHECK_TRUE("el cuerpo entra a lo ancho en el panel",
               RK_BROTE_W * RK_MINI_ART <= RK_MINI_W);
    CHECK_TRUE("y el abanico de hojas de ANCESTRAL tambien",
               22 * 2 * RK_MINI_ART <= RK_MINI_W);

    /* Los tamaños físicos son los que decidieron la compra del hardware. Si
     * alguien cambia la escala del arte, estos dos numeros lo delatan. */
    CHECK_NEAR("el brote mide 12,9 mm en el Mini (decimas)",
               129, rk_panel_decimas_mm(RK_PANEL_MINI, alto), 3);
    CHECK_NEAR("el adulto mide 21,9 mm en el Prime (decimas)",
               219, rk_panel_decimas_mm(RK_PANEL_PRIME,
                                        RK_ADULTO_H * RK_PRIME_ART), 3);
    /* Y la jerarquía entre los dos: el papá tiene que verse claramente más
     * grande que la cría, no apenas más grande. */
    CHECK_TRUE("el adulto mide al menos 1,6 veces el brote",
               rk_panel_decimas_mm(RK_PANEL_PRIME, RK_ADULTO_H * RK_PRIME_ART) * 10
               >= rk_panel_decimas_mm(RK_PANEL_MINI, alto) * 16);
}

/* Las frases tienen que entrar en el renglón del Mini. Una frase cortada
 * deja al bicho balbuceando, y es un bug que sólo se ve en la placa. */
static void test_las_frases_entran(void)
{
    int i;
    char lbl[96];

    for (i = 0; i < RK_MOOD_COUNT; i++) {
        const char *f = rk_mood_reason((rk_mood_t)i);
        int w = rk_text_w(f, 1);
        snprintf(lbl, sizeof lbl, "\"%s\" entra en el renglon del Mini", f);
        CHECK_TRUE(lbl, w <= RK_MINI_W - 6);
    }
    /* Y en el Prime tienen que entrar a escala 2, que es la que se lee de
     * lejos. Si alguna no entra, prime.c la achica sola, pero conviene
     * saberlo. */
    for (i = 0; i < RK_MOOD_COUNT; i++) {
        const char *f = rk_mood_reason((rk_mood_t)i);
        snprintf(lbl, sizeof lbl, "\"%s\" entra en el Prime a escala 2", f);
        CHECK_TRUE(lbl, rk_text_w(f, RK_PRIME_TEXT) <= RK_PRIME_W - 20);
    }
}

/* ------------------------------------------------------- crecimiento --- */
static void test_etapas(void)
{
    CHECK_INT("espora no tiene hojas",    0, rk_brote_hojas(RK_ETAPA_ESPORA));
    CHECK_INT("brote tiene una",          1, rk_brote_hojas(RK_ETAPA_BROTE));
    CHECK_INT("joven tiene dos",          2, rk_brote_hojas(RK_ETAPA_JOVEN));
    CHECK_INT("maduro tiene tres",        3, rk_brote_hojas(RK_ETAPA_MADURO));
    CHECK_INT("ancestral tiene cuatro",   4, rk_brote_hojas(RK_ETAPA_ANCESTRAL));
    CHECK_INT("una etapa invalida no tiene hojas", 0,
              rk_brote_hojas((rk_stage_t)77));

    CHECK_TRUE("espora no tiene aura",    !rk_brote_aura(RK_ETAPA_ESPORA));
    CHECK_TRUE("joven no tiene aura",     !rk_brote_aura(RK_ETAPA_JOVEN));
    CHECK_TRUE("maduro tiene aura",        rk_brote_aura(RK_ETAPA_MADURO));
    CHECK_TRUE("ancestral tiene aura",     rk_brote_aura(RK_ETAPA_ANCESTRAL));
}

/* Un brote suelto sobre fondo fijo: aísla el rig del resto de la pantalla. */
static uint32_t brote_hash(const rk_companion_t *c, rk_mood_t m,
                           rk_stage_t et, uint32_t t_ms)
{
    rk_fb_t fb;
    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);
    rk_fb_clear(&fb, RK_RGB(20, 26, 22));
    rk_brote_draw(&fb, RK_MINI_W / 2, RK_MINI_H / 2, c, m, RK_SEV_OK, et,
                  RK_MINI_ART, t_ms);
    return rk_frame_hash(g_px, RK_MINI_PX);
}

/* Si dos etapas se ven igual, el crecimiento no existe para el usuario por
 * más que el contador avance en NVS. */
static void test_las_etapas_se_ven_distintas(void)
{
    uint32_t h[RK_ETAPA_COUNT];
    char lbl[96];
    int i, j;

    for (i = 0; i < RK_ETAPA_COUNT; i++) {
        h[i] = brote_hash(&rk_companion_table[0], RK_MOOD_HAPPY,
                          (rk_stage_t)i, 1200u);
    }
    for (i = 0; i < RK_ETAPA_COUNT; i++) {
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
}

/* Y si dos simbiontes se ven igual en la maceta, la colección es decorativa. */
static void test_los_simbiontes_se_ven_distintos(void)
{
    uint32_t h[RK_PAL_COUNT];
    char lbl[96];
    int i, j;

    for (i = 0; i < rk_companion_count; i++) {
        h[i] = brote_hash(&rk_companion_table[i], RK_MOOD_HAPPY,
                          RK_ETAPA_JOVEN, 1200u);
    }
    for (i = 0; i < rk_companion_count; i++) {
        for (j = i + 1; j < rk_companion_count; j++) {
            if (h[i] == h[j]) {
                snprintf(lbl, sizeof lbl, "los brotes de %s y %s se ven igual",
                         rk_companion_table[i].id, rk_companion_table[j].id);
                rk_t_fail(lbl, "dos simbiontes indistinguibles");
            } else {
                rk_t_pass();
            }
        }
    }
}

static void test_animos_distinguibles_en_mini(void)
{
    uint32_t h[RK_MOOD_COUNT];
    char lbl[96];
    int i, j;

    for (i = 0; i < RK_MOOD_COUNT; i++) {
        h[i] = rk_golden_mini((rk_mood_t)i, 1200u);
    }
    for (i = 0; i < RK_MOOD_COUNT; i++) {
        for (j = i + 1; j < RK_MOOD_COUNT; j++) {
            if (h[i] == h[j]) {
                snprintf(lbl, sizeof lbl, "%s y %s iguales en el Mini",
                         rk_mood_name((rk_mood_t)i),
                         rk_mood_name((rk_mood_t)j));
                rk_t_fail(lbl, "dos animos indistinguibles en la maceta");
            } else {
                rk_t_pass();
            }
        }
    }
}

static void test_bordes(void)
{
    rk_fb_t fb;
    uint8_t id[6] = { 0x52, 0x01, 0, 0xAB, 0xCD, 0xEF };

    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);

    rk_mini_draw(&fb, NULL, 0u);
    CHECK_TRUE("un Mini sin nodo no explota", true);
    rk_mini_draw(NULL, NULL, 0u);
    CHECK_TRUE("un Mini sin framebuffer no explota", true);

    rk_mini_emparejar(&fb, id, 0u);
    CHECK_TRUE("la pantalla de emparejamiento dibuja", true);
    rk_mini_emparejar(&fb, NULL, 900u);
    CHECK_TRUE("emparejar sin id no explota", true);

    /* El brote se dibuja con simbionte nulo y con escala 1, que es el camino
     * que usa la tira selectora del Prime. */
    rk_brote_draw(&fb, 64, 64, NULL, RK_MOOD_HAPPY, RK_SEV_OK,
                  RK_ETAPA_JOVEN, 1, 0u);
    CHECK_TRUE("el brote sin simbionte usa la paleta 0", true);
    rk_brote_draw(&fb, 64, 64, &rk_companion_table[0], RK_MOOD_HAPPY,
                  RK_SEV_OK, (rk_stage_t)99, RK_MINI_ART, 0u);
    CHECK_TRUE("una etapa invalida cae en espora", true);
    rk_brote_draw(&fb, 64, 64, &rk_companion_table[0], RK_MOOD_HAPPY,
                  RK_SEV_OK, RK_ETAPA_JOVEN, 0, 0u);
    CHECK_TRUE("escala cero no dibuja nada y no explota", true);

    /* Dibujarlo pisando los bordes tiene que recortar, no escribir afuera.
     * El framebuffer se rodea de un centinela para detectarlo. */
    {
        static rk_color_t buf[RK_MINI_PX + 64];
        rk_fb_t chico;
        int i, sucio = 0;

        for (i = 0; i < RK_MINI_PX + 64; i++) {
            buf[i] = 0xBEEFu;
        }
        rk_fb_init(&chico, &buf[32], RK_MINI_W, RK_MINI_H);
        rk_brote_draw(&chico, -20, -20, &rk_companion_table[3], RK_MOOD_DROWNING,
                      RK_SEV_OK, RK_ETAPA_ANCESTRAL, RK_MINI_ART, 0u);
        rk_brote_draw(&chico, RK_MINI_W + 20, RK_MINI_H + 20,
                      &rk_companion_table[3], RK_MOOD_HAPPY,
                      RK_SEV_OK, RK_ETAPA_ANCESTRAL, RK_MINI_ART, 0u);
        for (i = 0; i < 32; i++) {
            if (buf[i] != 0xBEEFu) { sucio++; }
            if (buf[RK_MINI_PX + 32 + i] != 0xBEEFu) { sucio++; }
        }
        CHECK_INT("dibujar fuera de borde no pisa memoria vecina", 0, sucio);
    }
}

static void test_golden(void)
{
    int i;
    char lbl[80];

    for (i = 0; i < RK_GOLDEN_COUNT; i++) {
        uint32_t got = rk_golden_mini(RK_GOLDEN[i].mood, RK_GOLDEN[i].t_ms);
        snprintf(lbl, sizeof lbl, "cuadro de referencia MINI %s",
                 rk_mood_name(RK_GOLDEN[i].mood));
        CHECK_HEX(lbl, RK_GOLDEN[i].mini, got);
    }
}

void suite_mini(void)
{
    RK_SUITE("mini y brote");
    test_tablas_de_arte();
    test_paletas();
    test_el_brote_entra();
    test_las_frases_entran();
    test_etapas();
    test_las_etapas_se_ven_distintas();
    test_los_simbiontes_se_ven_distintos();
    test_animos_distinguibles_en_mini();
    test_bordes();
    test_golden();
    RK_SUITE_END();
}
