/* Tests del framebuffer monocromo y de la cara del Spore.
 *
 * Dos cosas importan acá y no son obvias:
 *
 *  - El formato de páginas del SSD1306 es fácil de escribir al revés, y un
 *    bit invertido no rompe nada: simplemente la cara sale rayada en el
 *    hardware y no en el simulador. Por eso se verifica el bit exacto.
 *  - En una OLED cada pixel encendido es corriente. Una cara que encienda
 *    media pantalla se come la batería del Spore, así que hay un presupuesto
 *    y se verifica.
 */
#include <stddef.h>
#include "rk_test.h"
#include "../gfx/mono.h"
#include "../art/eyes.h"
#include "../core/companion.h"

static uint8_t   g_buf[RK_OLED_BYTES];
static rk_mono_t g_m;

static void arena(void)
{
    rk_mono_init(&g_m, g_buf, RK_OLED_W, RK_OLED_H);
    rk_mono_clear(&g_m, false);
}

/* --------------------------------------------------------- framebuffer -- */
static void test_formato_paginas(void)
{
    arena();
    CHECK_INT("el buffer son 512 bytes", 512, RK_OLED_BYTES);

    /* El pixel (0,0) tiene que caer en el bit 0 del byte 0, y el (0,8) en el
     * bit 0 del byte 128: así lo espera el SSD1306 y así se vuelca sin
     * conversión. */
    rk_mono_px(&g_m, 0, 0, true);
    CHECK_HEX("el pixel (0,0) es el bit 0 del byte 0", 0x01, g_buf[0]);

    arena();
    rk_mono_px(&g_m, 0, 7, true);
    CHECK_HEX("el pixel (0,7) es el bit 7 del byte 0", 0x80, g_buf[0]);

    arena();
    rk_mono_px(&g_m, 0, 8, true);
    CHECK_HEX("el pixel (0,8) arranca la pagina 1", 0x01, g_buf[128]);
    CHECK_HEX("y no toca la pagina 0", 0x00, g_buf[0]);

    arena();
    rk_mono_px(&g_m, 127, 31, true);
    CHECK_HEX("la esquina opuesta es el bit 7 del ultimo byte", 0x80, g_buf[511]);
}

static void test_lectura_escritura(void)
{
    arena();
    rk_mono_px(&g_m, 40, 20, true);
    CHECK_TRUE("lo que se escribe se lee", rk_mono_get(&g_m, 40, 20));
    CHECK_TRUE("el vecino queda apagado", !rk_mono_get(&g_m, 41, 20));

    rk_mono_px(&g_m, 40, 20, false);
    CHECK_TRUE("apagar funciona", !rk_mono_get(&g_m, 40, 20));

    rk_mono_clear(&g_m, true);
    CHECK_INT("clear encendido prende todo", RK_OLED_W * RK_OLED_H,
              rk_mono_encendidos(&g_m));
    rk_mono_clear(&g_m, false);
    CHECK_INT("clear apagado apaga todo", 0, rk_mono_encendidos(&g_m));
}

static void test_recorte_mono(void)
{
    int i, j, fuera = 0;

    arena();
    /* Coordenadas imposibles en todas las primitivas. Si alguna escribe
     * fuera de rango corrompe la RAM contigua del C3 sin avisar. */
    rk_mono_px(&g_m, -1, 0, true);
    rk_mono_px(&g_m, 0, -1, true);
    rk_mono_px(&g_m, 128, 0, true);
    rk_mono_px(&g_m, 0, 32, true);
    rk_mono_px(&g_m, -500, -500, true);
    rk_mono_hline(&g_m, -50, 16, 300, true);
    rk_mono_vline(&g_m, 64, -50, 300, true);
    rk_mono_fill_rect(&g_m, -40, -40, 400, 400, true);
    rk_mono_rect(&g_m, -10, -10, 300, 300, true);
    rk_mono_disc(&g_m, -20, -20, 60, true);
    rk_mono_disc(&g_m, 200, 60, 60, true);
    rk_mono_fill_round(&g_m, -30, -30, 300, 300, 20, true);
    rk_mono_arco(&g_m, 300, 300, 40, 20, true, true);
    rk_mono_text(&g_m, -60, -20, "FUERA DE RANGO", true);
    CHECK_TRUE("nada de esto rompe", true);

    /* Y los que sí caen adentro tienen que estar adentro. */
    arena();
    rk_mono_hline(&g_m, -50, 16, 300, true);
    for (i = 0; i < RK_OLED_W; i++) {
        if (!rk_mono_get(&g_m, i, 16)) { fuera++; }
    }
    CHECK_INT("una hline desbordada igual pinta el ancho entero", 0, fuera);
    for (j = 0; j < RK_OLED_H; j++) {
        if (j != 16) {
            for (i = 0; i < RK_OLED_W; i++) {
                if (rk_mono_get(&g_m, i, j)) { fuera++; }
            }
        }
    }
    CHECK_INT("y no se derrama a otras filas", 0, fuera);

    CHECK_INT("contar encendidos en NULL devuelve 0", 0, rk_mono_encendidos(NULL));
    CHECK_TRUE("leer de NULL devuelve falso", !rk_mono_get(NULL, 0, 0));
}

