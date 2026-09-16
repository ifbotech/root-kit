/* Regresión visual de la cara, por hash.
 *
 * Renderiza la pantalla completa de cada MODELO en cada ÁNIMO y compara un
 * FNV-1a del framebuffer contra los valores fijados en golden.h. Es lo que
 * hace que un cambio involuntario —en el rig, en una paleta, en una
 * constante de layout— se note en el mismo commit y no tres semanas después
 * mirando una captura vieja.
 *
 * Cuando el cambio es intencional:   make golden
 */
#include <stddef.h>
#include "rk_test.h"
#include "golden_util.h"
#include "golden.h"
#include "../nodo/power.h"

static rk_color_t g_px[RK_MINI_PX];

static void test_determinismo(void)
{
    /* Función pura del estado y del tiempo. Si esto falla hay estado
     * escondido, y entonces el simulador y la placa pueden divergir. */
    CHECK_HEX("el mismo estado produce el mismo cuadro",
              rk_golden_cara(0, RK_MOOD_HAPPY, 1200u),
              rk_golden_cara(0, RK_MOOD_HAPPY, 1200u));

    /* La mirada deriva y la respiración mueve la cara, así que otro instante
     * tiene que dar otro cuadro. Es la prueba de que el tiempo entra de
     * verdad en el render y la cara no está congelada. */
    CHECK_TRUE("otro instante da otro cuadro",
               rk_golden_cara(0, RK_MOOD_HAPPY, 1200u) !=
               rk_golden_cara(0, RK_MOOD_HAPPY, 4248u));
    CHECK_TRUE("otro animo da otro cuadro",
               rk_golden_cara(0, RK_MOOD_HAPPY, 1200u) !=
               rk_golden_cara(0, RK_MOOD_THIRSTY, 1200u));
    CHECK_TRUE("otro modelo da otro cuadro",
               rk_golden_cara(0, RK_MOOD_HAPPY, 1200u) !=
               rk_golden_cara(1, RK_MOOD_HAPPY, 1200u));
}

static void test_pantalla_sin_nodo(void)
{
    rk_fb_t fb;

    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);
    rk_cara_draw(&fb, NULL, 0u);
    CHECK_TRUE("sin nodo la pantalla no explota", true);
    rk_cara_draw(NULL, NULL, 0u);
    CHECK_TRUE("sin framebuffer tampoco", true);

    rk_cara_emparejar(&fb, NULL, NULL, 0u);
    CHECK_TRUE("emparejar sin id ni modelo no explota", true);
    {
        uint8_t id[6] = { 0x52, 0, 0, 0xAB, 0xCD, 0xEF };
        rk_cara_emparejar(&fb, id, rk_persona_at(2), 900u);
        CHECK_TRUE("la pantalla de emparejamiento dibuja", true);
    }
}

/* El aviso de batería es lo único que la app no puede resolver sola, así que
 * tiene que aparecer y tiene que parpadear. */
static void test_aviso_de_bateria(void)
{
    rk_node_t n;
    rk_fb_t   fb;
    uint32_t  llena, vacia_on, vacia_off;

    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);

    /* Las tres capturas van en el MISMO instante salvo donde se compara el
     * parpadeo. La cara se anima sola —la mirada deriva, la respiracion
     * mueve todo— asi que comparar dos instantes distintos no dice nada
     * sobre el aviso de bateria: dice que paso el tiempo. */
    rk_golden_nodo(&n, 0, RK_MOOD_HAPPY);
    n.tel.batt_mv = 4100;               /* celda llena */
    rk_cara_draw(&fb, &n, 0u);
    llena = rk_frame_hash(g_px, RK_MINI_PX);

    n.tel.batt_mv = 3250;               /* por debajo del aviso */
    CHECK_TRUE("3250 mV esta por debajo del umbral de aviso",
               rk_batt_pct(3250) < 15);
    rk_cara_draw(&fb, &n, 0u);
    vacia_on = rk_frame_hash(g_px, RK_MINI_PX);

    CHECK_TRUE("con la celda baja aparece el aviso", llena != vacia_on);

    /* Medio periodo despues el aviso se apaga. Se compara contra la celda
     * llena EN ESE MISMO INSTANTE, que es lo unico que aisla el aviso del
     * resto de la animacion. */
    rk_cara_draw(&fb, &n, 900u);
    vacia_off = rk_frame_hash(g_px, RK_MINI_PX);
    n.tel.batt_mv = 4100;
    rk_cara_draw(&fb, &n, 900u);
    CHECK_TRUE("y parpadea, que es lo que lo hace avisar",
               vacia_on != vacia_off);
    CHECK_HEX("apagado, la pantalla es la de siempre",
              rk_frame_hash(g_px, RK_MINI_PX), vacia_off);
}

static void test_golden(void)
{
    int i;
    char lbl[96];

    CHECK_INT("la tabla cubre todos los modelos y animos",
              rk_persona_count * RK_MOOD_COUNT, RK_GOLDEN_COUNT);

    for (i = 0; i < RK_GOLDEN_COUNT; i++) {
        uint32_t got = rk_golden_cara(RK_GOLDEN[i].persona,
                                      RK_GOLDEN[i].mood, RK_GOLDEN[i].t_ms);
        snprintf(lbl, sizeof lbl, "referencia %s / %s",
                 rk_persona_at(RK_GOLDEN[i].persona)->nombre,
                 rk_mood_name(RK_GOLDEN[i].mood));
        CHECK_HEX(lbl, RK_GOLDEN[i].hash, got);
    }
}

void suite_render(void)
{
    RK_SUITE("cara");
    test_determinismo();
    test_pantalla_sin_nodo();
    test_aviso_de_bateria();
    test_golden();
    RK_SUITE_END();
}
