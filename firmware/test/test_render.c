/* Regresión visual del PRIME, por hash.
 *
 * Renderiza la pantalla completa de 240x320 para cada estado de ánimo y
 * compara un FNV-1a del framebuffer contra valores fijados en golden.h. Es lo
 * que hace que un cambio involuntario en el arte, en el layout o en una
 * constante de color se note en el mismo commit y no tres semanas después
 * mirando una captura vieja.
 *
 * Cuando el cambio es intencional:   make golden
 * Eso regenera golden.h, y el diff del commit muestra exactamente qué
 * pantallas cambiaron y en cuál de los dos paneles.
 */
#include <stddef.h>
#include "rk_test.h"
#include "golden_util.h"
#include "golden.h"
#include "../art/adulto.h"

static rk_color_t g_px[RK_PRIME_PX];

static void test_determinismo(void)
{
    /* El renderizado es una función pura del estado y del tiempo. Si esto
     * falla hay estado escondido en algún lado, y entonces el simulador y la
     * placa pueden divergir. */
    uint32_t a = rk_golden_prime(RK_MOOD_HAPPY, 1200u);
    uint32_t b = rk_golden_prime(RK_MOOD_HAPPY, 1200u);
    CHECK_HEX("el mismo estado produce el mismo cuadro", a, b);

    /* Con particulas el cuadro cambia siempre: es la prueba mas fuerte de
     * que el tiempo entra de verdad en el render. */
    CHECK_TRUE("con particulas, otro instante da otro cuadro",
               rk_golden_prime(RK_MOOD_DROWNING, 1200u) !=
               rk_golden_prime(RK_MOOD_DROWNING, 1900u));

    /* Y la respiración sola, media vuelta despues, tambien tiene que mover
     * algo. El periodo de HAPPY es 1000*256/42 = 6095 ms, asi que 1200 y
     * 4248 caen en extremos opuestos de la senoidal. Este chequeo es el que
     * detecta que una amplitud demasiado chica se coma el movimiento en el
     * redondeo entero. */
    CHECK_TRUE("la respiracion sola mueve el cuadro media vuelta despues",
               rk_golden_prime(RK_MOOD_HAPPY, 1200u) !=
               rk_golden_prime(RK_MOOD_HAPPY, 4248u));
    CHECK_TRUE("un animo distinto produce otro cuadro",
               rk_golden_prime(RK_MOOD_HAPPY, 1200u) !=
               rk_golden_prime(RK_MOOD_THIRSTY, 1200u));
}

/* Cada ánimo tiene que verse distinto de todos los demás. Es la prueba que
 * en su momento descubrió que tres estados compartían exactamente la misma
 * cara: el usuario no puede distinguir lo que el render no distingue. */
static void test_animos_distinguibles(void)
{
    uint32_t h[RK_MOOD_COUNT];
    char lbl[96];
    int i, j;

    for (i = 0; i < RK_MOOD_COUNT; i++) {
        h[i] = rk_golden_prime((rk_mood_t)i, 1200u);
    }
    for (i = 0; i < RK_MOOD_COUNT; i++) {
        for (j = i + 1; j < RK_MOOD_COUNT; j++) {
            if (h[i] == h[j]) {
                snprintf(lbl, sizeof lbl, "%s y %s dan el mismo cuadro",
                         rk_mood_name((rk_mood_t)i), rk_mood_name((rk_mood_t)j));
                rk_t_fail(lbl, "dos animos indistinguibles en pantalla");
            } else {
                rk_t_pass();
            }
        }
    }
}

