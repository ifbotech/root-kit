/* Regresión visual por hash.
 *
 * Renderiza la pantalla completa para cada estado de ánimo y compara un
 * FNV-1a del framebuffer contra valores fijados en golden.h. Es lo que hace
 * que un cambio involuntario en el arte, en el layout o en una constante de
 * color se note en el mismo commit y no tres semanas después mirando una
 * captura vieja.
 *
 * Cuando el cambio es intencional:   make golden
 * Eso regenera golden.h, y el diff del commit muestra exactamente qué
 * pantallas cambiaron.
 */
#include <stddef.h>
#include "rk_test.h"
#include <string.h>
#include "golden_util.h"
#include "golden.h"

static void test_determinismo(void)
{
    /* El renderizado es una función pura del estado y del tiempo. Si esto
     * falla hay estado escondido en algún lado, y entonces el simulador y la
     * Terminal pueden divergir. */
    uint32_t a = rk_golden_render(RK_MOOD_HAPPY, 1200u);
    uint32_t b = rk_golden_render(RK_MOOD_HAPPY, 1200u);
    CHECK_HEX("el mismo estado produce el mismo cuadro", a, b);

    /* Con particulas el cuadro cambia siempre: es la prueba mas fuerte de
     * que el tiempo entra de verdad en el render. */
    CHECK_TRUE("con particulas, otro instante da otro cuadro",
               rk_golden_render(RK_MOOD_DROWNING, 1200u) !=
               rk_golden_render(RK_MOOD_DROWNING, 1900u));

    /* Y la respiración sola, media vuelta despues, tambien tiene que mover
     * algo. El periodo de HAPPY es 1000*256/42 = 6095 ms, asi que 1200 y
     * 4248 caen en extremos opuestos de la senoidal. Este chequeo es el que
     * detecta que una amplitud demasiado chica se coma el movimiento en el
     * redondeo entero. */
    CHECK_TRUE("la respiracion sola mueve el cuadro media vuelta despues",
               rk_golden_render(RK_MOOD_HAPPY, 1200u) !=
               rk_golden_render(RK_MOOD_HAPPY, 4248u));
    CHECK_TRUE("un animo distinto produce otro cuadro",
               rk_golden_render(RK_MOOD_HAPPY, 1200u) !=
               rk_golden_render(RK_MOOD_THIRSTY, 1200u));
}

static void test_sin_plantas(void)
{
    rk_state_t st;
    rk_fb_t    fb;

    static rk_color_t px[RK_CANVAS_W * RK_CANVAS_H];
    rk_fb_init(&fb, px, RK_CANVAS_W, RK_CANVAS_H);
    memset(&st, 0, sizeof st);
    rk_screen_draw(&fb, &st, 0u);      /* count == 0 */
    CHECK_TRUE("la pantalla vacia no explota", true);

    rk_screen_draw(&fb, NULL, 0u);
    CHECK_TRUE("estado NULL no explota", true);

    CHECK_INT("sin plantas no hay zona tactil", -1, rk_screen_hit(&st, 80, 230));
    CHECK_INT("estado NULL no tiene zona tactil", -1, rk_screen_hit(NULL, 80, 230));
}

static void test_zona_tactil(void)
{
    rk_state_t st;
    rk_golden_state(&st, RK_MOOD_HAPPY);

    CHECK_INT("tocar arriba no selecciona nada", -1, rk_screen_hit(&st, 80, 40));
    CHECK_INT("tocar el primer tercio",  0, rk_screen_hit(&st, 20,  235));
    CHECK_INT("tocar el segundo tercio", 1, rk_screen_hit(&st, 80,  235));
    CHECK_INT("tocar el tercer tercio",  2, rk_screen_hit(&st, 140, 235));
    CHECK_INT("fuera por la izquierda",  -1, rk_screen_hit(&st, -5, 235));
    CHECK_INT("fuera por la derecha",    -1, rk_screen_hit(&st, 500, 235));
}

static void test_golden(void)
{
    int i;
    char lbl[80];

    for (i = 0; i < RK_GOLDEN_COUNT; i++) {
        uint32_t got = rk_golden_render(RK_GOLDEN[i].mood, RK_GOLDEN[i].t_ms);
        snprintf(lbl, sizeof lbl, "cuadro de referencia %s",
                 rk_mood_name(RK_GOLDEN[i].mood));
        CHECK_HEX(lbl, RK_GOLDEN[i].hash, got);
    }
}

void suite_render(void)
{
    RK_SUITE("render");
    test_determinismo();
    test_sin_plantas();
    test_zona_tactil();
    test_golden();
    RK_SUITE_END();
}