static void test_formas(void)
{
    arena();
    rk_mono_fill_rect(&g_m, 10, 10, 20, 10, true);
    CHECK_INT("fill_rect pinta ancho por alto", 200, rk_mono_encendidos(&g_m));

    arena();
    rk_mono_rect(&g_m, 0, 0, 10, 10, true);
    CHECK_INT("rect pinta solo el perimetro", 36, rk_mono_encendidos(&g_m));

    /* El disco tiene que ser simétrico: una raíz entera mal redondeada deja
     * un ojo con un lado más gordo, y a 32 pixeles de alto se nota. */
    arena();
    rk_mono_disc(&g_m, 64, 16, 10, true);
    {
        int y, asim = 0;
        for (y = 0; y < RK_OLED_H; y++) {
            int izq = 0, der = 0, x;
            for (x = 0; x < 64; x++)  { if (rk_mono_get(&g_m, x, y)) { izq++; } }
            for (x = 65; x < 128; x++) { if (rk_mono_get(&g_m, x, y)) { der++; } }
            if (izq != der) { asim++; }
        }
        CHECK_INT("el disco es simetrico", 0, asim);
    }

    arena();
    rk_mono_text(&g_m, 4, 4, "TUGA", true);
    CHECK_TRUE("el texto dibuja algo", rk_mono_encendidos(&g_m) > 20);
}

/* ---------------------------------------------------------------- cara -- */
static void test_caras(void)
{
    int m, vacias = 0, excedidas = 0, peor = 0;
    rk_mood_t peor_mood = RK_MOOD_UNKNOWN;

    for (m = 0; m < RK_MOOD_COUNT; m++) {
        int n;
        arena();
        rk_eyes_draw_stage(&g_m, (rk_mood_t)m, RK_ETAPA_JOVEN, 1200u);
        n = rk_mono_encendidos(&g_m);
        if (n < 20)                    { vacias++; }
        if (n > RK_EYES_MAX_PIXELES)   { excedidas++; }
        if (n > peor) { peor = n; peor_mood = (rk_mood_t)m; }
    }
    printf("         la cara mas cara es %s con %d pixeles de %d\n",
           rk_mood_name(peor_mood), peor, RK_OLED_W * RK_OLED_H);

    CHECK_INT("ninguna cara sale vacia", 0, vacias);
    CHECK_INT("ninguna cara se pasa del presupuesto de pixeles", 0, excedidas);

    /* Un ánimo fuera de rango no debe dejar la pantalla en cualquier cosa. */
    arena();
    rk_eyes_draw_stage(&g_m, (rk_mood_t)999, RK_ETAPA_JOVEN, 0u);
    CHECK_TRUE("un animo invalido cae en algo dibujable",
               rk_mono_encendidos(&g_m) > 20);
    rk_eyes_draw_stage(NULL, RK_MOOD_HAPPY, RK_ETAPA_JOVEN, 0u);
    CHECK_TRUE("dibujar en NULL no explota", true);
}

static void test_caras_distinguibles(void)
{
    /* Si dos ánimos producen la misma cara, el Spore no comunica nada: el
     * usuario mira la maceta y no sabe si tiene sed o frío. Este test
     * encontró que THIRSTY, COLD y HOT compartían dibujo. */
    uint8_t copias[RK_MOOD_COUNT][RK_OLED_BYTES];
    int a, b, iguales = 0;

    for (a = 0; a < RK_MOOD_COUNT; a++) {
        arena();
        rk_eyes_draw_stage(&g_m, (rk_mood_t)a, RK_ETAPA_JOVEN, 1200u);
        memcpy(copias[a], g_buf, RK_OLED_BYTES);
    }
    for (a = 0; a < RK_MOOD_COUNT; a++) {
        for (b = a + 1; b < RK_MOOD_COUNT; b++) {
            if (memcmp(copias[a], copias[b], RK_OLED_BYTES) == 0) {
                char lbl[96];
                snprintf(lbl, sizeof lbl, "%s y %s dibujan lo mismo",
                         rk_mood_name((rk_mood_t)a), rk_mood_name((rk_mood_t)b));
                rk_t_fail(lbl, "dos animos indistinguibles en la OLED");
                iguales++;
            }
        }
    }
    CHECK_INT("los once animos se ven distintos en la OLED", 0, iguales);
}