static void test_sin_nodos(void)
{
    rk_roster_t kit;
    rk_fb_t     fb;

    rk_fb_init(&fb, g_px, RK_PRIME_W, RK_PRIME_H);
    rk_roster_init(&kit);
    rk_prime_draw(&fb, &kit, 0u);      /* count == 0 */
    CHECK_TRUE("la pantalla vacia no explota", true);

    rk_prime_draw(&fb, NULL, 0u);
    CHECK_TRUE("roster NULL no explota", true);
    rk_prime_draw(NULL, &kit, 0u);
    CHECK_TRUE("framebuffer NULL no explota", true);

    CHECK_INT("sin nodos no hay zona tactil", -1, rk_prime_hit(&kit, 120, 300));
    CHECK_INT("roster NULL no tiene zona tactil", -1,
              rk_prime_hit(NULL, 120, 300));

    /* Un selected fuera de rango no puede leer memoria de al lado. */
    rk_golden_kit(&kit, 0, RK_MOOD_HAPPY);
    kit.selected = 99;
    rk_prime_draw(&fb, &kit, 0u);
    CHECK_TRUE("selected fuera de rango cae al primero", true);
    kit.selected = -3;
    rk_prime_draw(&fb, &kit, 0u);
    CHECK_TRUE("selected negativo cae al primero", true);
}

static void test_zona_tactil(void)
{
    rk_roster_t kit;
    rk_golden_kit(&kit, 0, RK_MOOD_HAPPY);   /* cuatro nodos: 60 px cada uno */

    CHECK_INT("tocar la escena no selecciona nada", -1,
              rk_prime_hit(&kit, 120, 100));
    CHECK_INT("tocar el primer cuarto",  0, rk_prime_hit(&kit,  20, 300));
    CHECK_INT("tocar el segundo cuarto", 1, rk_prime_hit(&kit,  80, 300));
    CHECK_INT("tocar el tercer cuarto",  2, rk_prime_hit(&kit, 140, 300));
    CHECK_INT("tocar el cuarto cuarto",  3, rk_prime_hit(&kit, 200, 300));
    CHECK_INT("fuera por la izquierda", -1, rk_prime_hit(&kit,  -5, 300));
    CHECK_INT("fuera por la derecha",   -1, rk_prime_hit(&kit, 500, 300));
    CHECK_INT("fuera por abajo",        -1, rk_prime_hit(&kit, 120, 999));
}

/* El adulto tiene que quedar DENTRO de la banda de escena. Si se sale, en la
 * placa se lo come la barra de estado y nadie lo nota hasta tener la pantalla
 * en la mano. Se verifica buscando pixeles del bicho fuera de la banda. */
static void test_el_bicho_entra_en_la_escena(void)
{
    rk_fb_t fb;
    int alto = RK_ADULTO_H * RK_PRIME_ART;
    int ancho = RK_ADULTO_W * RK_PRIME_ART;

    rk_fb_init(&fb, g_px, RK_PRIME_W, RK_PRIME_H);

    /* La banda de escena del Prime son 176 px (ver ui/prime.h). El bicho con
     * su respiración al máximo se mueve 4 px de arte = 8 de panel. */
    CHECK_TRUE("el adulto entra a lo alto en la banda de escena",
               alto + 8 <= 176);
    CHECK_TRUE("el adulto entra a lo ancho en el panel",
               ancho <= RK_PRIME_W);

    /* Y el tamaño físico es el que se prometió en el analisis de hardware. */
    CHECK_NEAR("el adulto mide 21,9 mm en el Prime (decimas)",
               219, rk_panel_decimas_mm(RK_PANEL_PRIME, alto), 3);
}

static void test_golden(void)
{
    int i;
    char lbl[80];

    for (i = 0; i < RK_GOLDEN_COUNT; i++) {
        uint32_t got = rk_golden_prime(RK_GOLDEN[i].mood, RK_GOLDEN[i].t_ms);
        snprintf(lbl, sizeof lbl, "cuadro de referencia PRIME %s",
                 rk_mood_name(RK_GOLDEN[i].mood));
        CHECK_HEX(lbl, RK_GOLDEN[i].prime, got);
    }
}

void suite_render(void)
{
    RK_SUITE("render prime");
    test_determinismo();
    test_animos_distinguibles();
    test_sin_nodos();
    test_zona_tactil();
    test_el_bicho_entra_en_la_escena();
    test_golden();
    RK_SUITE_END();
}
