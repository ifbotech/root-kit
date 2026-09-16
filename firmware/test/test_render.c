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
#include "../art/face.h"

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

/* Un cuadro de la transición entre dos ánimos, por hash. */
static uint32_t hash_mezcla(int persona, rk_mood_t desde, rk_mood_t hacia,
                            uint8_t pct, uint32_t t_ms)
{
    rk_fb_t fb;
    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);
    rk_face_draw_mezcla(&fb, rk_persona_at(persona), desde, hacia, pct,
                        RK_SEV_OK, 0u, 0u, t_ms);
    return rk_frame_hash(g_px, RK_MINI_PX);
}

static uint32_t hash_cara(int persona, rk_mood_t mood, uint32_t t_ms)
{
    rk_fb_t fb;
    rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);
    rk_face_draw(&fb, rk_persona_at(persona), mood, RK_SEV_OK, 0u, t_ms);
    return rk_frame_hash(g_px, RK_MINI_PX);
}

/* La cara no salta de un ánimo a otro: pasa por el medio. */
static void test_transicion(void)
{
    rk_face_geom_t a = { 100, 100, 0, 0, 0, -100, 0, 0, 0, 100 };
    rk_face_geom_t b = { 50, 60, 44, -10, 22, 100, 45, 12, 3, -100 };
    rk_face_geom_t m = rk_face_geom_lerp(&a, &b, 50u);
    rk_face_geom_t z = rk_face_geom_lerp(&a, &b, 0u);
    rk_face_geom_t f = rk_face_geom_lerp(&a, &b, 100u);
    rk_cara_anim_t an;
    uint32_t h_a, h_b, h_m, h1, h2, h3;
    int p, i, ok;
    char lbl[96];

    CHECK_INT("a mitad de camino la boca esta recta", 0, m.boca_curva);
    CHECK_INT("y el parpado a medio bajar", 22, m.tapa_sup);
    CHECK_INT("y la mirada al centro", 0, m.mira_x);
    CHECK_TRUE("en 0 es exactamente el origen", memcmp(&z, &a, sizeof a) == 0);
    CHECK_TRUE("en 100 es exactamente el destino", memcmp(&f, &b, sizeof b) == 0);
    CHECK_INT("mas de 100 satura", b.abre, rk_face_geom_lerp(&a, &b, 250u).abre);

    CHECK_INT("ease en 0", 0, rk_face_ease(0u));
    CHECK_INT("ease en 50", 50, rk_face_ease(50u));
    CHECK_INT("ease en 100", 100, rk_face_ease(100u));
    CHECK_TRUE("arranca despacio", rk_face_ease(25u) < 20u);
    CHECK_TRUE("termina despacio", rk_face_ease(75u) > 80u);
    ok = 1;
    for (i = 1; i <= 100; i++) {
        if (rk_face_ease((uint8_t)i) < rk_face_ease((uint8_t)(i - 1))) { ok = 0; }
    }
    CHECK_TRUE("la curva nunca retrocede", ok);

    /* Los extremos de la mezcla son las caras de siempre, pixel por pixel:
     * la transición no cambia lo que ya estaba fijado en golden.h. */
    for (p = 0; p < rk_persona_count; p++) {
        h_a = hash_cara(p, RK_MOOD_HAPPY, 1200u);
        h_b = hash_cara(p, RK_MOOD_THIRSTY, 1200u);
        snprintf(lbl, sizeof lbl, "%s: en 0 es la cara de origen", rk_persona_at(p)->id);
        CHECK_HEX(lbl, h_a, hash_mezcla(p, RK_MOOD_HAPPY, RK_MOOD_THIRSTY, 0u, 1200u));
        snprintf(lbl, sizeof lbl, "%s: en 100 es la cara de destino", rk_persona_at(p)->id);
        CHECK_HEX(lbl, h_b, hash_mezcla(p, RK_MOOD_HAPPY, RK_MOOD_THIRSTY, 100u, 1200u));
        snprintf(lbl, sizeof lbl, "%s: mismo animo con mezcla es la misma cara", rk_persona_at(p)->id);
        CHECK_HEX(lbl, h_a, hash_mezcla(p, RK_MOOD_HAPPY, RK_MOOD_HAPPY, 37u, 1200u));
        h_m = hash_mezcla(p, RK_MOOD_HAPPY, RK_MOOD_THIRSTY, 30u, 1200u);
        snprintf(lbl, sizeof lbl, "%s: en el medio es otra cara", rk_persona_at(p)->id);
        CHECK_TRUE(lbl, h_m != h_a && h_m != h_b);
    }
    /* Y el medio se mueve: tres instantes, tres cuadros distintos. */
    h1 = hash_mezcla(0, RK_MOOD_HAPPY, RK_MOOD_COLD, 20u, 1200u);
    h2 = hash_mezcla(0, RK_MOOD_HAPPY, RK_MOOD_COLD, 50u, 1200u);
    h3 = hash_mezcla(0, RK_MOOD_HAPPY, RK_MOOD_COLD, 80u, 1200u);
    CHECK_TRUE("la transicion avanza", h1 != h2 && h2 != h3 && h1 != h3);
    /* Entre sonrisa y mueca, la boca pasa por todas las curvas sin saltar:
     * cada instante intermedio es distinto del anterior. */
    h1 = hash_mezcla(1, RK_MOOD_HAPPY, RK_MOOD_SCORCHED, 10u, 1200u);
    h2 = hash_mezcla(1, RK_MOOD_HAPPY, RK_MOOD_SCORCHED, 20u, 1200u);
    CHECK_TRUE("la boca se curva de a poco", h1 != h2);

    /* El reloj de la transición. */
    rk_cara_anim_iniciar(&an, RK_MOOD_HAPPY, 1000u);
    CHECK_INT("recien iniciada no hay transicion", 100, rk_cara_anim_pct(&an, 1000u));
    rk_cara_anim_animo(&an, RK_MOOD_HAPPY, 1500u);
    CHECK_TRUE("el mismo animo no arranca nada", !rk_cara_anim_en_curso(&an, 1500u));
    rk_cara_anim_animo(&an, RK_MOOD_THIRSTY, 2000u);
    CHECK_TRUE("cambiar de animo arranca la transicion", rk_cara_anim_en_curso(&an, 2000u));
    CHECK_INT("arranca en 0", 0, rk_cara_anim_pct(&an, 2000u));
    CHECK_INT("a mitad de tiempo va por la mitad", 50, rk_cara_anim_pct(&an, 2000u + RK_CARA_TRANSICION_MS / 2u));
    CHECK_TRUE("tarda lo que dice",
               rk_cara_anim_pct(&an, 2000u + RK_CARA_TRANSICION_MS - 1u) < 100u &&
               rk_cara_anim_pct(&an, 2000u + RK_CARA_TRANSICION_MS) == 100u);
    CHECK_TRUE("y despues termino", !rk_cara_anim_en_curso(&an, 3000u));
    ok = 1;
    for (i = 1; i <= (int)RK_CARA_TRANSICION_MS; i++) {
        if (rk_cara_anim_pct(&an, 2000u + (uint32_t)i) < rk_cara_anim_pct(&an, 2000u + (uint32_t)i - 1u)) { ok = 0; }
    }
    CHECK_TRUE("nunca retrocede", ok);
    /* Interrumpida pasada la mitad, sigue desde el destino que ya dominaba. */
    rk_cara_anim_animo(&an, RK_MOOD_COLD, 2000u + 300u);
    CHECK_INT("interrumpida: viene del animo que dominaba", RK_MOOD_THIRSTY, an.desde);
    CHECK_INT("y va al nuevo", RK_MOOD_COLD, an.hacia);
    /* Interrumpida antes de la mitad, vuelve al origen. */
    rk_cara_anim_animo(&an, RK_MOOD_HOT, 2000u + 300u + 40u);
    CHECK_INT("interrumpida temprano: vuelve al origen", RK_MOOD_THIRSTY, an.desde);
    /* Desborde del reloj: una transición que empezó justo antes de que el
     * contador diera la vuelta termina igual. */
    rk_cara_anim_iniciar(&an, RK_MOOD_HAPPY, 0xFFFFFFF0u);
    rk_cara_anim_animo(&an, RK_MOOD_THIRSTY, 0xFFFFFFF0u);
    CHECK_INT("aguanta el desborde del reloj", 100, rk_cara_anim_pct(&an, 400u));

    /* Con un nodo: el cuadro a mitad de transición no es ninguno de los dos. */
    {
        rk_node_t n;
        rk_fb_t fb;
        uint32_t hx;
        rk_golden_nodo(&n, 0, RK_MOOD_HAPPY);
        rk_fb_init(&fb, g_px, RK_MINI_W, RK_MINI_H);
        memset(&an, 0, sizeof an);
        rk_cara_draw_anim(&fb, &n, &an, 0u, 5000u);
        h_a = rk_frame_hash(g_px, RK_MINI_PX);
        CHECK_HEX("sin transicion es la cara de siempre", rk_golden_cara(0, RK_MOOD_HAPPY, 5000u), h_a);
        n.verdict.mood = RK_MOOD_THIRSTY;
        rk_cara_draw_anim(&fb, &n, &an, 0u, 5100u);
        hx = rk_frame_hash(g_px, RK_MINI_PX);
        rk_cara_draw_anim(&fb, &n, &an, 0u, 5100u + 100u);
        h_m = rk_frame_hash(g_px, RK_MINI_PX);
        h_b = rk_golden_cara(0, RK_MOOD_THIRSTY, 5100u + 100u);
        CHECK_HEX("al cambiar el animo arranca desde la cara anterior",
                  rk_golden_cara(0, RK_MOOD_HAPPY, 5100u), hx);
        CHECK_TRUE("a los 100 ms no es ninguna de las dos", h_m != h_b && h_m != hash_cara(0, RK_MOOD_HAPPY, 5200u));
        rk_cara_draw_anim(&fb, &n, &an, 0u, 5100u + RK_CARA_TRANSICION_MS);
        CHECK_HEX("al terminar es la cara nueva", rk_golden_cara(0, RK_MOOD_THIRSTY, 5100u + RK_CARA_TRANSICION_MS),
                  rk_frame_hash(g_px, RK_MINI_PX));
        rk_cara_draw_anim(NULL, &n, &an, 0u, 0u);
        rk_cara_draw_anim(&fb, NULL, &an, 0u, 0u);
        rk_cara_anim_draw(&fb, NULL, NULL, RK_SEV_OK, 0u, 0u, 0u);
        CHECK_TRUE("NULL no explota", true);
    }
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
    test_transicion();
    test_golden();
    RK_SUITE_END();
}