static void test_crecimiento_visible(void)
{
    /* El ojo crece con la etapa: es la única señal de madurez que hay en
     * monocromo, así que tiene que ser medible, no una intención. */
    int e, prev = 0, no_crece = 0;

    for (e = 0; e < RK_ETAPA_COUNT; e++) {
        int n;
        arena();
        rk_eyes_draw_stage(&g_m, RK_MOOD_DARK, (rk_stage_t)e, 1200u);
        n = rk_mono_encendidos(&g_m);
        if (e > 0 && n <= prev) { no_crece++; }
        prev = n;
    }
    CHECK_INT("cada etapa enciende mas pixeles que la anterior", 0, no_crece);
}

static void test_animacion(void)
{
    uint8_t a[RK_OLED_BYTES], b[RK_OLED_BYTES];

    /* Determinismo: mismo instante, mismo cuadro. */
    arena();
    rk_eyes_draw_stage(&g_m, RK_MOOD_HAPPY, RK_ETAPA_JOVEN, 1200u);
    memcpy(a, g_buf, RK_OLED_BYTES);
    arena();
    rk_eyes_draw_stage(&g_m, RK_MOOD_HAPPY, RK_ETAPA_JOVEN, 1200u);
    CHECK_INT("el mismo instante da el mismo cuadro", 0,
              memcmp(a, g_buf, RK_OLED_BYTES));

    /* El parpadeo cae a los 3700 ms y dura 110. */
    arena();
    rk_eyes_draw_stage(&g_m, RK_MOOD_HAPPY, RK_ETAPA_JOVEN, 3750u);
    memcpy(b, g_buf, RK_OLED_BYTES);
    CHECK_TRUE("parpadea en la ventana esperada",
               memcmp(a, b, RK_OLED_BYTES) != 0);

    /* La mirada pasea: dos instantes lejanos tienen que diferir. */
    arena();
    rk_eyes_draw_stage(&g_m, RK_MOOD_DARK, RK_ETAPA_JOVEN, 500u);
    memcpy(a, g_buf, RK_OLED_BYTES);
    arena();
    rk_eyes_draw_stage(&g_m, RK_MOOD_DARK, RK_ETAPA_JOVEN, 3300u);
    CHECK_TRUE("la pupila se mueve con el tiempo",
               memcmp(a, g_buf, RK_OLED_BYTES) != 0);

    /* Barrido largo: ningún instante debe dejar la pantalla vacía ni pasarse
     * del presupuesto, incluido el medio del parpadeo. */
    {
        uint32_t t;
        int vacios = 0, excedidos = 0;
        for (t = 0u; t < 12000u; t += 53u) {
            int mm;
            for (mm = 0; mm < RK_MOOD_COUNT; mm++) {
                int n;
                arena();
                rk_eyes_draw_stage(&g_m, (rk_mood_t)mm, RK_ETAPA_ANCESTRAL, t);
                n = rk_mono_encendidos(&g_m);
                if (n == 0)                     { vacios++; }
                if (n > RK_EYES_MAX_PIXELES)    { excedidos++; }
            }
        }
        CHECK_INT("ningun instante deja la pantalla en blanco", 0, vacios);
        CHECK_INT("ningun instante se pasa del presupuesto", 0, excedidos);
    }
}

static void test_splash(void)
{
    arena();
    rk_eyes_splash(&g_m, "TUGA.EXE", RK_ETAPA_MADURO);
    CHECK_TRUE("el splash dibuja el nombre y la etapa",
               rk_mono_encendidos(&g_m) > 60);
    rk_eyes_splash(&g_m, NULL, RK_ETAPA_ESPORA);
    CHECK_TRUE("sin nombre tampoco rompe", true);
    rk_eyes_splash(NULL, "X", RK_ETAPA_ESPORA);
    CHECK_TRUE("con framebuffer NULL tampoco", true);
}

void suite_eyes(void)
{
    RK_SUITE("cara del spore");
    test_formato_paginas();
    test_lectura_escritura();
    test_recorte_mono();
    test_formas();
    test_caras();
    test_caras_distinguibles();
    test_crecimiento_visible();
    test_animacion();
    test_splash();
    RK_SUITE_END();
}
