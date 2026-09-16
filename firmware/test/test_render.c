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
#include "../ui/despertar.h"

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
    rk_cara_dormida(NULL, 0u);
    rk_despertar_draw(NULL, NULL, 0u);
    CHECK_TRUE("dormida y despertar sin framebuffer no explotan", true);
}

/* La pantalla muestra el QR y los ojos, nada más. La batería no se dibuja:
 * con la celda crítica y la planta bien, la cara se duerme. */
static void test_bateria_sin_iconos(void)
{
    rk_node_t n;
    rk_fb_t   fb;
    uint32_t  llena, media, critica, dormida;

    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);

    rk_golden_nodo(&n, 0, RK_MOOD_HAPPY);
    n.tel.batt_mv = 4100;
    rk_cara_draw(&fb, &n, 1200u);
    llena = rk_frame_hash(g_px, RK_MINI_PX);

    n.tel.batt_mv = 3420;               /* baja, pero no crítica */
    rk_cara_draw(&fb, &n, 1200u);
    media = rk_frame_hash(g_px, RK_MINI_PX);
    CHECK_HEX("con batería baja la pantalla no cambia: el aviso va a la app",
              llena, media);

    n.tel.batt_mv = 3100;               /* crítica */
    rk_cara_draw(&fb, &n, 1200u);
    critica = rk_frame_hash(g_px, RK_MINI_PX);
    rk_golden_nodo(&n, 0, RK_MOOD_SLEEPING);
    rk_cara_draw(&fb, &n, 1200u);
    dormida = rk_frame_hash(g_px, RK_MINI_PX);
    CHECK_HEX("con la celda crítica la cara contenta se duerme", dormida, critica);

    /* Pero una planta con sed sigue pidiendo agua aunque no haya batería:
     * es lo último que conviene esconder. */
    rk_golden_nodo(&n, 0, RK_MOOD_THIRSTY);
    rk_cara_draw(&fb, &n, 1200u);
    llena = rk_frame_hash(g_px, RK_MINI_PX);
    n.tel.batt_mv = 3100;
    rk_cara_draw(&fb, &n, 1200u);
    CHECK_HEX("la sed se muestra igual con la celda crítica",
              llena, rk_frame_hash(g_px, RK_MINI_PX));
}

/* Dormida antes del cofre: no puede delatar al personaje. */
static void test_dormida_no_delata(void)
{
    rk_fb_t fb;
    uint32_t a, b;
    int i, n = RK_MINI_PX, color_de_piel = 0;

    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);
    rk_cara_dormida(&fb, 1200u);
    a = rk_frame_hash(g_px, RK_MINI_PX);
    for (i = 0; i < n; i++) {
        int k;
        for (k = 0; k < rk_persona_count; k++) {
            if (g_px[i] == rk_persona_at(k)->fondo) {
                color_de_piel++;
            }
        }
    }
    CHECK_INT("la cara dormida no usa la piel de ningún personaje", 0, color_de_piel);
    rk_cara_dormida(&fb, 2600u);
    b = rk_frame_hash(g_px, RK_MINI_PX);
    CHECK_TRUE("y respira: otro instante da otro cuadro", a != b);
}

/* El despertar: negro, ojos cerrados, dos intentos, abiertos. */
static void test_despertar(void)
{
    rk_fb_t fb;
    uint32_t t;
    int subidas = 0;
    uint8_t antes = 100u;
    uint32_t h_cerrado, h_abierto;

    CHECK_INT("arranca con los ojos cerrados", 100, rk_despertar_cierre(0u));
    CHECK_INT("termina con los ojos abiertos", 0, rk_despertar_cierre(RK_DESP_OJOS_MS));
    CHECK_TRUE("no terminó a mitad de escena", !rk_despertar_termino(RK_DESP_OJOS_MS));
    CHECK_TRUE("terminó al final", rk_despertar_termino(RK_DESP_FIN_MS));

    for (t = 0u; t < RK_DESP_FIN_MS; t += 20u) {
        uint8_t c = rk_despertar_cierre(t);
        if (c > antes) {
            subidas++;
        }
        antes = c;
        CHECK_TRUE("el cierre está en rango", c <= 100u);
    }
    /* Hay un tramo en que los párpados vuelven a bajar: el segundo intento. */
    CHECK_TRUE("los ojos se vuelven a cerrar una vez antes de abrirse", subidas > 0);

    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);
    rk_despertar_draw(&fb, rk_persona_at(1), 850u);
    h_cerrado = rk_frame_hash(g_px, RK_MINI_PX);
    rk_despertar_draw(&fb, rk_persona_at(1), 2400u);
    h_abierto = rk_frame_hash(g_px, RK_MINI_PX);
    CHECK_TRUE("cerrado y abierto son cuadros distintos", h_cerrado != h_abierto);

    rk_despertar_draw(&fb, rk_persona_at(1), 100u);
    CHECK_TRUE("en el negro no hay piel todavía",
               g_px[RK_MINI_PX / 2] != rk_persona_at(1)->fondo);
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
    test_bateria_sin_iconos();
    test_dormida_no_delata();
    test_despertar();
    test_golden();
    RK_SUITE_END();
}
